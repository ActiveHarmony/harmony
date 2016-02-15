/*
 * Copyright 2003-2016 Jeffrey K. Hollingsworth
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

/*
 * Structure to encapsulate the state of a Harmony plug-in.
 */
typedef struct hplugin {
    const char* name;
    void*       data;

    struct hook_strategy {
        strategy_generate_t generate;
        strategy_rejected_t reject;
        strategy_analyze_t  analyze;
        strategy_best_t     best;
    } strategy;

    struct hook_layer {
        layer_generate_t generate;
        layer_analyze_t  analyze;
    } layer;

    hook_alloc_t  alloc;
    hook_init_t   init;
    hook_join_t   join;
    hook_setcfg_t setcfg;
    hook_fini_t   fini;

    // Index list of trials waiting in the generation side of this layer.
    int* wait_generate;
    int wait_generate_len;
    int wait_generate_cap;

    // Index list of trials waiting in the analyze side of this layer.
    int* wait_analyze;
    int wait_analyze_len;
    int wait_analyze_cap;

} hplugin_t;

/*
 * Structure to encapsulate the state of a single search instance.
 */
struct hsearch {
    hspace_t space;        // Search space definition.
    hcfg_t   cfg;          // Configuration environment.
    hpoint_t best;         // Best known point.

    hplugin_t* pstack;     // Plug-in stack.
    int        pstack_len;
    int        pstack_cap;

    // Workflow state.
    hflow_t flow;
    int     curr_layer;
    int     paused_id;

    // List of all points generated, but not yet returned to the strategy.
    htrial_t* pending;
    int       pending_cap;
    int       pending_len;
    int       clients;
    int       per_client;

    // List of all trials (point/performance pairs) waiting for client fetch.
    int* ready;
    int  ready_head;
    int  ready_tail;
    int  ready_cap;

    char* buf;
    int   buf_len;
    const char* errmsg;
};

/*
 * Callback registration system.
 */
typedef struct callback {
    int        fd;     // Listen on this file descriptor for incoming data.
    hsearch_t* search; // Search this callback is associated with.
    cb_func_t  func;   // Call this function to process incoming data.
    int        index;  // Return to this layer index when data is ready.
} callback_t;

callback_t* cbs;       // Callback lists.
int         cbs_len;
int         cbs_cap;

/*
 * Variables used for select().
 */
struct timeval  polltime;
struct timeval* pollstate;
fd_set fds;
int maxfd;
int total_pending_len;
int total_pending_cap;

/*
 * Other global variables.
 */
static const char *home_dir;
static hsearch_t slist[1];
static int slist_len;

/*
 * These variables should be removed before the multi-search
 * session-core is merged.
 */
static hsearch_t* tmp_curr_search;
const hcfg_t* session_cfg;

/*
 * Base search structure management prototypes.
 */
static int  open_search(hsearch_t* search, hmesg_t* mesg);
static void close_search(hsearch_t* search);
static void free_search(hsearch_t* search);

/*
 * Internal helper function prototypes.
 */
static int  generate_trial(hsearch_t* search);
static int  plugin_workflow(hsearch_t* search, int trial_idx);
static int  workflow_transition(hsearch_t* search, int trial_idx);
static int  handle_callback(callback_t* cb);
static int  handle_join(hsearch_t* search, hmesg_t* mesg);
static int  handle_getcfg(hsearch_t* search, hmesg_t* mesg);
static int  handle_setcfg(hsearch_t* search, hmesg_t* mesg);
static int  handle_best(hmesg_t* mesg);
static int  handle_fetch(hsearch_t* search, hmesg_t* mesg);
static int  handle_report(hsearch_t* search, hmesg_t* mesg);
static int  handle_reject(hsearch_t* search, int trial_idx);
static int  handle_restart(hsearch_t* search, hmesg_t* mesg);
static int  handle_wait(hsearch_t* search, int trial_idx);
static int  load_strategy(hsearch_t* search, const char* file);
static int  load_layers(hsearch_t* search, const char* list);
static int  extend_lists(hsearch_t* search, int target_cap);
static void reverse_array(void* ptr, int head, int tail);
static int  update_state(hmesg_t* mesg, hsearch_t* search);

/*
 * Core session routines begin here.
 */
