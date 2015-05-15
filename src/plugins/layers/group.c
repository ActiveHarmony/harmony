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

/**
 * \page group Input Grouping (group.so)
 *
 * This processing layer allows search space input variables
 * (dimensions) to be iteratively search in groups.  The value for all
 * input variables not in the current search group remain constant.
 *
 * Input variables are specified via a zero-based index, determined by
 * the order they are defined in the session signature.  For instance,
 * the first variable defined via harmony_int(), harmony_real(), or
 * harmony_enum() can be referenced via the index 0.
 *
 * ### Example ###
 * Given a tuning session with nine input variables, the following
 * group specification:
 *
 *     (0,1,2),(3,4,5,6),(7,8)
 *
 * would instruct the layer to search the first three variables until
 * the search strategy converges.  Input values for the other six
 * variables remain constant during this search.
 *
 * Upon convergence, the search is restarted and allowed to
 * investigate the next four variables.  The first three variables are
 * forced to use the best discovered values from the prior search.
 * This pattern continues until all groups have been searched.
 *
 * Grouping should be beneficial for search spaces whose input
 * variables are relatively independent with respect to the reported
 * performance.
 */

#include "session-core.h"
#include "hsignature.h"
#include "hpoint.h"
#include "hutil.h"
#include "hcfg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

/*
 * Name used to identify this plugin layer.
 * All Harmony plugin layers must define this variable.
 */
const char harmony_layer_name[] = "group";

/*
 * Configuration variables used in this plugin.
 * These will automatically be registered by session-core upon load.
 */
hcfg_info_t plugin_keyinfo[] = {
    { CFGKEY_GROUP_LIST, NULL, "List of input parameter indexes." },
    { NULL }
};

int parse_group(const char* buf);

typedef struct group_def {
    int* idx;
    int idx_len;
} group_def_t;

static char internal_restart_req;
static int cap_max;
static hval_t* locked_val;
static hval_t* hint_val;
static group_def_t* glist;
static int glist_len, glist_cap;
static int glist_curr;
static hpoint_t best;

int group_init(hsignature_t* sig)
{
    const char* ptr;

    if (internal_restart_req) {
        /* Ignore our own requests to re-initialize. */
        return 0;
    }

    ptr = hcfg_get(session_cfg, CFGKEY_GROUP_LIST);
    if (!ptr) {
        session_error(CFGKEY_GROUP_LIST
                      " configuration variable must be defined.");
        return -1;
    }

    /* The maximum group size is the number of input ranges. */
    cap_max = sig->range_len;

    locked_val = calloc(cap_max, sizeof(hval_t));
    if (!locked_val) {
        session_error("Could not allocate memory for locked value list.");
        return -1;
    }

    hint_val = calloc(cap_max, sizeof(hval_t));
    if (!hint_val) {
        session_error("Could not allocate memory for hint value list.");
        return -1;
    }

    if (hpoint_init(&best, cap_max) != 0) {
        session_error("Could not initialize best point container.");
        return -1;
    }

    glist = NULL;
    glist_len = glist_cap = 0;
    return parse_group(ptr);
}

int group_setcfg(const char* key, const char* val)
{
    int i, retval = 0;

    if (strcmp(key, CFGKEY_CONVERGED) == 0 && val && *val == '1') {
        session_best(&best);

        /* Update locked values with converged group. */
        for (i = 0; i < glist[glist_curr].idx_len; ++i) {
            int idx = glist[glist_curr].idx[i];
            locked_val[idx] = best.val[idx];
        }

        if (++glist_curr < glist_len) {
            internal_restart_req = 1;
            retval = session_restart();
            internal_restart_req = 0;
        }
    }
    return retval;
}

int group_generate(hflow_t* flow, htrial_t* trial)
{
    int i;

    if (glist_curr < glist_len) {
        hval_t* trial_val = trial->point.val;

        /* Initialize locked values, if needed. */
        for (i = 0; i < cap_max; ++i) {
            if (locked_val[i].type == HVAL_UNKNOWN)
                locked_val[i] = trial_val[i];
        }

        /* Base hint values on locked values. */
        memcpy(hint_val, locked_val, sizeof(hval_t) * cap_max);

        /* Allow current group values from the trial point to pass through. */
        for (i = 0; i < glist[glist_curr].idx_len; ++i) {
            int idx = glist[glist_curr].idx[i];
            hint_val[idx] = trial_val[idx];
        }

        /* If any values differ from our hint, reject it. */
        if (memcmp(hint_val, trial_val, sizeof(hval_t) * cap_max) != 0) {
            flow->point.id = 0;
            flow->point.n = cap_max;
            flow->point.val = hint_val;
            flow->status = HFLOW_REJECT;
            return 0;
        }
    }

    flow->status = HFLOW_ACCEPT;
    return 0;
}

int group_fini(void)
{
    int i;

    if (glist_curr == glist_len) {
        for (i = 0; i < glist_len; ++i)
            free(glist[i].idx);
        free(glist);
    }
    return 0;
}

int parse_group(const char* buf)
{
    int i, j, count, success;

    int* list = malloc(cap_max * sizeof(int));
    char* seen = calloc(cap_max, sizeof(char));

    if (!list || !seen) {
        session_error("Error allocating memory for group parsing function");
        return -1;
    }

    success = 0;
    for (i = 0; *buf != '\0'; ++i) {
        while (isspace(*buf)) ++buf;
        if (*buf != '(') break;
        ++buf;

        if (i >= glist_cap - 1) {
            if (array_grow(&glist, &glist_cap, sizeof(group_def_t)) != 0) {
                session_error("Error allocating group definition list");
                goto cleanup;
            }
        }

        for (j = 0; *buf != ')'; ++j) {
            if (j >= cap_max) {
                session_error("Too many indexes in group specification");
                goto cleanup;
            }
            if (sscanf(buf, " %d %n", &list[j], &count) < 1) {
                session_error("Error parsing group specification");
                goto cleanup;
            }
            if (list[j] < 0 || list[j] >= cap_max) {
                session_error("Group specification member out of bounds");
                goto cleanup;
            }
            seen[ list[j] ] = 1;
            buf += count;
            if (*buf == ',') ++buf;
        }
        ++buf;

        /* Allocate memory for the parsed index list. */
        glist[i].idx = malloc(j * sizeof(int));
        if (!glist[i].idx) {
            session_error("Error allocating memory for group index list");
            goto cleanup;
        }
        /* Copy index from list into glist. */
        memcpy(glist[i].idx, list, j * sizeof(int));
        glist[i].idx_len = j;

        while (isspace(*buf) || *buf == ',') ++buf;
    }
    if (*buf != '\0') {
        session_error("Error parsing group specification");
        goto cleanup;
    }
    glist_len = i;

    /* Produce a final group of all unseen input indexes. */
    for (i = 0, j = 0; j < cap_max; ++j) {
        if (!seen[j])
            list[i++] = j;
    }

    /* Only add the final group if necessary. */
    if (i) {
        glist[glist_len].idx = list;
        glist[glist_len].idx_len = i;
        ++glist_len;
    }
    else {
        free(list);
    }
    success = 1;

  cleanup:
    free(seen);

    if (!success) {
        free(list);
        return -1;
    }

    return 0;
}
