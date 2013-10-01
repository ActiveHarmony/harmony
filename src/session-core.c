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
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

/* Session structure exported to 3rd party plug-ins. */
hsession_t *sess;

/* Be sure all remaining global definitions are declared static to
 * reduce the number of symbols exported by --export-dynamic.
 */

/* --------------------------------
 * Callback registration variables.
 */
typedef struct callback {
    int fd;
    int index;
    cb_func_t func;
} callback_t;

static callback_t *cb = NULL;
static int cb_len = 0;
static int cb_cap = 0;

/* -------------------------
 * Plug-in system variables.
 */
static strategy_generate_t strategy_generate;
static strategy_rejected_t strategy_rejected;
static strategy_analyze_t  strategy_analyze;
static strategy_best_t     strategy_best;

static hook_init_t         strategy_init;
static hook_join_t         strategy_join;
static hook_getcfg_t       strategy_getcfg;
static hook_setcfg_t       strategy_setcfg;
static hook_fini_t         strategy_fini;

typedef struct layer {
    const char *name;

    layer_generate_t generate;
    layer_analyze_t  analyze;

    hook_init_t      init;
    hook_join_t      join;
    hook_getcfg_t    getcfg;
    hook_setcfg_t    setcfg;
    hook_fini_t      fini;

    int *wait_generate;
    int wait_generate_len;
    int wait_generate_cap;

    int *wait_analyze;
    int wait_analyze_len;
    int wait_analyze_cap;
} layer_t;

/* Stack of layer objects. */
static layer_t *lstack = NULL;
static int lstack_len = 0;
static int lstack_cap = 0;

/* ------------------------------
 * Forward function declarations.
 */
static int generate_trial(void);
static int plugin_workflow(int trial_idx);
static int workflow_transition(int trial_idx);
static int handle_callback(callback_t *cb);
static int handle_join(hmesg_t *mesg);
static int handle_getcfg(hmesg_t *mesg);
static int handle_setcfg(hmesg_t *mesg);
static int handle_fetch(hmesg_t *mesg);
static int handle_report(hmesg_t *mesg);
static int handle_reject(int trial_idx);
static int handle_wait(int trial_idx);
static int load_strategy(const char *file);
static int load_layers(const char *list);
static int extend_lists(int target_cap);
static void reverse_array(void *ptr, int head, int tail);

/* -------------------
 * Workflow variables.
 */
static const char *errmsg;
static int curr_layer = 0;
static hflow_t flow;

/* List of all points generated, but not yet returned to the strategy. */
static htrial_t *pending = NULL;
static int pending_cap = 0;
static int pending_len = 0;

/* List of all trials (point/performance pairs) waiting for client fetch. */
static int *ready = NULL;
static int ready_head = 0;
static int ready_tail = 0;
static int ready_cap = 0;

static int per_client = DEFAULT_PER_CLIENT;
static int num_clients = 0;

