/*
 * Copyright 2003-2013 Jeffrey K. Hollingsworth
 *
 * This file is part of Active Harmony.
 *
 * Active Harmony is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Active Harmony is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Active Harmony.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "session-core.h"
#include "hsession.h"
#include "hcfg.h"
#include "hmesg.h"
#include "hutil.h"
#include "hsockutil.h"
#include "defaults.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

/* ISO C forbids conversion of object pointer to function pointer.  We
 * get around this by first casting to a word-length integer.  (LP64
 * compilers assumed).
 */
#define HACK_CAST_INT(x) (int (*)(hmesg_t *))(long)(x)
#define HACK_CAST_VOID(x) (void (*)(void))(long)(x)

/* Session structure exported to strategies and plugins. */
hsession_t *sess;

/* Be sure all remaining global definitions are declared static to
 * avoid possible namspace conflicts in the GOT due to PIC behavior.
 */

/* Forward function declarations. */
static int strategy_load(hmesg_t *mesg);
static int plugin_list_load(hmesg_t *mesg, const char *list);
static int prefetch_init(hmesg_t *mesg);
static int plugin_workflow(hmesg_t *mesg);
static int point_enqueue(hmesg_t *mesg);
static int point_dequeue(hmesg_t *mesg);

/* Callback registration structures. */
typedef struct callback {
    int fd;
    int (*func)(hmesg_t *);
} callback_t;

static callback_t *cb = NULL;
static int cb_len = 0;
static int cb_cap = 0;

/* Strategy hooks. */
static int (*strategy_fetch)(hmesg_t *);
static int (*strategy_report)(hmesg_t *);
static hpoint_t *strategy_best;

/* Plugin system. */
typedef struct plugin {
    int (*report)(hmesg_t *mesg);
    int (*fetch)(hmesg_t *mesg);
    void (*fini)(void);
    const char *name;
} plugin_t;

static plugin_t *plugin = NULL;
static int plugin_len = 0;
static int plugin_cap = 0;

/* Prefetching variables. */
static hpoint_t *pfqueue;
static int pfq_cap, pfq_tail, pfq_head;
static int pfq_requested, pfq_reported, pfq_atomic;

/* Variables used for select(). */
static struct timeval polltime, *pollstate;
static fd_set fds;
static int maxfd;

/* =================================
 * Core session routines begin here.
 */

