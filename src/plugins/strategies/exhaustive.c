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
 * \page exhaustive Exhaustive (exhaustive.so)
 *
 * This search strategy starts with the minimum-value point (i.e.,
 * using the minimum value for each tuning variable), and incrementing
 * the tuning variables like an odometer until the maximum-value point
 * is reached.  This strategy is guaranteed to visit all points within
 * a search space.
 *
 * It is mainly used as a basis of comparison for more intelligent
 * search strategies.
 *
 *
 * **Configuration Variables**
 * Key    | Type       | Default | Description
 * ------ | ---------- | ------- | -----------
 * PASSES | Integer    | 1       | Number of passes through the search space before the search is considered converged.
 */

#include "strategy.h"
#include "session-core.h"
#include "hperf.h"
#include "hutil.h"
#include "hcfg.h"
#include "defaults.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

hpoint_t best;
double   best_perf;

/* Forward function definitions. */
int strategy_cfg(void);
int increment(void);

/* Variables to track current search state. */
int N;
hrange_t *range;
hpoint_t curr;
unsigned long *idx;
int remaining_passes = 1;
int final_id = 0;

/*
 * Invoked once on strategy load.
 */
int strategy_init(hsignature_t *sig)
{
    int i;

    if (strategy_cfg() != 0)
        return -1;

    best = HPOINT_INITIALIZER;
    best_perf = INFINITY;

    N = sig->range_len;
    range = sig->range;
    if (hpoint_init(&curr, N) != 0) {
        session_error("Could not allocate memory candidate point.");
        return -1;
    }
    for (i = 0; i < N; ++i) {
        curr.val[i].type = range[i].type;
        switch (range[i].type) {
        case HVAL_INT:  curr.val[i].value.i = range[i].bounds.i.min; break;
        case HVAL_REAL: curr.val[i].value.r = range[i].bounds.r.min; break;
        case HVAL_STR:  curr.val[i].value.s = range[i].bounds.s.set[0]; break;
        default:
            session_error("Invalid range detected during strategy init.");
            return -1;
        }
    }
    curr.id = 1;

    idx = (unsigned long *) calloc(N, sizeof(unsigned long));
    if (!idx) {
        session_error("Could not allocate memory for index array.");
        return -1;
    }

    if (session_setcfg(CFGKEY_STRATEGY_CONVERGED, "0") != 0) {
        session_error("Could not set "
                      CFGKEY_STRATEGY_CONVERGED " config variable.");
        return -1;
    }
    return 0;
}

int strategy_cfg(void)
{
    const char *cfgstr;

    cfgstr = session_getcfg(CFGKEY_PASSES);
    if (cfgstr)
        remaining_passes = atoi(cfgstr);

    return 0;
}

/*
 * Generate a new candidate configuration.
 */
int strategy_generate(hflow_t *flow, hpoint_t *point)
{
    if (remaining_passes > 0) {
        if (hpoint_copy(point, &curr) != 0) {
            session_error("Could not copy current point during generation.");
            return -1;
        }

        if (increment() != 0)
            return -1;
        ++curr.id;
    }
    else {
        if (hpoint_copy(point, &best) != 0) {
            session_error("Could not copy best point during generation.");
            return -1;
        }
    }

    flow->status = HFLOW_ACCEPT;
    return 0;
}

/*
 * Regenerate a point deemed invalid by a later plug-in.
 */
int strategy_rejected(hflow_t *flow, hpoint_t *point)
{
    hpoint_t *hint = &flow->point;
    int orig_id = point->id;

    if (hint && hint->id != -1) {
        if (hpoint_copy(point, hint) != 0) {
            session_error("Could not copy hint during reject.");
            return -1;
        }
    }
    else {
        if (hpoint_copy(point, &curr) != 0) {
            session_error("Could not copy current point during reject.");
            return -1;
        }

        if (increment() != 0)
            return -1;
    }
    point->id = orig_id;

    flow->status = HFLOW_ACCEPT;
    return 0;
}

/*
 * Analyze the observed performance for this configuration point.
 */
int strategy_analyze(htrial_t *trial)
{
    double perf = hperf_unify(trial->perf);

    if (best_perf > perf) {
        best_perf = perf;
        if (hpoint_copy(&best, &trial->point) != 0) {
            session_error("Internal error: Could not copy point.");
            return -1;
        }
    }

    if (trial->point.id == final_id) {
        if (session_setcfg(CFGKEY_STRATEGY_CONVERGED, "1") != 0) {
            session_error("Internal error: Could not set convergence status.");
            return -1;
        }
    }

    return 0;
}

/*
 * Return the best performing point thus far in the search.
 */
int strategy_best(hpoint_t *point)
{
    if (hpoint_copy(point, &best) != 0) {
        session_error("Could not copy best point during request for best.");
        return -1;
    }
    return 0;
}

int increment(void)
{
    int i;

    if (remaining_passes <= 0)
        return 0;

    for (i = 0; i < N; ++i) {
        ++idx[i];
        switch (range[i].type) {
        case HVAL_INT:
            curr.val[i].value.i += range[i].bounds.i.step;
            if (curr.val[i].value.i > range[i].bounds.i.max) {
                curr.val[i].value.i = range[i].bounds.i.min;
                idx[i] = 0;
                continue;  // Overflow detected.
            }
            break;

        case HVAL_REAL:
            curr.val[i].value.r =
                (range[i].bounds.r.step > 0.0)
                ? range[i].bounds.r.min + (idx[i] * range[i].bounds.r.step)
                : nextafter(curr.val[i].value.r, INFINITY);

            if (curr.val[i].value.r > range[i].bounds.r.max) {
                curr.val[i].value.r = range[i].bounds.r.min;
                idx[i] = 0;
                continue;  // Overflow detected.
            }
            break;

        case HVAL_STR:
            if (idx[i] >= range[i].bounds.s.set_len) {
                curr.val[i].value.s = range[i].bounds.s.set[0];
                idx[i] = 0;
                continue;  // Overflow detected.
            }
            curr.val[i].value.s = range[i].bounds.s.set[ idx[i] ];
            break;

        default:
            session_error("Invalid range detected.");
            return -1;
        }

        // No overflow detected.  Leave the function.
        return 0;
    }

    // We only reach this point if all values overflowed.
    if (--remaining_passes <= 0)
        final_id = curr.id - 1;

    return 0;
}