int main(int argc, char* argv[])
{
    struct stat sb;
    int retval;
    fd_set ready_fds;
    hmesg_t mesg = HMESG_INITIALIZER;

    if (argc < 2) {
        fprintf(stderr, "%s should not be launched manually.\n", argv[0]);
        return -1;
    }
    home_dir = argv[1];

    // Check that we have been launched correctly by checking that
    // STDIN_FILENO is a socket descriptor.
    //
    // Print an error message to stderr if this is not the case.  This
    // should be the only time an error message is printed to stdout
    // or stderr.
    //
    if (fstat(STDIN_FILENO, &sb) < 0) {
        perror("Could not determine the status of STDIN");
        return -1;
    }

    if (!S_ISSOCK(sb.st_mode)) {
        fprintf(stderr, "%s should not be launched manually.\n", argv[0]);
        return -1;
    }

    // Initialize global data structures.
    pollstate = &polltime;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    maxfd = STDIN_FILENO;

    // Receive the initial session message.
    mesg.type = HMESG_SESSION;
    printf("Receiving initial session message on fd %d\n", STDIN_FILENO);
    if (mesg_recv(STDIN_FILENO, &mesg) < 1) {
        slist[0].errmsg = "Socket or deserialization error";
        goto error;
    }

    if (mesg.type != HMESG_SESSION || mesg.status != HMESG_STATUS_REQ) {
        slist[0].errmsg = "Invalid initial message";
        goto error;
    }

    if (open_search(&slist[0], &mesg) != 0) {
        slist[0].errmsg = "Could not open new search";
        goto error;
    }

    // Send the initial session message acknowledgment.
    mesg.status = HMESG_STATUS_OK;
    if (mesg_send(STDIN_FILENO, &mesg) < 1) {
        slist[0].errmsg = mesg.data.string;
        goto error;
    }

    while (1) {
        ready_fds = fds;
        retval = select(maxfd + 1, &ready_fds, NULL, NULL, pollstate);
        if (retval < 0)
            goto error;

        // Launch callbacks, if needed.
        for (int i = 0; i < cbs_len; ++i) {
            if (FD_ISSET(cbs[i].fd, &ready_fds))
                handle_callback(&cbs[i]);
        }

        // Handle hmesg_t, if needed.
        if (FD_ISSET(STDIN_FILENO, &ready_fds)) {
            hsearch_t* search;

            retval = mesg_recv(STDIN_FILENO, &mesg);
            if (retval == 0) goto cleanup;
            if (retval <  0) goto error;

            // This is where the an existing search will be found, or
            // a new search will be created.
            //
            search = &slist[0];
            tmp_curr_search = &slist[0];
            session_cfg = &slist[0].cfg;
            search->flow.status = HFLOW_ACCEPT;
            search->flow.point  = hpoint_zero;

            hcfg_set(&search->cfg, CFGKEY_CURRENT_CLIENT, mesg.state.client);
            switch (mesg.type) {
            case HMESG_JOIN:    retval = handle_join(search, &mesg); break;
            case HMESG_GETCFG:  retval = handle_getcfg(search, &mesg); break;
            case HMESG_SETCFG:  retval = handle_setcfg(search, &mesg); break;
            case HMESG_BEST:    retval = handle_best(&mesg); break;
            case HMESG_FETCH:   retval = handle_fetch(search, &mesg); break;
            case HMESG_REPORT:  retval = handle_report(search, &mesg); break;
            case HMESG_RESTART: retval = handle_restart(search, &mesg); break;

            default:
                search->errmsg = "Invalid message type";
                goto error;
            }
            if (retval != 0)
                goto error;

            if (update_state(&mesg, search) != 0) {
                search->errmsg = "Could not update message session state";
                goto error;
            }
            hcfg_set(&search->cfg, CFGKEY_CURRENT_CLIENT, NULL);

            if (mesg_send(STDIN_FILENO, &mesg) < 1)
                goto error;

            tmp_curr_search = NULL;
            session_cfg = NULL;
        }

        // Generate another point, if there's room in the queue.
        while (pollstate != NULL) {

            tmp_curr_search = &slist[0];
            session_cfg = &slist[0].cfg;

            generate_trial(&slist[0]);

            tmp_curr_search = NULL;
            session_cfg = NULL;

            if (slist[0].pending_len == slist[0].pending_cap ||
                slist[0].flow.status == HFLOW_WAIT)
                pollstate = NULL;
        }
    }

  error:
    mesg.status = HMESG_STATUS_FAIL;
    mesg.data.string = slist[0].errmsg;
    if (mesg_send(STDIN_FILENO, &mesg) < 1) {
        fprintf(stderr, "%s: Error sending error message: %s\n",
                argv[0], mesg.data.string);
    }
    retval = -1;

  cleanup:
    close_search(&slist[0]);
    free_search(&slist[0]);
    free(cbs);
    hmesg_fini(&mesg);

    return retval;
}