int main(int argc, char **argv)
{
    struct stat sb;
    int i, retval;
    fd_set ready_fds;
    char *key, *val;
    const char *list;
    hmesg_t session_mesg, mesg;

    /* Check that we have been launched correctly by checking that
     * STDIN_FILENO is a socket descriptor.
     *
     * Print an error message to stderr if this is not the case.  This
     * should be the only time an error message is printed to stdout
     * or stderr.
     */
    if (fstat(STDIN_FILENO, &sb) < 0) {
        perror("Could not determine the status of STDIN");
        return -1;
    }

    if (!S_ISSOCK(sb.st_mode)) {
        fprintf(stderr, "%s should not be launched manually.\n", argv[0]);
        return -1;
    }

    /* Initialize data structures. */
    mesg = HMESG_INITIALIZER;
    session_mesg = HMESG_INITIALIZER;
    pollstate = &polltime;

    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    maxfd = STDIN_FILENO;

    if (array_grow(&cb, &cb_cap, sizeof(callback_t)) < 0)
        goto error;

    if (array_grow(&plugin, &plugin_cap, sizeof(plugin_t)) < 0)
        goto error;

    /* Receive initial session message. */
    if (mesg_recv(STDIN_FILENO, &session_mesg) < 1) {
        mesg.type = HMESG_SESSION;
        mesg.data.string = "Socket or deserialization error";
        goto error;
    }
    mesg.dest = session_mesg.dest;
    mesg.type = session_mesg.type;

    if (session_mesg.type != HMESG_SESSION ||
        session_mesg.status != HMESG_STATUS_REQ)
    {
        mesg.data.string = "Invalid initial message";
        goto error;
    }

    /* Load and initialize the strategy code object. */
    sess = &session_mesg.data.session;
    if (strategy_load(&session_mesg) < 0) {
        mesg.data.string = session_mesg.data.string;
        goto error;
    }

    /* Load and initialize requested plugin's. */
    list = hcfg_get(sess->cfg, CFGKEY_SESSION_PLUGINS);
    if (list && plugin_list_load(&session_mesg, list) < 0) {
        mesg.data.string = session_mesg.data.string;
        goto error;
    }

    session_mesg.status = HMESG_STATUS_OK;
    if (mesg_send(STDIN_FILENO, &session_mesg) < 1) {
        mesg.data.string = session_mesg.data.string;
        goto error;
    }

    /* Initialize prefetching.
     *
     * This must happen after the strategy and all plugin's have been
     * initialize, since the request to prefetch points might be coming
     * from them.
     */
    if (prefetch_init(&mesg) < 0)
        goto error;

    while (1) {
        ready_fds = fds;
        retval = select(maxfd + 1, &ready_fds, NULL, NULL, pollstate);
        if (retval < 0)
            goto error;

        /* Launch callbacks, if needed. */
        for (i = 0; i < cb_len; ++i) {
            if (FD_ISSET(cb[i].fd, &ready_fds)) {
                if (mesg_recv(cb[i].fd, &mesg) < 1)
                    goto error;

                if (cb[i].func && cb[i].func(&mesg) < 0) {
                    if (mesg.status == HMESG_STATUS_BUSY)
                        continue;
                    if (mesg.status == HMESG_STATUS_FAIL)
                        goto error;
                }
                ++mesg.index;

                if (plugin_workflow(&mesg) < 0) {
                    if (mesg.status == HMESG_STATUS_FAIL)
                        goto error;
                }

                if (mesg.type == HMESG_FETCH &&
                    mesg.status == HMESG_STATUS_OK)
                {
                    if (pfq_cap) {
                        if (point_enqueue(&mesg) < 0)
                            goto error;
                    }
                    else {
                        if (mesg_send(STDIN_FILENO, &mesg) < 1)
                            goto error;
                    }
                }
            }
        }

        /* Handle hmesg_t, if needed. */
        if (FD_ISSET(STDIN_FILENO, &ready_fds)) {
            if (mesg_recv(STDIN_FILENO, &mesg) < 1)
                goto error;

            switch (mesg.type) {
            case HMESG_JOIN:
                if (hsignature_equal(&mesg.data.join, &sess->sig)) {
                    if (hsignature_copy(&mesg.data.join, &sess->sig) < 0)
                        goto error;
                    mesg.status = HMESG_STATUS_OK;
                }
                else {
                    mesg.status = HMESG_STATUS_FAIL;
                    mesg.data.string = "Incompatible join signature.";
                }
                if (mesg_send(STDIN_FILENO, &mesg) < 1)
                    goto error;
                break;

            case HMESG_QUERY:
                mesg.data.string = hcfg_get(sess->cfg, mesg.data.string);
                mesg.status = HMESG_STATUS_OK;
                if (mesg_send(STDIN_FILENO, &mesg) < 1)
                    goto error;
                break;

            case HMESG_INFORM:
                hcfg_parse((char *)mesg.data.string, &key, &val);
                if (key) {
                    mesg.data.string = hcfg_get(sess->cfg, key);
                    if (hcfg_set(sess->cfg, key, val) < 0) {
                        mesg.status = HMESG_STATUS_FAIL;
                        mesg.data.string = "Internal hcfg error";
                    }
                    else {
                        mesg.status = HMESG_STATUS_OK;
                    }
                }
                else {
                    mesg.data.string = strerror(EINVAL);
                    mesg.status = HMESG_STATUS_FAIL;
                }
                if (mesg_send(STDIN_FILENO, &mesg) < 1)
                    goto error;
                break;

            case HMESG_FETCH:
                if (pfq_cap) {
                    /* Service fetch requests exclusively from the queue,
                     * if enabled.
                     */
                    mesg.status = HMESG_STATUS_OK;
                    mesg.index = plugin_len;
                    if (point_dequeue(&mesg) < 0)
                        goto error;
                }
                else {
                    /* Otherwise, fetch a fresh point, and report to the
                     * client directly.
                     */
                    if (strategy_fetch(&mesg) < 0)
                        goto error;

                    if (plugin_workflow(&mesg) < 0) {
                        if (mesg.status == HMESG_STATUS_FAIL)
                            goto error;
                    }
                }

                if (mesg_send(STDIN_FILENO, &mesg) < 1)
                    goto error;
                break;

            case HMESG_REPORT:
                mesg.index = plugin_len;
                if (plugin_workflow(&mesg) < 0) {
                    if (mesg.status == HMESG_STATUS_FAIL)
                        goto error;
                }

                /* Always return HMESG_STATUS_OK, unless a failure is
                 * detected.
                 */
                hpoint_fini(&mesg.data.report.cand);
                mesg.status = HMESG_STATUS_OK;
                if (mesg_send(STDIN_FILENO, &mesg) < 1)
                    goto error;

                if (pfq_cap && pfq_atomic && ++pfq_reported == pfq_cap) {
                    /* Atomic prefetch queue has been exhausted.  We
                     * are free to begin prefetching again.
                     */
                    pfq_requested = 0;
                    pfq_reported = 0;
                    pollstate = &polltime;
                }
                break;

            default:
                goto error;
            }
        }

        /* Prefetch another point, if there's room in the queue. */
        if (pfq_requested < pfq_cap) {
            hmesg_scrub(&mesg);
            mesg.dest = -1;
            mesg.type = HMESG_FETCH;
            mesg.status = HMESG_STATUS_REQ;
            mesg.index = 0;
            mesg.data.fetch.best.id = -1;

            if (strategy_fetch(&mesg) < 0)
                goto error;

            if (plugin_workflow(&mesg) < 0)
                if (mesg.status == HMESG_STATUS_FAIL)
                    goto error;

            if (mesg.status == HMESG_STATUS_OK)
                if (point_enqueue(&mesg) < 0)
                    goto error;

            ++pfq_requested;
        }

        /* Prefetch queue at capacity.  Stop polling. */
        if (pollstate && pfq_requested == pfq_cap)
            pollstate = NULL;
    }
    goto cleanup;

  error:
    mesg.status = HMESG_STATUS_FAIL;
    mesg_send(STDIN_FILENO, &mesg);
    retval = -1;

  cleanup:
    for (i = plugin_len - 1; i >= 0; --i) {
        if (plugin[i].fini)
            plugin[i].fini();
    }

    return retval;
}