/* ----------------------------
 * Variables used for select().
 */
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
    const char *ptr;
    hmesg_t mesg = HMESG_INITIALIZER;
    hmesg_t session_mesg = HMESG_INITIALIZER;

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
    pollstate = &polltime;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    maxfd = STDIN_FILENO;

    if (array_grow(&cb, &cb_cap, sizeof(callback_t)) < 0)
        goto error;

    if (array_grow(&lstack, &lstack_cap, sizeof(layer_t)) < 0)
        goto error;

    /* Receive initial session message. */
    mesg.type = HMESG_SESSION;
    if (mesg_recv(STDIN_FILENO, &session_mesg) < 1) {
        mesg.data.string = "Socket or deserialization error";
        goto error;
    }

    if (session_mesg.type != HMESG_SESSION ||
        session_mesg.status != HMESG_STATUS_REQ)
    {
        mesg.data.string = "Invalid initial message";
        goto error;
    }

    /* Load and initialize the strategy code object. */
    sess = &session_mesg.data.session;

    ptr = hcfg_get(sess->cfg, CFGKEY_SESSION_STRATEGY);
    if (load_strategy(ptr) < 0)
        goto error;

    /* Load and initialize requested layer's. */
    ptr = hcfg_get(sess->cfg, CFGKEY_SESSION_LAYERS);
    if (ptr && load_layers(ptr) < 0)
        goto error;

    mesg.dest   = session_mesg.dest;
    mesg.type   = session_mesg.type;
    mesg.status = HMESG_STATUS_OK;
    mesg.src_id = session_mesg.src_id;
    if (mesg_send(STDIN_FILENO, &mesg) < 1) {
        errmsg = session_mesg.data.string;
        goto error;
    }

    ptr = hcfg_get(sess->cfg, CFGKEY_PER_CLIENT_STORAGE);
    if (ptr) {
        per_client = atoi(ptr);
        if (per_client < 1) {
            errmsg = "Invalid config value for " CFGKEY_PER_CLIENT_STORAGE ".";
            goto error;
        }
    }

    retval = 0;
    ptr = hcfg_get(sess->cfg, CFGKEY_CLIENT_COUNT);
    if (ptr) {
        retval = atoi(ptr);
        if (retval < 1) {
            errmsg = "Invalid config value for " CFGKEY_CLIENT_COUNT ".";
            goto error;
        }
        retval *= per_client;
    }
    if (extend_lists(retval) != 0)
        goto error;

    while (1) {
        flow.status = HFLOW_ACCEPT;
        flow.point  = HPOINT_INITIALIZER;

        ready_fds = fds;
        retval = select(maxfd + 1, &ready_fds, NULL, NULL, pollstate);
        if (retval < 0)
            goto error;

        /* Launch callbacks, if needed. */
        for (i = 0; i < cb_len; ++i) {
            if (FD_ISSET(cb[i].fd, &ready_fds))
                handle_callback(&cb[i]);
        }

        /* Handle hmesg_t, if needed. */
        if (FD_ISSET(STDIN_FILENO, &ready_fds)) {
            if (mesg_recv(STDIN_FILENO, &mesg) < 1)
                goto error;

            hcfg_set(sess->cfg, CFGKEY_CURRENT_CLIENT, mesg.src_id);
            switch (mesg.type) {
            case HMESG_JOIN:   retval = handle_join(&mesg); break;
            case HMESG_GETCFG: retval = handle_getcfg(&mesg); break;
            case HMESG_SETCFG: retval = handle_setcfg(&mesg); break;
            case HMESG_FETCH:  retval = handle_fetch(&mesg); break;
            case HMESG_REPORT: retval = handle_report(&mesg); break;
            default:
                errmsg = "Internal error: Unknown message type.";
                goto error;
            }
            if (retval != 0)
                goto error;

            hcfg_set(sess->cfg, CFGKEY_CURRENT_CLIENT, NULL);
            mesg.src_id = NULL;
            if (mesg_send(STDIN_FILENO, &mesg) < 1)
                goto error;
        }

        /* Generate another point, if there's room in the queue. */
        while (pollstate != NULL && pending_len < pending_cap) {
            generate_trial();
        }
    }
    goto cleanup;

  error:
    mesg.status = HMESG_STATUS_FAIL;
    mesg.data.string = errmsg;
    mesg_send(STDIN_FILENO, &mesg);

  cleanup:
    for (i = lstack_len - 1; i >= 0; --i) {
        if (lstack[i].fini)
            lstack[i].fini();
    }

    hmesg_fini(&session_mesg);
    hmesg_fini(&mesg);
    return retval;
}

int generate_trial(void)
{
    int idx;
    hpoint_t *point;

    /* Find a free point. */
    for (idx = 0; idx < pending_cap; ++idx) {
        if (pending[idx].point.id == -1) {
            point = (hpoint_t *) &pending[idx].point;
            break;
        }
    }
    if (idx == pending_cap) {
        errmsg = "Internal error: Point generation overflow.";
        return -1;
    }

    /* Call strategy generation routine. */
    if (strategy_generate(&flow, point) != 0)
        return -1;

    if (flow.status == HFLOW_WAIT) {
        /* Strategy requests that we pause point generation. */
        pollstate = NULL;
        return 0;
    }

    /* Begin generation workflow for new point. */
    ++pending_len;
    pending[idx].perf = INFINITY;
    curr_layer = 1;

    return plugin_workflow(idx);
}