/*
 * Base search structure management prototypes.
 */
int open_search(hsearch_t* search, hmesg_t* mesg)
{
    const char* ptr;

    // Before anything else, control the random seeds.
    static long seed;
    if (!seed) {
        seed = hcfg_int(&search->cfg, CFGKEY_RANDOM_SEED);
        if (seed < 0)
            seed = time(NULL);
        srand((int) seed);
        srand48(seed);
    }

    // Initialize the session.
    if (hspace_copy(&search->space, mesg->state.space) != 0) {
        search->errmsg = "Could not copy session data";
        return -1;
    }

    if (hcfg_copy(&search->cfg, mesg->data.cfg) != 0) {
        search->errmsg = "Could not copy configuration environment";
        return -1;
    }

    // Overwrite CFGKEY_HARMONY_HOME.
    if (hcfg_set(&search->cfg, CFGKEY_HARMONY_HOME, home_dir) != 0) {
        search->errmsg = "Could not set " CFGKEY_HARMONY_HOME " for session";
        return -1;
    }

    if (hcfg_int(&search->cfg, CFGKEY_PERF_COUNT) < 1) {
        search->errmsg = "Invalid " CFGKEY_PERF_COUNT " configuration value";
        return -1;
    }

    search->per_client = hcfg_int(&search->cfg, CFGKEY_GEN_COUNT);
    if (search->per_client < 1) {
        search->errmsg = "Invalid " CFGKEY_GEN_COUNT " configuration value";
        return -1;
    }

    int expected = hcfg_int(&search->cfg, CFGKEY_CLIENT_COUNT);
    if (expected < 1) {
        search->errmsg = "Invalid " CFGKEY_CLIENT_COUNT " configuration value";
        return -1;
    }

    if (extend_lists(search, expected * search->per_client) != 0)
        return -1;

    // Load and initialize the strategy code object.
    ptr = hcfg_get(&search->cfg, CFGKEY_STRATEGY);
    if (!ptr) {
        if (expected > 1)
            ptr = "pro.so";
        else
            ptr = "nm.so";
    }

    tmp_curr_search = &slist[0];
    session_cfg = &slist[0].cfg;

    if (load_strategy(search, ptr) != 0)
        return -1;

    // Load and initialize requested layers.
    ptr = hcfg_get(&search->cfg, CFGKEY_LAYERS);
    if (ptr && load_layers(search, ptr) != 0)
        return -1;

    tmp_curr_search = NULL;
    session_cfg = NULL;

    search->clients = 1;
    ++slist_len;
    return 0;
}

void close_search(hsearch_t* search)
{
    for (int i = search->pstack_len - 1; i > 0; --i) {
        if (search->pstack[i].fini)
            search->pstack[i].fini();
    }

    search->pstack_len = 0;
    search->ready_head = 0;
    search->ready_tail = 0;
    search->pending_len = 0;
    search->best.id = 0;
    search->space.id = 0;
    --slist_len;
}

void free_search(hsearch_t* search)
{
    free(search->buf);
    free(search->ready);

    for (int i = 0; i < search->pending_cap; ++i) {
        hpoint_fini((hpoint_t*) &search->pending[i].point);
        hperf_fini(&search->pending[i].perf);
    }
    free(search->pending);

    hpoint_fini(&search->flow.point);

    free(search->pstack);

    hpoint_fini(&search->best);
    hcfg_fini(&search->cfg);
    hspace_fini(&search->space);
}

/*
 * Internal helper function implementation.
 */
