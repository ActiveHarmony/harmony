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
 * \page random Random (random.so)
 *
 * This search strategy generates random points within the search
 * space.  Using* a pseudo-random method, a value is selected for each
 * tuning variable according to its defined bounds.  This search will
 * never reach a converged state.
 *
 * It is mainly used as a basis of comparison for more intelligent
 * search strategies.
 *
 * **Configuration Variables**
 * Key          | Type       | Default | Description
 * ------------ | ---------- | ------- | -----------
 * RANDOM_SEED  | Integer    | time()  | Value to seed the pseudo-random number generator.  Default is to seed the random generator by time.
 */

#include "strategy.h"
#include "session-core.h"
#include "hsignature.h"
#include "hutil.h"
#include "hcfg.h"
#include "defaults.h"
#include "libvertex.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>

hpoint_t best;
double   best_perf;

/* Forward function definitions. */
int strategy_cfg(void);

/* Variables to track current search state. */
vertex_t *curr;

/*
 * Invoked once on strategy load.
 */
int strategy_init(hsignature_t *sig)
{
    if (libvertex_init(sig) != 0) {
        session_error("Could not initialize vertex library.");
        return -1;
    }

    if (strategy_cfg() != 0)
        return -1;

    if (!curr) {
        /* One time memory allocation and/or initialization. */
        curr = vertex_alloc();
        if (!curr) {
            session_error("Could not allocate memory for testing vertex.");
            return -1;
        }
        curr->id = 1;

        best = HPOINT_INITIALIZER;
    }

    /* Initialization for subsequent calls to strategy_init(). */
    best_perf = INFINITY;

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

    cfgstr = session_getcfg(CFGKEY_RANDOM_SEED);
    if (cfgstr && *cfgstr) {
        srand(atoi(cfgstr));
    }
    else {
        srand(time(NULL));
    }

    return 0;
}

/*
 * Generate a new candidate configuration.
 */
int strategy_generate(hflow_t *flow, hpoint_t *point)
{
    vertex_rand(curr);
    if (vertex_to_hpoint(curr, point) != 0) {
        session_error("Internal error: Could not make point from vertex.");
        return -1;
    }
    ++curr->id;

    flow->status = HFLOW_ACCEPT;
    return 0;
}

/*
 * Regenerate a point deemed invalid by a later plug-in.
 */
int strategy_rejected(hflow_t *flow, hpoint_t *point)
{
    hpoint_t *hint = &flow->point;

    if (hint && hint->id != -1) {
        int orig_id = point->id;

        if (hpoint_copy(point, hint) != 0) {
            session_error("Internal error: Could not copy point.");
            return -1;
        }
        point->id = orig_id;
    }
    else {
        vertex_rand(curr);
        if (vertex_to_hpoint(curr, point) != 0) {
            session_error("Internal error: Could not make point from vertex.");
            return -1;
        }
    }

    flow->status = HFLOW_ACCEPT;
    return 0;
}

/*
 * Analyze the observed performance for this configuration point.
 */
int strategy_analyze(htrial_t *trial)
{
    if (best_perf > trial->perf) {
        best_perf = trial->perf;
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
int strategy_best(hpoint_t *point)
{
    if (hpoint_copy(point, &best) != 0) {
        session_error("Internal error: Could not copy point.");
        return -1;
    }
    return 0;
}
