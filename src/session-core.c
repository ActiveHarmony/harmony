/*
 * Copyright 2003-2015 Jeffrey K. Hollingsworth
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
#define _XOPEN_SOURCE 600 // Needed for srand48() and S_ISSOCK.

#include "session-core.h"
#include "hspace.h"
#include "hcfg.h"
#include "hmesg.h"
#include "hutil.h"
#include "hsockutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <dlfcn.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

/* --------------------------------
 * Session configuration variables.
 */
hspace_t session_space = HSPACE_INITIALIZER;
hcfg_t cfg = HCFG_INITIALIZER;
const hcfg_t* session_cfg = &cfg;

int perf_count;
int per_client;
int num_clients;

/* --------------------------------
 * Callback registration variables.
 */
typedef struct callback {
    int fd;
    int index;
    cb_func_t func;
} callback_t;

callback_t* cbs = NULL;
int cbs_len = 0;
int cbs_cap = 0;

/* -------------------------
 * Plug-in system variables.
 */
strategy_generate_t strategy_generate;
strategy_rejected_t strategy_rejected;
strategy_analyze_t  strategy_analyze;
strategy_best_t     strategy_best;

hook_init_t         strategy_init;
hook_join_t         strategy_join;
hook_setcfg_t       strategy_setcfg;
hook_fini_t         strategy_fini;

typedef struct layer {
    const char* name;

    layer_generate_t generate;
    layer_analyze_t  analyze;

    hook_init_t      init;
    hook_join_t      join;
    hook_setcfg_t    setcfg;
    hook_fini_t      fini;

    int* wait_generate;
    int wait_generate_len;
    int wait_generate_cap;

    int* wait_analyze;
    int wait_analyze_len;
    int wait_analyze_cap;
} layer_t;

/* Stack of layer objects. */
layer_t* lstack = NULL;
int lstack_len = 0;
int lstack_cap = 0;

/* ------------------------------
 * Forward function declarations.
 */
int init_session(void);
int generate_trial(void);
int plugin_workflow(int trial_idx);
int workflow_transition(int trial_idx);
int handle_callback(callback_t* cb);
int handle_join(hmesg_t* mesg);
int handle_getcfg(hmesg_t* mesg);
int handle_setcfg(hmesg_t* mesg);
int handle_best(hmesg_t* mesg);
int handle_fetch(hmesg_t* mesg);
int handle_report(hmesg_t* mesg);
int handle_reject(int trial_idx);
int handle_restart(hmesg_t* mesg);
int handle_wait(int trial_idx);
int update_state(hmesg_t* mesg);
int load_strategy(const char* file);
int load_layers(const char* list);
int extend_lists(int target_cap);
void reverse_array(void* ptr, int head, int tail);

/* -------------------
 * Workflow variables.
 */
const char* errmsg;
int curr_layer = 0;
hflow_t flow;
int paused_id;

/* List of all points generated, but not yet returned to the strategy. */
htrial_t* pending = NULL;
int pending_cap = 0;
int pending_len = 0;

/* List of all trials (point/performance pairs) waiting for client fetch. */
int* ready = NULL;
int ready_head = 0;
int ready_tail = 0;
int ready_cap = 0;

/* ----------------------------
 * Variables used for select().
 */
struct timeval  polltime;
struct timeval* pollstate;
fd_set fds;
int maxfd;

/* -------------------------------------------
 * Static buffers used for dynamic allocation.
 */
static char* cfg_buf = NULL;
static int cfg_len = 0;

/* =================================
 * Core session routines begin here.
 */