int generate_trial(hsearch_t* search)
{
    int idx;
    htrial_t* trial = NULL;

    // Find a free point.
    for (idx = 0; idx < search->pending_cap; ++idx) {
        trial = &search->pending[idx];
        if (!trial->point.id)
            break;
    }
    if (idx == search->pending_cap) {
        search->errmsg = "Point generation overflow";
        return -1;
    }

    // Reset the performance for this trial.
    hperf_reset(&trial->perf);

    // Call strategy generation routine.
    if (search->pstack[0].strategy.generate(&search->flow,
                                            (hpoint_t*)&trial->point) != 0)
        return -1;

    if (search->flow.status == HFLOW_ACCEPT) {
        ++search->pending_len;
        ++total_pending_len;

        // Begin generation workflow for new point.
        search->curr_layer = 1;
        return plugin_workflow(search, idx);
    }
    return 0;
}

int plugin_workflow(hsearch_t* search, int trial_idx)
{
    htrial_t* trial = &search->pending[trial_idx];

    while (search->curr_layer != 0 && search->curr_layer <= search->pstack_len)
    {
        int stack_idx = abs(search->curr_layer) - 1;
        int retval;

        search->flow.status = HFLOW_ACCEPT;
        if (search->curr_layer < 0) {
            // Analyze workflow.
            if (search->pstack[stack_idx].layer.analyze) {
                if (search->pstack[stack_idx].layer.analyze(&search->flow,
                                                            trial) != 0)
                    return -1;
            }
        }
        else {
            // Generate workflow.
            if (search->pstack[stack_idx].layer.generate) {
                if (search->pstack[stack_idx].layer.generate(&search->flow,
                                                             trial) != 0)
                    return -1;
            }
        }

        retval = workflow_transition(search, trial_idx);
        if (retval < 0) return -1;
        if (retval > 0) return  0;
    }

    if (search->curr_layer == 0) {
        // Completed analysis layers.  Send trial to strategy.
        if (search->pstack[0].strategy.analyze(trial) != 0)
            return -1;

        // Remove point data from pending list.
        ((hpoint_t*)&trial->point)->id = 0;
        --search->pending_len;

        // Point generation attempts may begin again.
        pollstate = &polltime;
    }
    else if (search->curr_layer > search->pstack_len) {
        // Completed generation layers.  Enqueue trial in ready queue.
        if (search->ready[search->ready_tail] != -1) {
            search->errmsg = "Ready queue overflow";
            return -1;
        }

        search->ready[search->ready_tail] = trial_idx;
        search->ready_tail = (search->ready_tail + 1) % search->ready_cap;
    }
    else {
        search->errmsg = "Invalid current plug-in layer";
        return -1;
    }

    return 0;
}

// Layer state machine transitions.
int workflow_transition(hsearch_t* search, int trial_idx)
{
    switch (search->flow.status) {
    case HFLOW_ACCEPT:
        search->curr_layer += 1;
        break;

    case HFLOW_WAIT:
        if (handle_wait(search, trial_idx) != 0) {
            return -1;
        }
        return 1;

    case HFLOW_RETURN:
    case HFLOW_RETRY:
        search->curr_layer = -search->curr_layer;
        break;

    case HFLOW_REJECT:
        if (handle_reject(search, trial_idx) != 0)
            return -1;

        if (search->flow.status == HFLOW_WAIT)
            return 1;

        search->curr_layer = 1;
        break;

    default:
        search->errmsg = "Invalid workflow status";
        return -1;
    }
    return 0;
}

int handle_reject(hsearch_t* search, int trial_idx)
{
    htrial_t* trial = &search->pending[trial_idx];

    if (search->curr_layer < 0) {
        search->errmsg = "REJECT invalid for analysis workflow";
        return -1;
    }

    // Regenerate this rejected point.
    if (search->pstack[0].strategy.reject(&search->flow,
                                          (hpoint_t*) &trial->point) != 0)
        return -1;

    return 0;
}

int handle_wait(hsearch_t* search, int trial_idx)
{
    int   idx = abs(search->curr_layer) - 1;
    int** list;
    int*  len;
    int*  cap;

    if (search->curr_layer < 0) {
        list = &search->pstack[idx].wait_analyze;
        len  = &search->pstack[idx].wait_analyze_len;
        cap  = &search->pstack[idx].wait_analyze_cap;
    }
    else {
        list = &search->pstack[idx].wait_generate;
        len  = &search->pstack[idx].wait_generate_len;
        cap  = &search->pstack[idx].wait_generate_cap;
    }

    if (*len == *cap) {
        int i;

        array_grow(list, cap, sizeof(int));
        for (i = *len; i < *cap; ++i)
            (*list)[i] = -1;
    }

    if ((*list)[*len] != -1) {
        search->errmsg = "Could not append to wait list";
        return -1;
    }

    (*list)[*len] = trial_idx;
    ++(*len);
    return 0;
}

