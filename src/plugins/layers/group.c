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
#include "hsignature.h"
#include "hpoint.h"
#include "defaults.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

/*
 * Name used to identify this plugin.  All Harmony plugins must define
 * this variable.
 */
const char harmony_layer_name[] = "group";

int parse_group(const char *buf);

typedef struct group_def {
    int *idx;
    int idx_len;
} group_def_t;

static int cap_max;
static hval_t *locked_val, *hint_val;
static group_def_t *group;
static int group_len, group_curr;
static hpoint_t best;

int group_init(hsignature_t *sig)
{
    const char *ptr;
    int i;

    if (group_len != group_curr) {
        /* Ignore requests to initialize if mid-search. */
        return 0;
    }

    ptr = session_getcfg("GROUP_LIST");
    if (!ptr) {
        session_error("GROUP_LIST configuration variable must be defined.");
        return -1;
    }

    /* For now, make enough room for the max possible number of groups. */
    cap_max = sig->range_len;
    group = (group_def_t *) malloc(sizeof(group_def_t) * cap_max);
    if (!group) {
        session_error("Could not allocate memory for group definition list.");
        return -1;
    }

    for (i = 0; i < sig->range_len; ++i) {
        /* For now, make enough room for the max possible parameter size. */
        group[i].idx = (int *) malloc(sizeof(int) * cap_max);
        if (!group[i].idx) {
            session_error("Could not allocate memory index list.");
            return -1;
        }
    }

    locked_val = (hval_t *) calloc(sizeof(hval_t), cap_max);
    if (!locked_val) {
        session_error("Could not allocate memory for locked value list.");
        return -1;
    }

    hint_val = (hval_t *) calloc(sizeof(hval_t), cap_max);
    if (!hint_val) {
        session_error("Could not allocate memory for hint value list.");
        return -1;
    }

    if (hpoint_init(&best, cap_max) != 0) {
        session_error("Could not initialize best point container.");
        return -1;
    }

    if (parse_group(ptr) != 0) {
        session_error("Parse error while processing GROUP_LIST.");
        return -1;
    }

    return 0;
}

int group_setcfg(const char *key, const char *val)
{
    int i;

    if (strcmp(key, CFGKEY_STRATEGY_CONVERGED) == 0 && val && *val == '1') {
        session_best(&best);

        /* Update locked values with converged group. */
        for (i = 0; i < group[group_curr].idx_len; ++i) {
            int idx = group[group_curr].idx[i];
            locked_val[idx] = best.val[idx];
        }

        if (++group_curr < group_len)
            return session_restart();
    }
    return 0;
}

int group_generate(hflow_t *flow, htrial_t *trial)
{
    int i;

    if (group_curr < group_len) {
        hval_t *trial_val = trial->point.val;

        /* Initialize locked values, if needed. */
        for (i = 0; i < cap_max; ++i) {
            if (locked_val[i].type == HVAL_UNKNOWN)
                locked_val[i] = trial_val[i];
        }

        /* Base hint values on locked values. */
        memcpy(hint_val, locked_val, sizeof(hval_t) * cap_max);

        /* Allow current group values from the trial point to pass through. */
        for (i = 0; i < group[group_curr].idx_len; ++i) {
            int idx = group[group_curr].idx[i];
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

    if (group_curr == group_len) {
        for (i = 0; i < group_len; ++i)
            free(group[i].idx);
        free(group);
    }
    return 0;
}

int parse_group(const char *buf)
{
    int i, j, count;

    for (i = 0; *buf != '\0'; ++i) {
        while (isspace(*buf)) ++buf;
        if (*buf != '(') break;
        ++buf;

        for (j = 0; *buf != ')'; ++j) {
            if (j == cap_max)
                return -1;
            if (sscanf(buf, " %d %n", &group[i].idx[j], &count) < 1)
                return -1;
            if (0 > group[i].idx[j] || group[i].idx[j] >= cap_max)
                return -1;

            buf += count;
            if (*buf == ',') ++buf;
        }
        ++buf;
        group[i].idx_len = j;

        while (isspace(*buf) || *buf == ',') ++buf;
    }
    if (*buf != '\0')
        return -1;

    group_len = i;
    return 0;
}