int main(int argc, char* argv[])
{
    struct stat sb;
    int i, retval;
    fd_set ready_fds;
    hmesg_t mesg = HMESG_INITIALIZER;

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

    /* Receive the initial session message. */
    mesg.type = HMESG_SESSION;
    printf("Receiving initial session message on fd %d\n", STDIN_FILENO);
    if (mesg_recv(STDIN_FILENO, &mesg) < 1) {
        mesg.data.string = "Socket or deserialization error";
        goto error;
    }

    if (mesg.type != HMESG_SESSION || mesg.status != HMESG_STATUS_REQ) {
        mesg.data.string = "Invalid initial message";
        goto error;
    }

    /* Initialize the session. */
    if (hspace_copy(&session_space, &mesg.state.space) != 0) {
        mesg.data.string = "Could not copy session data";
        goto error;
    }

    if (hcfg_copy(&cfg, &mesg.data.cfg) != 0) {
        mesg.data.string = "Could not copy configuration environment";
        goto error;
    }

    // Overwrite CFGKEY_HARMONY_HOME, which was passed to session as argv[1].
    if (hcfg_set(&cfg, CFGKEY_HARMONY_HOME, argv[1]) != 0) {
        mesg.data.string = "Could not set " CFGKEY_HARMONY_HOME " for session";
        goto error;
    }

    if (init_session() != 0)
        goto error;

    /* Send the initial session message acknowledgment. */
    mesg.status = HMESG_STATUS_OK;
    if (mesg_send(STDIN_FILENO, &mesg) < 1) {
        errmsg = mesg.data.string;
        goto error;
    }

    while (1) {
        flow.status = HFLOW_ACCEPT;
        flow.point  = hpoint_zero;

        ready_fds = fds;
        retval = select(maxfd + 1, &ready_fds, NULL, NULL, (pending_len == pending_cap)?NULL:pollstate);
        if (retval < 0)
            goto error;

        /* Launch callbacks, if needed. */
        for (i = 0; i < cbs_len; ++i) {
            if (FD_ISSET(cbs[i].fd, &ready_fds))
                handle_callback(&cbs[i]);
        }

        /* Handle hmesg_t, if needed. */
        if (FD_ISSET(STDIN_FILENO, &ready_fds)) {
            retval = mesg_recv(STDIN_FILENO, &mesg);
            if (retval == 0) goto cleanup;
            if (retval <  0) goto error;

            hcfg_set(&cfg, CFGKEY_CURRENT_CLIENT, mesg.state.client);
            switch (mesg.type) {
            case HMESG_JOIN:    retval = handle_join(&mesg); break;
            case HMESG_GETCFG:  retval = handle_getcfg(&mesg); break;
            case HMESG_SETCFG:  retval = handle_setcfg(&mesg); break;
            case HMESG_BEST:    retval = handle_best(&mesg); break;
            case HMESG_FETCH:   retval = handle_fetch(&mesg); break;
            case HMESG_REPORT:  retval = handle_report(&mesg); break;
            case HMESG_RESTART: retval = handle_restart(&mesg); break;

            default:
                errmsg = "Internal error: Unknown message type.";
                goto error;
            }
            if (retval != 0)
                goto error;

            if (update_state(&mesg) != 0) {
                errmsg = "Could not update session state in message";
                goto error;
            }
            hcfg_set(&cfg, CFGKEY_CURRENT_CLIENT, NULL);

            if (mesg_send(STDIN_FILENO, &mesg) < 1)
                goto error;
        }

        /* Generate another point, if there's room in the queue. */
        while (pollstate != NULL && pending_len < pending_cap) {
            generate_trial();
        }
    }

  error:
    mesg.status = HMESG_STATUS_FAIL;
    mesg.data.string = errmsg;
    if (mesg_send(STDIN_FILENO, &mesg) < 1) {
        fprintf(stderr, "%s: Error sending error message: %s\n",
                argv[0], mesg.data.string);
    }
    retval = -1;

  cleanup:
    for (i = lstack_len - 1; i >= 0; --i) {
        if (lstack[i].fini)
            lstack[i].fini();
    }
    hmesg_fini(&mesg);
    hcfg_fini(&cfg);
    hspace_fini(&session_space);
    free(cfg_buf);

    return retval;
}