int handle_callback(callback_t* cb)
{
    hsearch_t* search = cb->search;
    htrial_t** trial_list;
    int* list;
    int* len;
    int  trial_idx, idx, retval;

    search->curr_layer = cb->index;
    idx = abs(search->curr_layer) - 1;

    // The idx variable represents layer plugin index for now.
    if (search->curr_layer < 0) {
        list = search->pstack[idx].wait_analyze;
        len = &search->pstack[idx].wait_analyze_len;
    }
    else {
        list = search->pstack[idx].wait_generate;
        len = &search->pstack[idx].wait_generate_len;
    }

    if (*len < 1) {
        search->errmsg = "Callback on layer with empty waitlist";
        return -1;
    }

    // Prepare a list of htrial_t pointers.
    trial_list = malloc(*len * sizeof(htrial_t*));
    for (int i = 0; i < *len; ++i)
        trial_list[i] = &search->pending[ list[i] ];

    // Reusing idx to represent waitlist index.  (Shame on me.)
    idx = cb->func(cb->fd, &search->flow, *len, trial_list);
    free(trial_list);

    trial_idx = list[idx];
    retval = workflow_transition(search, trial_idx);
    if (retval < 0) return -1;
    if (retval > 0) return  0;

    --(*len);
    list[ idx] = list[*len];
    list[*len] = -1;
    return plugin_workflow(search, trial_idx);
}

int handle_join(hsearch_t* search, hmesg_t* mesg)
{
    // Grow the pending and ready queues.
    ++search->clients;
    if (extend_lists(search, search->clients * search->per_client) != 0)
        return -1;

    // Launch all join hooks defined in the plug-in stack.
    for (int i = 0; i < search->pstack_len; ++i) {
        if (search->pstack[i].join) {
            if (search->pstack[i].join(mesg->state.client) != 0)
                return -1;
        }
    }

    mesg->state.space = &search->space;
    mesg->status = HMESG_STATUS_OK;
    return 0;
}

int handle_getcfg(hsearch_t* search, hmesg_t* mesg)
{
    // Prepare getcfg response message for client.
    const char* cfgval = hcfg_get(&search->cfg, mesg->data.string);
    if (cfgval == NULL)
        cfgval = "";

    snprintf_grow(&search->buf, &search->buf_len, "%s=%s",
                  mesg->data.string, cfgval);
    mesg->data.string = search->buf;
    mesg->status = HMESG_STATUS_OK;
    return 0;
}

int handle_setcfg(hsearch_t* search, hmesg_t* mesg)
{
    char* sep = (char*) strchr(mesg->data.string, '=');
    const char* oldval;

    if (!sep) {
        search->errmsg = strerror(EINVAL);
        return -1;
    }
    *sep = '\0';

    // Store the original value, possibly allocating memory for it.
    oldval = hcfg_get(&search->cfg, mesg->data.string);
    if (oldval) {
        snprintf_grow(&search->buf, &search->buf_len, "%s", oldval);
        oldval = search->buf;
    }

    if (session_setcfg(mesg->data.string, sep + 1) != 0) {
        search->errmsg = "Error setting session configuration variable";
        return -1;
    }

    // Prepare setcfg response message for client.
    mesg->data.string = oldval;
    mesg->status = HMESG_STATUS_OK;
    return 0;
}

int handle_best(hmesg_t* mesg)
{
    mesg->status = HMESG_STATUS_OK;
    return 0;
}

int handle_fetch(hsearch_t* search, hmesg_t* mesg)
{
    int idx = search->ready[search->ready_head], paused;

    // Check if the session is paused.
    paused = hcfg_bool(&search->cfg, CFGKEY_PAUSED);

    if (!paused && idx >= 0) {
        // Send the next point on the ready queue.
        mesg->data.point = &search->pending[idx].point;

        // Remove the first point from the ready queue.
        search->ready[search->ready_head] = -1;
        search->ready_head = (search->ready_head + 1) % search->ready_cap;

        mesg->status = HMESG_STATUS_OK;
    }
    else {
        // Ready queue is empty, or session is paused.
        // Send the best known point.
        mesg->state.best = &search->best;
        if (search->pstack[0].strategy.best(&search->best) != 0)
            return -1;

        search->paused_id = mesg->data.point->id;
        mesg->status = HMESG_STATUS_BUSY;
    }
    return 0;
}

