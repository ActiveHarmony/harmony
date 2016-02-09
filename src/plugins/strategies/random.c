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

/**
 * \page random Random (random.so)
 *
 * This search strategy generates random points within the search
 * space.  Using* a pseudo-random method, a value is selected for each
 * tuning variable according to its defined bounds.  This search will
 * never reach a converged state.
 *
 * It is mainly used as a basis of comparison for more intelligent
 * search strategies.
 */

#include "strategy.h"
#include "session-core.h"
#include "hcfg.h"
#include "hspace.h"
#include "hpoint.h"
#include "hperf.h"
#include "hutil.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>

/*
 * Configuration variables used in this plugin.
 * These will automatically be registered by session-core upon load.
 */
hcfg_info_t plugin_keyinfo[] = {
    { CFGKEY_INIT_POINT, NULL, "Initial point begin testing from." },
    { NULL }
};

hpoint_t best = HPOINT_INITIALIZER;
double   best_perf;

/* Forward function definitions. */
int  config_strategy(void);
void randomize(hpoint_t* point);

/* Variables to track current search state. */
hspace_t* space;
hpoint_t  next;

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
        next.id = 1;
    }

    /* Initialization for subsequent calls to strategy_init(). */
    if (space != space_ptr) {
        if (hpoint_init(&next, space_ptr->len) != 0) {
            session_error("Could not initialize point structure");
            return -1;
        }
        space = space_ptr;
    }

    if (config_strategy() != 0)
        return -1;

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
    if (hpoint_copy(point, &next) != 0) {
        session_error("Could not copy point during generation.");
        return -1;
    }

    /* Prepare a new random vertex for the next call to strategy_generate. */
    randomize(&next);
    ++next.id;

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
            session_error("Internal error: Could not copy point.");
            return -1;
        }
    }
    else {
        randomize(point);
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
    return 0;
}

/*
 * Return the best performing point thus far in the search.
 */
int strategy_best(hpoint_t* point)
{
    if (hpoint_copy(point, &best) != 0) {
        session_error("Internal error: Could not copy point.");
        return -1;
    }
    return 0;
}

//
// Internal helper function implementation.
//
int config_strategy(void)
{
    const char* cfgval = hcfg_get(session_cfg, CFGKEY_INIT_POINT);
    if (cfgval) {
        if (hpoint_parse(&next, cfgval, space) != 0) {
            session_error("Error parsing point from " CFGKEY_INIT_POINT ".");
            return -1;
        }

        if (hpoint_align(&next, space) != 0) {
            session_error("Could not align initial point to search space");
            return -1;
        }
    }
    else {
        randomize(&next);
    }
    return 0;
}

void randomize(hpoint_t* point)
{
    for (int i = 0; i < space->len; ++i)
        point->term[i] = hrange_random(&space->dim[i]);
}