int init_session(void)
{
    const char* ptr;
    long seed;
    int clients;

    /* Before anything else, control the random seeds. */
    seed = hcfg_int(&cfg, CFGKEY_RANDOM_SEED);
    if (seed < 0)
        seed = time(NULL);
    srand((int) seed);
    srand48(seed);

    /* Initialize global data structures. */
    pollstate = &polltime;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    maxfd = STDIN_FILENO;

    if (array_grow(&cbs, &cbs_cap, sizeof(callback_t)) != 0) {
        errmsg = "Error allocating callback vector.";
        return -1;
    }

    if (array_grow(&lstack, &lstack_cap, sizeof(layer_t)) != 0) {
        errmsg = "Error allocating processing layer stack.";
        return -1;
    }

    perf_count = hcfg_int(&cfg, CFGKEY_PERF_COUNT);
    if (perf_count < 1) {
        errmsg = "Invalid " CFGKEY_PERF_COUNT " configuration key value";
        return -1;
    }

    per_client = hcfg_int(&cfg, CFGKEY_GEN_COUNT);
    if (per_client < 1) {
        errmsg = "Invalid " CFGKEY_GEN_COUNT " configuration key value";
        return -1;
    }

    clients = hcfg_int(&cfg, CFGKEY_CLIENT_COUNT);
    if (clients < 1) {
        errmsg = "Invalid " CFGKEY_CLIENT_COUNT " configuration key value";
        return -1;
    }

    /* Load and initialize the strategy code object. */
    ptr = hcfg_get(&cfg, CFGKEY_STRATEGY);
    if (!ptr) {
        if (clients > 1)
            ptr = "pro.so";
        else
            ptr = "nm.so";
    }

    if (load_strategy(ptr) != 0)
        return -1;

    /* Load and initialize requested layer's. */
    ptr = hcfg_get(&cfg, CFGKEY_LAYERS);
    if (ptr && load_layers(ptr) != 0)
        return -1;

    if (extend_lists(clients * per_client) != 0)
        return -1;

    return 0;
}

int generate_trial(void)
{
    int idx;
    htrial_t* trial = NULL;

    /* Find a free point. */
    for (idx = 0; idx < pending_cap; ++idx) {
        trial = &pending[idx];
        if (!trial->point.id)
            break;
    }
    if (idx == pending_cap) {
        errmsg = "Internal error: Point generation overflow.";
        return -1;
    }

    /* Reset the performance for this trial. */
    hperf_reset(&trial->perf);

    /* Call strategy generation routine. */
    if (strategy_generate(&flow, (hpoint_t*)&trial->point) != 0)
        return -1;

    if (flow.status == HFLOW_WAIT) {
        /* Strategy requests that we pause point generation. */
        pollstate = NULL;
        return 0;
    }
    ++pending_len;

    /* Begin generation workflow for new point. */
    curr_layer = 1;
    return plugin_workflow(idx);
}

int plugin_workflow(int trial_idx)
{
    htrial_t* trial = &pending[trial_idx];

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
        /* Completed analysis layers.  Send trial to strategy. */
        if (strategy_analyze(trial) != 0)
            return -1;

        /* Remove point data from pending list. */
        ((hpoint_t*)&trial->point)->id = 0;
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
    htrial_t* trial = &pending[trial_idx];

    if (curr_layer < 0) {
        errmsg = "Internal error: REJECT invalid for analysis workflow.";
        return -1;
    }

    /* Regenerate this rejected point. */
    if (strategy_rejected(&flow, (hpoint_t*) &trial->point) != 0)
        return -1;

    if (flow.status == HFLOW_WAIT)
        pollstate = NULL;

    return 0;
}