int handle_report(hsearch_t* search, hmesg_t* mesg)
{
    int idx;
    htrial_t* trial = NULL;

    // Find the associated trial in the pending list.
    for (idx = 0; idx < search->pending_cap; ++idx) {
        trial = &search->pending[idx];
        if (trial->point.id == mesg->data.point->id)
            break;
    }
    if (idx == search->pending_cap) {
        if (mesg->data.point->id == search->paused_id) {
            mesg->status = HMESG_STATUS_OK;
            return 0;
        }
        else {
            search->errmsg = "Rouge point support not yet implemented";
            return -1;
        }
    }
    search->paused_id = 0;

    // Update performance in our local records.
    hperf_copy(&trial->perf, mesg->data.perf);

    // Begin the workflow at the outermost analysis layer.
    search->curr_layer = -search->pstack_len;
    if (plugin_workflow(search, idx) != 0)
        return -1;

    mesg->status = HMESG_STATUS_OK;
    return 0;
}

int handle_restart(hsearch_t* search, hmesg_t* mesg)
{
    session_restart();
    mesg->status = HMESG_STATUS_OK;
    return 0;
}

int update_state(hmesg_t* mesg, hsearch_t* search)
{
    if (mesg->state.space->id < search->space.id) {
        mesg->state.space = &search->space;
    }
    else if (mesg->type != HMESG_JOIN) {
        mesg->state.space = &mesg->unpacked_space;
        mesg->unpacked_space.id = 0;
    }

    // Refresh the best known point from the strategy.
    if (search->pstack[0].strategy.best(&search->best) != 0) {
        return -1;
    }

    // Send it to the client if necessary.
    if (mesg->state.best->id < search->best.id) {
        mesg->state.best = &search->best;
    }
    else {
        mesg->state.best = &mesg->unpacked_best;
        mesg->unpacked_best.id = 0;
    }
    return 0;
}

/*
 * ISO C forbids conversion of object pointer to function pointer,
 * making it difficult to use dlsym() for functions.  We get around
 * this by first casting to a word-length integer.  (ILP32/LP64
 * compilers assumed).
 */
#define dlfptr(x, y) ((void*) (void (*)(void))(long)(dlsym((x), (y))))

/*
 * Loads strategy given name of library file.
 * Checks that strategy defines required functions,
 * and then calls the strategy's init function (if defined)
 */
int load_strategy(hsearch_t* search, const char* file)
{
    char* path;
    void* lib;
    void* func;
    hcfg_info_t* keyinfo;

    if (!file)
        return -1;

    path = sprintf_alloc("%s/libexec/%s", home_dir, file);
    lib = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    free(path);
    if (!lib) {
        search->errmsg = dlerror();
        return -1;
    }

    if (search->pstack_len == search->pstack_cap) {
        if (array_grow(&search->pstack, &search->pstack_cap,
                       sizeof(*search->pstack)) != 0)
        {
            search->errmsg = "Could not allocate memory for plug-in stack";
            return -1;
        }
    }

    func = dlfptr(lib, "strategy_generate");
    if (!func) {
        search->errmsg = "Strategy does not define strategy_generate()";
        return -1;
    }
    search->pstack[0].strategy.generate = (strategy_generate_t) func;

    func = dlfptr(lib, "strategy_rejected");
    if (!func) {
        search->errmsg = "Strategy does not define strategy_rejected()";
        return -1;
    }
    search->pstack[0].strategy.reject = (strategy_rejected_t) func;

    func = dlfptr(lib, "strategy_analyze");
    if (!func) {
        search->errmsg = "Strategy does not define strategy_analyze()";
        return -1;
    }
    search->pstack[0].strategy.analyze = (strategy_analyze_t) func;

    func = dlfptr(lib, "strategy_best");
    if (!func) {
        search->errmsg = "Strategy does not define strategy_best()";
        return -1;
    }
    search->pstack[0].strategy.best = (strategy_best_t) func;

    keyinfo = dlsym(lib, "plugin_keyinfo");
    if (keyinfo) {
        if (hcfg_reginfo(&search->cfg, keyinfo) != 0) {
            search->errmsg = "Error registering strategy configuration keys";
            return -1;
        }
    }

    search->pstack[0].alloc  =  (hook_alloc_t) dlfptr(lib, "strategy_alloc");
    search->pstack[0].init   =   (hook_init_t) dlfptr(lib, "strategy_init");
    search->pstack[0].join   =   (hook_join_t) dlfptr(lib, "strategy_join");
    search->pstack[0].setcfg = (hook_setcfg_t) dlfptr(lib, "strategy_setcfg");
    search->pstack[0].fini   =   (hook_fini_t) dlfptr(lib, "strategy_fini");

    if (search->pstack[0].alloc) {
        search->pstack[0].data = search->pstack[0].alloc();
        if (!search->pstack[0].data)
            return -1;
    }

    if (search->pstack[0].init) {
        if (search->pstack[0].init(&search->space) != 0)
            return -1;
    }

    ++search->pstack_len;
    return 0;
}