int plugin_workflow(int trial_idx)
{
    htrial_t *trial = &pending[trial_idx];

    while (curr_layer != 0 && curr_layer <= lstack_len)
    {
        int stack_idx = abs(curr_layer) - 1;
        int retval;

        flow.status = HFLOW_ACCEPT;
        if (curr_layer < 0) {
            /* Analyze workflow. */
            if (lstack[stack_idx].analyze) {
                if (lstack[stack_idx].analyze(&flow, trial) != 0)
                    return -1;
            }
        }
        else {
            /* Generate workflow. */
            if (lstack[stack_idx].generate) {
                if (lstack[stack_idx].generate(&flow, trial) != 0)
                    return -1;
            }
        }

        retval = workflow_transition(trial_idx);
        if (retval < 0) return -1;
        if (retval > 0) return  0;
    }

    if (curr_layer == 0) {
        hpoint_t *point = (hpoint_t *) &trial->point;

        /* Completed analysis layers.  Send trial to strategy. */
        if (strategy_analyze(trial) != 0)
            return -1;

        /* Remove point data from pending list. */
        hpoint_fini(point);
        *point = HPOINT_INITIALIZER;
        --pending_len;

        /* Point generation attempts may begin again. */
        pollstate = &polltime;
    }
    else if (curr_layer > lstack_len) {
        /* Completed generation layers.  Enqueue trial in ready queue. */
        if (ready[ready_tail] != -1) {
            errmsg = "Internal error: Ready queue overflow.";
            return -1;
        }

        ready[ready_tail] = trial_idx;
        ready_tail = (ready_tail + 1) % ready_cap;
    }
    else {
        errmsg = "Internal error: Invalid current plug-in layer.";
        return -1;
    }

    return 0;
}

/* Layer state machine transitions. */
int workflow_transition(int trial_idx)
{
    switch (flow.status) {
    case HFLOW_ACCEPT:
        curr_layer += 1;
        break;

    case HFLOW_WAIT:
        if (handle_wait(trial_idx) != 0) {
            return -1;
        }
        return 1;

    case HFLOW_RETURN:
    case HFLOW_RETRY:
        curr_layer = -curr_layer;
        break;

    case HFLOW_REJECT:
        if (handle_reject(trial_idx) != 0)
            return -1;

        if (flow.status == HFLOW_WAIT)
            return 1;

        curr_layer = 1;
        break;

    default:
        errmsg = "Internal error: Invalid workflow status.";
        return -1;
    }
    return 0;
}

int handle_reject(int trial_idx)
{
    htrial_t *trial = &pending[trial_idx];

    if (curr_layer < 0) {
        errmsg = "Internal error: REJECT invalid for analysis workflow.";
        return -1;
    }

    /* Regenerate this rejected point. */
    if (strategy_rejected(&flow, (hpoint_t *) &trial->point) != 0)
        return -1;

    if (flow.status == HFLOW_WAIT)
        pollstate = NULL;

    return 0;
}

int handle_wait(int trial_idx)
{
    int idx = abs(curr_layer) - 1;
    int **list;
    int *len, *cap;

    if (curr_layer < 0) {
        list = &lstack[idx].wait_analyze;
        len  = &lstack[idx].wait_analyze_len;
        cap  = &lstack[idx].wait_analyze_cap;
    }
    else {
        list = &lstack[idx].wait_generate;
        len  = &lstack[idx].wait_generate_len;
        cap  = &lstack[idx].wait_generate_cap;
    }

    if (*len == *cap) {
        int i;

        array_grow(list, cap, sizeof(int));
        for (i = *len; i < *cap; ++i)
            (*list)[i] = -1;
    }

    if ((*list)[*len] != -1) {
        errmsg = "Internal error: Could not append to wait list.";
        return -1;
    }

    (*list)[*len] = trial_idx;
    ++(*len);
    return 0;
}

