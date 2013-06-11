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

#include "strategy.h"
#include "session-core.h"
#include "hutil.h"
#include "hcfg.h"
#include "defaults.h"
#include "libvertex.h"

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
vertex_t *curr;
int remaining_passes;

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

    best = HPOINT_INITIALIZER;
    best_perf = INFINITY;

    curr = vertex_alloc();
    if (!curr) {
        session_error("Could not allocate memory for testing vertex.");
        return -1;
    }
    vertex_min(curr);
    curr->id = 1;

    if (session_inform(CFGKEY_STRATEGY_CONVERGED, "0") != 0) {
        session_error("Could not set "
                      CFGKEY_STRATEGY_CONVERGED " config variable.");
        return -1;
    }
    return 0;
}

int strategy_cfg(void)
{
    const char *cfgstr;

    cfgstr = session_query(CFGKEY_BRUTE_PASSES);
    if (cfgstr)
        remaining_passes = atoi(cfgstr);

    return 0;
}

/*
 * Generate a new candidate configuration.
 */
int strategy_generate(hflow_t *flow, hpoint_t *point)
{
    if (curr->id > 0) {
        if (vertex_to_hpoint(curr, point) != 0) {
            session_error("Internal error: Could not make point from vertex.");
            return -1;
        }

        ++curr->id;
        if (increment() != 0)
            return -1;
    }
    else {
        if (hpoint_copy(point, &best) != 0) {
            session_error("Internal error: Could not copy point.");
            return -1;
        }
    }

    flow->status = HFLOW_ACCEPT;
    return 0;
}

/*
 * Regenerate a point deemed invalid by a later plug-in.
 */
int strategy_rejected(hpoint_t *point, hpoint_t *hint)
{
    if (hint && hint->id != -1) {
        int orig_id = point->id;

        if (hpoint_copy(point, hint) != 0) {
            session_error("Internal error: Could not copy point.");
            return -1;
        }
        point->id = orig_id;
    }
    else {
        if (vertex_to_hpoint(curr, point) != 0) {
            session_error("Internal error: Could not make point from vertex.");
            return -1;
        }

        if (increment() != 0)
            return -1;
    }

    return 0;
}

int increment(void)
{
    if (curr->id == 0)
        return 0;

    if (vertex_incr(curr)) {
        if (--remaining_passes < 0) {
            if (session_inform(CFGKEY_STRATEGY_CONVERGED, "1") != 0) {
                session_error("Internal error: Could not increment vertex.");
                return -1;
            }
            curr->id = 0;
        }
    }
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