int load_layers(hsearch_t* search, const char* list)
{
    char* path = NULL;
    int path_len = 0;
    int retval = 0;

    while (list && *list) {
        void* lib;
        hcfg_info_t* keyinfo;
        int tail = search->pstack_len;

        const char* end = strchr(list, SESSION_LAYER_SEP);
        if (!end)
            end = list + strlen(list);

        if (search->pstack_len == search->pstack_cap) {
            if (array_grow(&search->pstack, &search->pstack_cap,
                           sizeof(*search->pstack)) != 0)
            {
                retval = -1;
                goto cleanup;
            }
        }

        if (snprintf_grow(&path, &path_len, "%s/libexec/%.*s",
                          home_dir, end - list, list) < 0)
        {
            retval = -1;
            goto cleanup;
        }

        lib = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
        if (!lib) {
            search->errmsg = dlerror();
            retval = -1;
            goto cleanup;
        }

        const char* prefix = dlsym(lib, "harmony_layer_name");
        if (!prefix) {
            search->errmsg = dlerror();
            retval = -1;
            goto cleanup;
        }
        search->pstack[tail].name = prefix;

        if (snprintf_grow(&path, &path_len, "%s_alloc", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }
        search->pstack[tail].alloc = (hook_alloc_t) dlfptr(lib, path);

        if (snprintf_grow(&path, &path_len, "%s_init", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }
        search->pstack[tail].init = (hook_init_t) dlfptr(lib, path);

        if (snprintf_grow(&path, &path_len, "%s_join", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }
        search->pstack[tail].join = (hook_join_t) dlfptr(lib, path);

        if (snprintf_grow(&path, &path_len, "%s_generate", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }
        search->pstack[tail].layer.generate =
            (layer_generate_t) dlfptr(lib, path);

        if (snprintf_grow(&path, &path_len, "%s_analyze", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }
        search->pstack[tail].layer.analyze =
            (layer_analyze_t) dlfptr(lib, path);

        if (snprintf_grow(&path, &path_len, "%s_setcfg", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }
        search->pstack[tail].setcfg = (hook_setcfg_t) dlfptr(lib, path);

        if (snprintf_grow(&path, &path_len, "%s_fini", prefix) < 0) {
            retval = -1;
            goto cleanup;
        }
        search->pstack[tail].fini = (hook_fini_t) dlfptr(lib, path);

        keyinfo = dlsym(lib, "plugin_keyinfo");
        if (keyinfo) {
            if (hcfg_reginfo(&search->cfg, keyinfo) != 0) {
                search->errmsg = "Error registering strategy default config";
                retval = -1;
                goto cleanup;
            }
        }

        if (search->pstack[tail].alloc) {
            search->pstack[tail].data = search->pstack[tail].alloc();
            if (!search->pstack[tail].data) {
                retval = -1;
                goto cleanup;
            }
        }

        if (search->pstack[tail].init) {
            search->curr_layer = tail + 1;
            if (search->pstack[tail].init(&search->space) != 0) {
                retval = -1;
                goto cleanup;
            }
        }
        ++search->pstack_len;

        if (*end)
            list = end + 1;
        else
            list = NULL;
    }

  cleanup:
    free(path);
    return retval;
}

int extend_lists(hsearch_t* search, int target_cap)
{
    int orig_cap = search->pending_cap;

    if (orig_cap >= target_cap && orig_cap != 0)
        return 0;

    if (   search->ready
        && search->ready_tail <= search->ready_head
        && search->ready[search->ready_head] != -1)
    {
        int i = search->ready_cap - search->ready_head;

        // Shift ready array to align head with array index 0.
        reverse_array(search->ready, 0, search->ready_cap);
        reverse_array(search->ready, 0, i);
        reverse_array(search->ready, i, search->ready_cap);

        search->ready_head = 0;
        search->ready_tail = (search->ready_cap
                              - search->ready_tail
                              + search->ready_head);
    }

    search->ready_cap = target_cap;
    if (array_grow(&search->ready, &search->ready_cap,
                   sizeof(*search->ready)) != 0)
    {
        search->errmsg = "Could not extend ready array";
        return -1;
    }

    search->pending_cap = target_cap;
    if (array_grow(&search->pending, &search->pending_cap,
                   sizeof(*search->pending)) != 0)
    {
        search->errmsg = "Could not extend pending array";
        return -1;
    }

    for (int i = orig_cap; i < search->pending_cap; ++i)
        search->ready[i] = -1;

    return 0;
}

void reverse_array(void* ptr, int head, int tail)
{
    unsigned long* arr = (unsigned long*) ptr;

    while (head < --tail) {
        // Swap head and tail entries.
        arr[head] ^= arr[tail];
        arr[tail] ^= arr[head];
        arr[head] ^= arr[tail];
        ++head;
    }
}

/*
 * Exported functions for pluggable modules.
 */
int callback_generate(int fd, cb_func_t func)
{
    if (cbs_len >= cbs_cap) {
        if (array_grow(&cbs, &cbs_cap, sizeof(*cbs)) != 0)
            return -1;
    }

    cbs[ cbs_len ].search = tmp_curr_search;
    cbs[ cbs_len ].fd     = fd;
    cbs[ cbs_len ].index  = tmp_curr_search->curr_layer;
    cbs[ cbs_len ].func   = func;
    ++cbs_len;

    FD_SET(fd, &fds);
    if (maxfd < fd)
        maxfd = fd;

    return 0;
}

int callback_analyze(int fd, cb_func_t func)
{
    if (cbs_len >= cbs_cap) {
        if (array_grow(&cbs, &cbs_cap, sizeof(*cbs)) != 0)
            return -1;
    }

    cbs[ cbs_len ].search = tmp_curr_search;
    cbs[ cbs_len ].fd     = fd;
    cbs[ cbs_len ].index  = -tmp_curr_search->curr_layer;
    cbs[ cbs_len ].func   = func;
    ++cbs_len;

    FD_SET(fd, &fds);
    if (maxfd < fd)
        maxfd = fd;

    return 0;
}

/*
 * Central interface for shared configuration between pluggable modules.
 */
int session_setcfg(const char* key, const char* val)
{
    if (hcfg_set(&tmp_curr_search->cfg, key, val) != 0)
        return -1;

    // Make sure setcfg callbacks are triggered after the
    // configuration has been set.  Otherwise, any subsequent calls to
    // setcfg further down the stack will be nullified when we return
    // to this frame.
    //
    for (int i = 0; i < tmp_curr_search->pstack_len; ++i) {
        if (tmp_curr_search->pstack[i].setcfg) {
            if (tmp_curr_search->pstack[i].setcfg(key, val) != 0)
                return -1;
        }
    }
    return 0;
}

int session_best(hpoint_t* pt)
{
    return tmp_curr_search->pstack[0].strategy.best(pt);
}

int session_restart(void)
{
    // Finalize all plug-ins associated with this search.
    for (int i = tmp_curr_search->pstack_len - 1; i >= 0; --i) {
        if (tmp_curr_search->pstack[i].fini) {
            if (tmp_curr_search->pstack[i].fini() != 0)
                return -1;
        }
    }

    // Re-initialize all plug-ins associated with this search.
    for (int i = 0; i < tmp_curr_search->pstack_len; ++i) {
        if (tmp_curr_search->pstack[i].init) {
            if (tmp_curr_search->pstack[i].init(&tmp_curr_search->space) != 0)
                return -1;
        }
    }

    return 0;
}

void session_error(const char* msg)
{
    tmp_curr_search->errmsg = msg;
}
