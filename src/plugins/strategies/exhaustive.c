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
 */

#include "strategy.h"
#include "session-core.h"
#include "hutil.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

/*
 * Configuration variables used in this plugin.
 * These will automatically be registered by session-core upon load.
 */
const hcfg_info_t plugin_keyinfo[] = {
    { CFGKEY_PASSES, "1",
      "Number of passes through the search space before the search "
      "is considered converged." },
    { CFGKEY_INIT_POINT, NULL,
      "Initial point begin testing from." },
    { NULL }
};

hpoint_t best = HPOINT_INITIALIZER;
double   best_perf;

//
// Strategy specific structures.
//
typedef union unit {
    unsigned long index;
    double        value;
} unit_u;

/* Forward function definitions. */
int  config_strategy(void);
void increment(void);
int  make_hpoint(const unit_u* index, hpoint_t* point);

/* Variables to track current search state. */
hspace_t* space;
unit_u*   head;
unit_u*   next;
unsigned  next_id;
unit_u*   wrap;

int remaining_passes;
int final_id = 0;
int outstanding_points = 0, final_point_received = 0;

/*
 * Invoked once on strategy load.
 */
int strategy_init(hspace_t* space_ptr)
{
    if (!space) {
        /* One time memory allocation and/or initialization. */

        /* The best point and trial counter should only be initialized once,
         * and thus be retained across a restart.
         */
        best_perf = HUGE_VAL;
        next_id = 1;
    }

    /* Initialization for every call to strategy_init(). */
    if (space != space_ptr) {
        if (next) free(next);
        next = malloc(space_ptr->len * sizeof(*next));
        if (!next) {
            session_error("Could not allocate next index array");
            return -1;
        }

        if (head) free(head);
        head = malloc(space_ptr->len * sizeof(*head));
        if (!head) {
            session_error("Could not allocate head index array");
            return -1;
        }

        if (wrap) free(wrap);
        wrap = malloc(space_ptr->len * sizeof(*wrap));
        if (!wrap) {
            session_error("Could not allocate wrap index array");
            return -1;
        }
        space = space_ptr;
    }

    if (config_strategy() != 0)
        return -1;

    // Determine each search dimension's upper limit.
    for (int i = 0; i < space->len; ++i) {
        if (hrange_finite(&space->dim[i]))
            wrap[i].index = hrange_limit(&space->dim[i]);
        else
            wrap[i].value = space->dim[i].bounds.r.max;
    }

    // Initialize the next point
    memcpy(next, head, space->len * sizeof(*next));

    if (session_setcfg(CFGKEY_CONVERGED, "0") != 0) {
        session_error("Could not set " CFGKEY_CONVERGED " config variable.");
        return -1;
    }
    return 0;
}

/*
 * Generate a new candidate configuration.
 */
int strategy_generate(hflow_t* flow, hpoint_t* point)
{
    if (remaining_passes > 0) {
        if (make_hpoint(next, point) != 0) {
            session_error("Could not make point from index during generate");
            return -1;
        }
        point->id = next_id;

        increment();
        ++next_id;
    }
    else {
        if (hpoint_copy(point, &best) != 0) {
            session_error("Could not copy best point during generation.");
            return -1;
        }
    }

    /* every time we send out a point that's before
       the final point, increment the numebr of points
       we're waiting for results from */
    if(! final_id || next_id <= final_id)
      outstanding_points++;

    flow->status = HFLOW_ACCEPT;
    return 0;
}

/*
 * Regenerate a point deemed invalid by a later plug-in.
 */
int strategy_rejected(hflow_t* flow, hpoint_t* point)
{
    if (flow->point.id) {
        hpoint_t* hint = &flow->point;

        hint->id = point->id;
        if (hpoint_copy(point, hint) != 0) {
            session_error("Could not copy hint during reject.");
            return -1;
        }
    }
    else {
        if (make_hpoint(next, point) != 0) {
            session_error("Could not make point from index during reject");
            return -1;
        }
        increment();
    }

    flow->status = HFLOW_ACCEPT;
    return 0;
}

/*
 * Analyze the observed performance for this configuration point.
 */
int strategy_analyze(htrial_t* trial)
{
    double perf = hperf_unify(&trial->perf);

    if (best_perf > perf) {
        best_perf = perf;
        if (hpoint_copy(&best, &trial->point) != 0) {
            session_error("Internal error: Could not copy point.");
            return -1;
        }
    }

    if (trial->point.id == final_id) {
        if (session_setcfg(CFGKEY_CONVERGED, "1") != 0) {
            session_error("Internal error: Could not set convergence status.");
            return -1;
        }
    }

    /* decrement the number of points we're waiting for
       when we get a point back that was generated before
       the final point */
    if(! final_id || trial->point.id <= final_id)
        outstanding_points--;

    if (trial->point.id == final_id)
        final_point_received = 1;

    /* converged when the final point has been received,
       and there are no outstanding points */
    if(outstanding_points <= 0 && final_point_received) {
        if (session_setcfg(CFGKEY_CONVERGED, "1") != 0) {
            session_error("Internal error: Could not set convergence status.");
            return -1;
        }
    }

    return 0;
}

/*
 * Return the best performing point thus far in the search.
 */
int strategy_best(hpoint_t* point)
{
    if (hpoint_copy(point, &best) != 0) {
        session_error("Could not copy best point during request for best.");
        return -1;
    }
    return 0;
}

//
// Internal helper function implementation.
//
int config_strategy(void)
{
    const char* cfgstr;

    remaining_passes = hcfg_int(session_cfg, CFGKEY_PASSES);
    if (remaining_passes < 0) {
        session_error("Invalid value for " CFGKEY_PASSES ".");
        return -1;
    }

    cfgstr = hcfg_get(session_cfg, CFGKEY_INIT_POINT);
    if (cfgstr) {
        hpoint_t init;

        if (hpoint_parse(&init, cfgstr, space) != 0) {
            session_error("Error parsing point from " CFGKEY_INIT_POINT ".");
            return -1;
        }

        if (!hpoint_align(&init, space) != 0) {
            session_error("Could not align initial point to search space");
            return -1;
        }

        for (int i = 0; i < space->len; ++i) {
            if (hrange_finite(&space->dim[i]))
                head[i].index = hrange_index(&space->dim[i], &init.term[i]);
            else
                head[i].value = init.term[i].value.r;
        }
        hpoint_fini(&init);
    }
    else {
        memset(head, 0, space->len * sizeof(*head));
    }
    return 0;
}

void increment(void)
{
    if (remaining_passes <= 0)
        return;

    for (int i = 0; i < space->len; ++i) {
        if (hrange_finite(&space->dim[i])) {
            ++next[i].index;
            if (next[i].index == wrap[i].index) {
                next[i].index = 0;
                continue; // Overflow detected.
            }
        }
        else {
            double nextval = nextafter(next[i].value, HUGE_VAL);
            if (!(next[i].value < nextval)) {
                next[i].value = space->dim[i].bounds.r.min;
                continue; // Overflow detected.
            }
            next[i].value = nextval;
        }
        return; // No overflow detected.  Exit function.
    }

    // All values overflowed.
    if (--remaining_passes <= 0)
        final_id = next_id;
}

int make_hpoint(const unit_u* unit, hpoint_t* point)
{
    if (point->len != space->len) {
        if (hpoint_init(point, space->len) != 0)
            return -1;
    }

    for (int i = 0; i < point->len; ++i) {
        if (hrange_finite(&space->dim[i]))
            point->term[i] = hrange_value(&space->dim[i], unit[i].index);
        else
            point->term[i].value.r = unit[i].value;
    }
    return 0;
}