int handle_wait(int trial_idx)
{
    int   idx = abs(curr_layer) - 1;
    int** list;
    int*  len;
    int*  cap;

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

int handle_callback(callback_t* cb)
{
    htrial_t** trial_list;
    int* list;
    int* len;
    int  i, trial_idx, idx, retval;

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
    trial_list = malloc(*len * sizeof(htrial_t*));
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

int handle_join(hmesg_t* mesg)
{
    int i;

    if (hspace_copy(&mesg->state.space, &session_space) < 0) {
        errmsg = "Could not copy search space to message";
        return -1;
    }

    /* Grow the pending and ready queues. */
    ++num_clients;
    if (extend_lists(num_clients * per_client) != 0)
        return -1;

    /* Launch all join hooks defined in the plug-in stack. */
    if (strategy_join && strategy_join(mesg->state.client) != 0)
        return -1;

    for (i = 0; i < lstack_len; ++i) {
        if (lstack[i].join && lstack[i].join(mesg->state.client) != 0)
            return -1;
    }

    mesg->status = HMESG_STATUS_OK;
    return 0;
}

int handle_getcfg(hmesg_t* mesg)
{
    /* Prepare getcfg response message for client. */
    const char* cfgval = hcfg_get(&cfg, mesg->data.string);
    if (cfgval == NULL)
        cfgval = "";

    snprintf_grow(&cfg_buf, &cfg_len, "%s=%s", mesg->data.string, cfgval);
    mesg->data.string = cfg_buf;
    mesg->status = HMESG_STATUS_OK;
    return 0;
}

int handle_setcfg(hmesg_t* mesg)
{
    char* sep = (char*) strchr(mesg->data.string, '=');
    const char* oldval;

    if (!sep) {
        errmsg = strerror(EINVAL);
        return -1;
    }
    *sep = '\0';

    /* Store the original value, possibly allocating memory for it. */
    oldval = hcfg_get(&cfg, mesg->data.string);
    if (oldval) {
        snprintf_grow(&cfg_buf, &cfg_len, "%s", oldval);
        oldval = cfg_buf;
    }

    if (session_setcfg(mesg->data.string, sep + 1) != 0) {
        errmsg = "Error setting session configuration variable";
        return -1;
    }

    /* Prepare setcfg response message for client. */
    mesg->data.string = oldval;
    mesg->status = HMESG_STATUS_OK;
    return 0;
}

int handle_best(hmesg_t* mesg)
{
    mesg->status = HMESG_STATUS_OK;
    return 0;
}

int handle_fetch(hmesg_t* mesg)
{
    int idx = ready[ready_head], paused;
    htrial_t* next;

    /* Check if the session is paused. */
    paused = hcfg_bool(&cfg, CFGKEY_PAUSED);

    if (!paused && idx >= 0) {
        /* Send the next point on the ready queue. */
        next = &pending[idx];

        if (hpoint_copy(&mesg->data.point, &next->point) != 0) {
            errmsg = "Internal error: Could not copy candidate point data.";
            return -1;
        }

        /* Remove the first point from the ready queue. */
        ready[ready_head] = -1;
        ready_head = (ready_head + 1) % ready_cap;

        mesg->status = HMESG_STATUS_OK;
    }
    else {
        /* Ready queue is empty, or session is paused.
         * Send the best known point. */
        if (strategy_best(&mesg->state.best) != 0)
            return -1;

        paused_id = mesg->data.point.id;
        mesg->status = HMESG_STATUS_BUSY;
    }
    return 0;
}

int handle_report(hmesg_t* mesg)
{
    int idx;
    htrial_t* trial = NULL;

    /* Find the associated trial in the pending list. */
    for (idx = 0; idx < pending_cap; ++idx) {
        trial = &pending[idx];
        if (trial->point.id == mesg->data.point.id)
            break;
    }
    if (idx == pending_cap) {
        if (mesg->data.point.id == paused_id) {
            hperf_fini(&mesg->data.perf);
            mesg->status = HMESG_STATUS_OK;
            return 0;
        }
        else {
            errmsg = "Rouge point support not yet implemented.";
            return -1;
        }
    }
    paused_id = 0;

    /* Update performance in our local records. */
    hperf_copy(&trial->perf, &mesg->data.perf);

    /* Begin the workflow at the outermost analysis layer. */
    curr_layer = -lstack_len;
    if (plugin_workflow(idx) != 0)
        return -1;

    hperf_fini(&mesg->data.perf);
    mesg->status = HMESG_STATUS_OK;
    return 0;
}

int handle_restart(hmesg_t* mesg)
{
    session_restart();
    mesg->status = HMESG_STATUS_OK;
    return 0;
}

int update_state(hmesg_t* mesg)
{
    if (mesg->state.space.id < session_space.id) {
        if (hspace_copy(&mesg->state.space, &session_space) != 0)
            return -1;
    }
    else if (mesg->type != HMESG_JOIN) {
        mesg->state.space.id = 0;
    }

    unsigned client_best = mesg->state.best.id;
    if (strategy_best(&mesg->state.best) != 0) {
        return -1;
    }
    else if (client_best == mesg->state.best.id) {
        mesg->state.best.id = 0;
    }
    return 0;
}

/* ISO C forbids conversion of object pointer to function pointer,
 * making it difficult to use dlsym() for functions.  We get around
 * this by first casting to a word-length integer.  (ILP32/LP64
 * compilers assumed).
 */
#define dlfptr(x, y) ((void (*)(void))(long)(dlsym((x), (y))))

/* Loads strategy given name of library file.
 * Checks that strategy defines required functions,
 * and then calls the strategy's init function (if defined)
 */
int load_strategy(const char* file)
{
    const char* root;
    char* path;
    void* lib;
    hcfg_info_t* keyinfo;

    if (!file)
        return -1;

    root = hcfg_get(&cfg, CFGKEY_HARMONY_HOME);
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

    keyinfo = dlsym(lib, "plugin_keyinfo");
    if (keyinfo) {
        if (hcfg_reginfo(&cfg, keyinfo) != 0) {
            errmsg = "Error registering strategy configuration keys.";
            return -1;
        }
    }

    if (strategy_init)
        return strategy_init(&session_space);

    return 0;
}

int load_layers(const char* list)
{
    const char* home = hcfg_get(&cfg, CFGKEY_HARMONY_HOME);
    char* path = NULL;
    int path_len = 0;
    int retval = 0;

    while (list && *list) {
        void* lib;
        hcfg_info_t* keyinfo;
        const char* end = strchr(list, SESSION_LAYER_SEP);
        if (!end)
            end = list + strlen(list);

        if (lstack_len == lstack_cap) {
            if (array_grow(&lstack, &lstack_cap, sizeof(layer_t)) < 0) {
                retval = -1;
                goto cleanup;
            }
        }

        if (snprintf_grow(&path, &path_len, "%s/libexec/%.*s",
                          home, end - list, list) < 0)
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

        const char* prefix = dlsym(lib, "harmony_layer_name");
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

        keyinfo = dlsym(lib, "plugin_keyinfo");
        if (keyinfo) {
            if (hcfg_reginfo(&cfg, keyinfo) != 0) {
                errmsg = "Error registering strategy configuration keys.";
                return -1;
            }
        }

        if (lstack[lstack_len].init) {
            curr_layer = lstack_len + 1;
            if (lstack[lstack_len].init(&session_space) < 0) {
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
    if (array_grow(&ready, &ready_cap, sizeof(htrial_t*)) != 0) {
        errmsg = "Internal error: Could not extend ready array.";
        return -1;
    }

    pending_cap = target_cap;
    if (array_grow(&pending, &pending_cap, sizeof(htrial_t)) != 0) {
        errmsg = "Internal error: Could not extend pending array.";
        return -1;
    }

    for (i = orig_cap; i < pending_cap; ++i)
        ready[i] = -1;

    return 0;
}

void reverse_array(void* ptr, int head, int tail)
{
    unsigned long* arr = (unsigned long*) ptr;

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
    if (cbs_len >= cbs_cap) {
        if (array_grow(&cbs, &cbs_cap, sizeof(callback_t)) < 0)
            return -1;
    }
    cbs[cbs_len].fd = fd;
    cbs[cbs_len].index = curr_layer;
    cbs[cbs_len].func = func;
    ++cbs_len;

    FD_SET(fd, &fds);
    if (maxfd < fd)
        maxfd = fd;

    return 0;
}

int callback_analyze(int fd, cb_func_t func)
{
    if (cbs_len >= cbs_cap) {
        if (array_grow(&cbs, &cbs_cap, sizeof(callback_t)) < 0)
            return -1;
    }
    cbs[cbs_len].fd = fd;
    cbs[cbs_len].index = -curr_layer;
    cbs[cbs_len].func = func;
    ++cbs_len;

    FD_SET(fd, &fds);
    if (maxfd < fd)
        maxfd = fd;

    return 0;
}

/* Central interface for shared configuration between pluggable modules. */
int session_setcfg(const char* key, const char* val)
{
    int i;

    if (hcfg_set(&cfg, key, val) != 0)
        return -1;

    /* Make sure setcfg callbacks are triggered after the
     * configuration has been set.  Otherwise, any subsequent
     * calls to setcfg further down the stack will be nullified
     * when we return to this frame.
     */
    if (strategy_setcfg && strategy_setcfg(key, val) != 0)
        return -1;

    for (i = 0; i < lstack_len; ++i)
        if (lstack[i].setcfg && lstack[i].setcfg(key, val) != 0)
            return -1;

    return 0;
}

int session_best(hpoint_t* pt)
{
    return strategy_best(pt);
}

int session_restart(void)
{
    int i;

    for (i = lstack_len - 1; i >= 0; --i)
        if (lstack[i].fini && lstack[i].fini() != 0)
            return -1;

    if (strategy_init && strategy_init(&session_space) != 0)
        return -1;

    for (i = 0; i < lstack_len; ++i)
        if (lstack[i].init && lstack[i].init(&session_space) != 0)
            return -1;

    return 0;
}

void session_error(const char* msg)
{
    errmsg = msg;
}