int strategy_load(hmesg_t *mesg)
{
    const char *tmp;
    char *filename;
    void *lib;
    int (*init)(hmesg_t *);

    tmp = hcfg_get(sess->cfg, CFGKEY_SESSION_STRATEGY);
    if (!tmp)
        tmp = DEFAULT_STRATEGY;
    filename = sprintf_alloc("%s/libexec/%s",
                             hcfg_get(sess->cfg, CFGKEY_HARMONY_ROOT), tmp);
    lib = dlopen(filename, RTLD_LAZY | RTLD_GLOBAL);
    free(filename);
    if (!lib) {
        mesg->data.string = dlerror();
        return -1;
    }

    init = HACK_CAST_INT(dlsym(lib, "strategy_init"));
    strategy_fetch = HACK_CAST_INT(dlsym(lib, "strategy_fetch"));
    strategy_report = HACK_CAST_INT(dlsym(lib, "strategy_report"));
    strategy_best = (hpoint_t *) dlsym(lib, "strategy_best");

    if (!init) {
        mesg->data.string = "Strategy does not define strategy_init()";
        return -1;
    }

    if (!strategy_fetch) {
        mesg->data.string = "Strategy does not define strategy_fetch()";
        return -1;
    }

    if (!strategy_report) {
        mesg->data.string = "Strategy does not define strategy_report()";
        return -1;
    }

    if (!strategy_best) {
        mesg->data.string = "Strategy does not define strategy_best";
        return -1;
    }

    return init(mesg);
}