int handle_callback(callback_t *cb)
{
    htrial_t **trial_list;
    int *list, *len, trial_idx;
    int i, idx, retval;

    curr_layer = cb->index;
    idx = abs(curr_layer) - 1;

    /* The idx variable represents layer plugin index for now. */
    if (curr_layer < 0) {
        list = lstack[idx].wait_analyze;
        len = &lstack[idx].wait_analyze_len;
    }
    else {
        list = lstack[idx].wait_generate;
        len = &lstack[idx].wait_generate_len;
    }

    if (*len < 1) {
        errmsg = "Internal error: Callback on layer with empty waitlist.";
        return -1;
    }

    /* Prepare a list of htrial_t pointers. */
    trial_list = (htrial_t **) malloc(*len * sizeof(htrial_t *));
    for (i = 0; i < *len; ++i)
        trial_list[i] = &pending[ list[i] ];

    /* Reusing idx to represent waitlist index.  (Shame on me.) */
    idx = cb->func(cb->fd, &flow, *len, trial_list);
    free(trial_list);

    trial_idx = list[idx];
    retval = workflow_transition(trial_idx);
    if (retval < 0) return -1;
    if (retval > 0) return  0;

    --(*len);
    list[ idx] = list[*len];
    list[*len] = -1;
    return plugin_workflow(trial_idx);
}

int handle_join(hmesg_t *mesg)
{
    int i;

    /* Verify that client signature matches current session. */
    if (!hsignature_match(&mesg->data.join, &sess->sig)) {
        errmsg = "Incompatible join signature.";
        return -1;
    }

    if (hsignature_copy(&mesg->data.join, &sess->sig) < 0) {
        errmsg = "Internal error: Could not copy signature.";
        return -1;
    }

    /* Grow the pending and ready queues. */
    ++num_clients;
    if (extend_lists(num_clients * per_client) != 0)
        return -1;

    /* Launch all join hooks defined in the plug-in stack. */
    if (strategy_join && strategy_join(mesg->src_id) != 0)
        return -1;

    for (i = 0; i < lstack_len; ++i) {
        if (lstack[i].join && lstack[i].join(mesg->src_id) != 0)
            return -1;
    }

    mesg->status = HMESG_STATUS_OK;
    return 0;
}

int handle_getcfg(hmesg_t *mesg)
{
    int i;
    const char *key;

    key = mesg->data.string;

    /* Launch all getcfg hooks defined in the plug-in stack. */
    if (strategy_getcfg && strategy_getcfg(key) != 0)
        return -1;

    for (i = 0; i < lstack_len; ++i) {
        if (lstack[i].getcfg && lstack[i].getcfg(key) != 0)
            return -1;
    }

    /* Prepare getcfg response message for client. */
    mesg->data.string = hcfg_get(sess->cfg, key);
    mesg->status = HMESG_STATUS_OK;
    return 0;
}

int handle_setcfg(hmesg_t *mesg)
{
    static char *buf = NULL;
    static int buflen = 0;

    int i;
    char *key, *val;
    const char *oldval;

    hcfg_parse((char *)mesg->data.string, &key, &val);
    if (!key) {
        errmsg = strerror(EINVAL);
        return -1;
    }

    /* Launch all setcfg hooks defined in the plug-in stack. */
    if (strategy_setcfg && strategy_setcfg(key, val) != 0)
        return -1;

    for (i = 0; i < lstack_len; ++i) {
        if (lstack[i].setcfg && lstack[i].setcfg(key, val) != 0)
            return -1;
    }

    /* Store the original value, possibly allocating memory for it. */
    oldval = hcfg_get(sess->cfg, key);
    if (oldval) {
        snprintf_grow(&buf, &buflen, "%s", oldval);
        oldval = buf;
    }

    /* Finally, update the configuration system. */
    if (hcfg_set(sess->cfg, key, val) != 0) {
        errmsg = "Internal error: Could not modify configuration.";
        return -1;
    }

    /* Prepare setcfg response message for client. */
    mesg->data.string = oldval;
    mesg->status = HMESG_STATUS_OK;
    return 0;
}

int handle_fetch(hmesg_t *mesg)
{
    int idx = ready[ready_head];
    hpoint_t *cand = &mesg->data.fetch.cand;
    hpoint_t *best = &mesg->data.fetch.best;
    htrial_t *next;

    /* If ready queue is empty, inform client that we're busy. */
    if (idx == -1) {
        mesg->status = HMESG_STATUS_BUSY;
        return 0;
    }
    next = &pending[idx];

    /* Otherwise, prepare a response to the client. */
    *cand = HPOINT_INITIALIZER;
    if (hpoint_copy(cand, &next->point) != 0) {
        errmsg = "Internal error: Could not copy candidate point data.";
        return -1;
    }

    *best = HPOINT_INITIALIZER;
    if (strategy_best(best) != 0)
        return -1;

    /* Remove the first point from the ready queue. */
    ready[ready_head] = -1;
    ready_head = (ready_head + 1) % ready_cap;

    mesg->status = HMESG_STATUS_OK;
    return 0;
}

int handle_report(hmesg_t *mesg)
{
    int idx;

    /* Find the associated trial in the pending list. */
    for (idx = 0; idx < pending_cap; ++idx) {
        if (pending[idx].point.id == mesg->data.report.cand.id)
            break;
    }
    if (idx == pending_cap) {
        errmsg = "Rouge point support not yet implemented.";
        return -1;
    }

    /* Update performance in our local records. */
    pending[idx].perf = mesg->data.report.perf;

    /* Begin the workflow at the outermost analysis layer. */
    curr_layer = -lstack_len;
    if (plugin_workflow(idx) != 0)
        return -1;

    hmesg_scrub(mesg);
    mesg->status = HMESG_STATUS_OK;
    return 0;
}

/* ISO C forbids conversion of object pointer to function pointer,
 * making it difficult to use dlsym() for functions.  We get around
 * this by first casting to a word-length integer.  (ILP32/LP64
 * compilers assumed).
 */
#define dlfptr(x, y) ((void (*)(void))(long)(dlsym((x), (y))))

int load_strategy(const char *file)
{
    const char *root;
    char *path;
    void *lib;

    if (!file)
        file = DEFAULT_STRATEGY;

    root = hcfg_get(sess->cfg, CFGKEY_HARMONY_HOME);
    path = sprintf_alloc("%s/libexec/%s", root, file);

    lib = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    free(path);
    if (!lib) {
        errmsg = dlerror();
        return -1;
    }

    strategy_generate = (strategy_generate_t) dlfptr(lib, "strategy_generate");
    strategy_rejected = (strategy_rejected_t) dlfptr(lib, "strategy_rejected");
    strategy_analyze  =  (strategy_analyze_t) dlfptr(lib, "strategy_analyze");
    strategy_best     =     (strategy_best_t) dlfptr(lib, "strategy_best");

    strategy_init     =         (hook_init_t) dlfptr(lib, "strategy_init");
    strategy_join     =         (hook_join_t) dlfptr(lib, "strategy_join");
    strategy_getcfg   =       (hook_getcfg_t) dlfptr(lib, "strategy_getcfg");
    strategy_setcfg   =       (hook_setcfg_t) dlfptr(lib, "strategy_setcfg");
    strategy_fini     =         (hook_fini_t) dlfptr(lib, "strategy_fini");

    if (!strategy_generate) {
        errmsg = "Strategy does not define strategy_generate()";
        return -1;
    }

    if (!strategy_rejected) {
        errmsg = "Strategy does not define strategy_rejected()";
        return -1;
    }

    if (!strategy_analyze) {
        errmsg = "Strategy does not define strategy_analyze()";
        return -1;
    }

    if (!strategy_best) {
        errmsg = "Strategy does not define strategy_best()";
        return -1;
    }

    if (strategy_init)
        return strategy_init(&sess->sig);

    return 0;
}