int plugin_list_load(hmesg_t *mesg, const char *list)
{
    const char *prefix, *end;
    char *fname;
    int fname_len, retval;
    void *lib;
    int (*init)(hmesg_t *);

    fname = NULL;
    fname_len = 0;
    retval = 0;
    while (list && *list) {
        if (plugin_len == plugin_cap) {
            if (array_grow(&plugin, &plugin_cap, sizeof(plugin_t)) < 0) {
                retval = -1;
                goto cleanup;
            }
        }
        end = strchr(list, SESSION_PLUGIN_SEP);
        if (!end)
            end = list + strlen(list);

        prefix = hcfg_get(sess->cfg, CFGKEY_HARMONY_ROOT);
        if (snprintf_grow(&fname, &fname_len, "%s/libexec/%.*s",
                          prefix, end - list, list) < 0)
        {
            retval = -1;
            goto cleanup;
        }

        lib = dlopen(fname, RTLD_LAZY);
        if (!lib) {
            hmesg_scrub(mesg);
            mesg->data.string = dlerror();
            retval = -1;
            goto cleanup;
        }

        prefix = (const char *) dlsym(lib, "harmony_plugin_name");
        if (!prefix) {
            hmesg_scrub(mesg);
            mesg->data.string = dlerror();
            retval = -1;
            goto cleanup;
        }

        plugin[plugin_len].name = prefix;
        if (snprintf_grow(&fname, &fname_len, "%s_init", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }

        init = HACK_CAST_INT(dlsym(lib, fname));
        if (init && init(mesg) < 0) {
            retval = -1;
            goto cleanup;
        }

        if (snprintf_grow(&fname, &fname_len, "%s_report", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }
        plugin[plugin_len].report = HACK_CAST_INT(dlsym(lib, fname));

        if (snprintf_grow(&fname, &fname_len, "%s_fetch", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }
        plugin[plugin_len].fetch = HACK_CAST_INT(dlsym(lib, fname));

        if (snprintf_grow(&fname, &fname_len, "%s_fini", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }
        plugin[plugin_len].fini = HACK_CAST_VOID(dlsym(lib, fname));

        ++plugin_len;
        list = *end ? end + 1 : NULL;
    }

  cleanup:
    free(fname);
    return retval;
}

int prefetch_init(hmesg_t *mesg)
{
    const char *cfgstr;
    int i;

    cfgstr = hcfg_get(sess->cfg, CFGKEY_PREFETCH_COUNT);
    if (cfgstr && strcasecmp(cfgstr, "auto") == 0) {
        cfgstr = hcfg_get(sess->cfg, CFGKEY_CLIENT_COUNT);
        if (cfgstr)
            pfq_cap = atoi(cfgstr);
        else
            pfq_cap = DEFAULT_CLIENT_COUNT;
    }
    else if (cfgstr) {
        pfq_cap = atoi(cfgstr);
    }
    else {
        pfq_cap = DEFAULT_PREFETCH_COUNT;
    }

    if (pfq_cap < 0) {
        mesg->data.string =
            "Invalid config value for " CFGKEY_PREFETCH_COUNT;
        return -1;
    }
    else if (pfq_cap) {
        pfqueue = (hpoint_t *) malloc(pfq_cap * sizeof(hpoint_t));
        if (!pfqueue)
            return -1;
        for (i = 0; i < pfq_cap; ++i)
            pfqueue[i] = HPOINT_INITIALIZER;
    }

    cfgstr = hcfg_get(sess->cfg, CFGKEY_PREFETCH_ATOMIC);
    if (cfgstr && strcmp(cfgstr, "1") == 0)
        pfq_atomic = 1;
    else
        pfq_atomic = DEFAULT_PREFETCH_ATOMIC;

    return 0;
}

int plugin_workflow(hmesg_t *mesg)
{
    if (mesg->type == HMESG_FETCH) {
        while (mesg->index < plugin_len) {
            if (plugin[mesg->index].fetch) {
                if (plugin[mesg->index].fetch(mesg) < 0)
                    return -1;
            }
            ++mesg->index;
        }
    }

    else if (mesg->type == HMESG_REPORT) {
        while (mesg->index > 0) {
            --mesg->index;
            if (plugin[mesg->index].report) {
                if (plugin[mesg->index].report(mesg) < 0)
                    return -1;
            }
        }
        if (strategy_report(mesg) < 0) {
            mesg->status = HMESG_STATUS_FAIL;
            return -1;
        }
    }

    else {
        mesg->status = HMESG_STATUS_FAIL;
        errno = EINVAL;
        return -1;
    }

    return 0;
}

int point_enqueue(hmesg_t *mesg)
{
    hpoint_t *tail, *cand;

    tail = &pfqueue[pfq_tail];
    cand = &mesg->data.fetch.cand;

    /* Stash this message in the prefetch buffer. */
    if (tail->id != -1) {
        fprintf(stderr, "Internal error: fetch buffer overflow.\n");
        return -1;
    }

    *tail = HPOINT_INITIALIZER;
    if (hpoint_copy(tail, cand) < 0) {
        perror("Internal error copying message to prefetch queue");
        return -1;
    }

    pfq_tail = (pfq_tail + 1) % pfq_cap;
    return 0;
}

int point_dequeue(hmesg_t *mesg)
{
    hpoint_t *head, *cand, *best;

    head = &pfqueue[pfq_head];
    cand = &mesg->data.fetch.cand;
    best = &mesg->data.fetch.best;

    if (head->id != -1) {
        *cand = HPOINT_INITIALIZER;
        if (hpoint_copy(cand, head) < 0)
            return -1;

        *best = HPOINT_INITIALIZER;
        if (best->id < strategy_best->id)
            if (hpoint_copy(best, strategy_best) < 0)
                return -1;

        hpoint_fini(head);
        head->id = -1;
        pfq_head = (pfq_head + 1) % pfq_cap;

        if (!pfq_atomic) {
            /* New queue slots available.  Polling may begin again. */
            --pfq_requested;
            pollstate = &polltime;
        }
        mesg->status = HMESG_STATUS_OK;
    }
    else {
        mesg->status = HMESG_STATUS_BUSY;
    }

    return 0;
}

/* ========================================================
 * Exported functions for 3rd party plugins and strategies.
 */

/* Allows 3rd party plugins to receive notification of a data event. */
int callback_register(int fd, int (*func)(hmesg_t *))
{
    struct stat sb;

    /* Sanity check input. */
    if (fstat(fd, &sb) < 0 || !S_ISSOCK(sb.st_mode)) {
        errno = ENOTSOCK;
        return -1;
    }

    if (cb_len >= cb_cap) {
        if (array_grow(&cb, &cb_cap, sizeof(callback_t)) < 0)
            return -1;
    }
    cb[cb_len].fd = fd;
    cb[cb_len].func = func;
    ++cb_len;

    FD_SET(fd, &fds);
    if (maxfd < fd)
        maxfd = fd;

    return 0;
}

/* Removes a file descriptor from the listen set. */
int callback_unregister(int fd)
{
    int i;

    for (i = 0; i < cb_len; ++i)
        if (cb[i].fd == fd)
            break;

    if (i == cb_len)
        return -1;

    FD_CLR(fd, &fds);
    while (i < cb_len) {
        cb[i].fd = cb[i+1].fd;
        cb[i].func = cb[i+1].func;
    }
    --cb_len;
    return 0;
}