int load_layers(const char *list)
{
    const char *prefix, *end;
    char *path;
    int path_len, retval;
    void *lib;

    path = NULL;
    path_len = 0;
    retval = 0;
    while (list && *list) {
        if (lstack_len == lstack_cap) {
            if (array_grow(&lstack, &lstack_cap, sizeof(layer_t)) < 0) {
                retval = -1;
                goto cleanup;
            }
        }
        end = strchr(list, SESSION_LAYER_SEP);
        if (!end)
            end = list + strlen(list);

        prefix = hcfg_get(sess->cfg, CFGKEY_HARMONY_HOME);
        if (snprintf_grow(&path, &path_len, "%s/libexec/%.*s",
                          prefix, end - list, list) < 0)
        {
            retval = -1;
            goto cleanup;
        }

        lib = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
        if (!lib) {
            errmsg = dlerror();
            retval = -1;
            goto cleanup;
        }

        prefix = (const char *) dlsym(lib, "harmony_layer_name");
        if (!prefix) {
            errmsg = dlerror();
            retval = -1;
            goto cleanup;
        }
        lstack[lstack_len].name = prefix;

        if (snprintf_grow(&path, &path_len, "%s_init", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }
        lstack[lstack_len].init = (hook_init_t) dlfptr(lib, path);

        if (snprintf_grow(&path, &path_len, "%s_join", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }
        lstack[lstack_len].join = (hook_join_t) dlfptr(lib, path);

        if (snprintf_grow(&path, &path_len, "%s_generate", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }
        lstack[lstack_len].generate = (layer_generate_t) dlfptr(lib, path);

        if (snprintf_grow(&path, &path_len, "%s_analyze", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }
        lstack[lstack_len].analyze = (layer_analyze_t) dlfptr(lib, path);

        if (snprintf_grow(&path, &path_len, "%s_getcfg", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }
        lstack[lstack_len].getcfg = (hook_getcfg_t) dlfptr(lib, path);

        if (snprintf_grow(&path, &path_len, "%s_setcfg", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }
        lstack[lstack_len].setcfg = (hook_setcfg_t) dlfptr(lib, path);

        if (snprintf_grow(&path, &path_len, "%s_fini", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }
        lstack[lstack_len].fini = (hook_fini_t) dlfptr(lib, path);

        if (lstack[lstack_len].init) {
            curr_layer = lstack_len + 1;
            if (lstack[lstack_len].init(&sess->sig) < 0) {
                retval = -1;
                goto cleanup;
            }
        }
        ++lstack_len;

        if (*end)
            list = end + 1;
        else
            list = NULL;
    }

  cleanup:
    free(path);
    return retval;
}

int extend_lists(int target_cap)
{
    int orig_cap = pending_cap;
    int i;

    if (pending_cap >= target_cap && pending_cap != 0)
        return 0;

    if (ready && ready_tail <= ready_head && ready[ready_head] != -1) {
        i = ready_cap - ready_head;

        /* Shift ready array to align head with array index 0. */
        reverse_array(ready, 0, ready_cap);
        reverse_array(ready, 0, i);
        reverse_array(ready, i, ready_cap);

        ready_head = 0;
        ready_tail = ready_cap - ready_tail + ready_head;
    }

    ready_cap = target_cap;
    if (array_grow(&ready, &ready_cap, sizeof(htrial_t *)) != 0) {
        errmsg = "Internal error: Could not extend ready array.";
        return -1;
    }

    pending_cap = target_cap;
    if (array_grow(&pending, &pending_cap, sizeof(htrial_t)) != 0) {
        errmsg = "Internal error: Could not extend pending array.";
        return -1;
    }

    for (i = orig_cap; i < pending_cap; ++i) {
        hpoint_t *point = (hpoint_t *) &pending[i].point;
        *point = HPOINT_INITIALIZER;
        ready[i] = -1;
    }
    return 0;
}

void reverse_array(void *ptr, int head, int tail)
{
    unsigned long *arr = (unsigned long *) ptr;

    while (head < --tail) {
        /* Swap head and tail entries. */
        arr[head] ^= arr[tail];
        arr[tail] ^= arr[head];
        arr[head] ^= arr[tail];
        ++head;
    }
}

/* ========================================================
 * Exported functions for pluggable modules.
 */

int callback_generate(int fd, cb_func_t func)
{
    if (cb_len >= cb_cap) {
        if (array_grow(&cb, &cb_cap, sizeof(callback_t)) < 0)
            return -1;
    }
    cb[cb_len].fd = fd;
    cb[cb_len].index = curr_layer;
    cb[cb_len].func = func;
    ++cb_len;

    FD_SET(fd, &fds);
    if (maxfd < fd)
        maxfd = fd;

    return 0;
}

int callback_analyze(int fd, cb_func_t func)
{
    if (cb_len >= cb_cap) {
        if (array_grow(&cb, &cb_cap, sizeof(callback_t)) < 0)
            return -1;
    }
    cb[cb_len].fd = fd;
    cb[cb_len].index = -curr_layer;
    cb[cb_len].func = func;
    ++cb_len;

    FD_SET(fd, &fds);
    if (maxfd < fd)
        maxfd = fd;

    return 0;
}

const char *session_getcfg(const char *key)
{
    return hcfg_get(sess->cfg, key);
}

int session_setcfg(const char *key, const char *val)
{
    return hcfg_set(sess->cfg, key, val);
}

void session_error(const char *msg)
{
    errmsg = msg;
}
