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
#include "hperf.h"
#include "hutil.h"
#include "hcfg.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

/*
 * Configuration variables used in this plugin.
 * These will automatically be registered by session-core upon load.
 */
hcfg_info_t plugin_keyinfo[] = {
    { CFGKEY_PASSES, "1",
      "Number of passes through the search space before the search "
      "is considered converged." },
    { CFGKEY_INIT_POINT, NULL,
      "Initial point begin testing from." },
    { NULL }
};

hpoint_t best = HPOINT_INITIALIZER;
double   best_perf;

/* Forward function definitions. */
int strategy_cfg(hspace_t* space);
int increment(void);

/* Variables to track current search state. */
int N;
hrange_t* dim;
hpoint_t curr;
unsigned long* idx;
int remaining_passes = 1;
int final_id = 0;
int outstanding_points = 0, final_point_received = 0;

/*
 * Invoked once on strategy load.
 */
int strategy_init(hspace_t* space)
{
    N = space->len;
    dim = space->dim;

    if (!idx) {
        /* One time memory allocation and/or initialization. */
        if (hpoint_init(&curr, N) != 0) {
            session_error("Could not allocate memory candidate point.");
            return -1;
        }

        idx = malloc(N * sizeof(unsigned long));
        if (!idx) {
            session_error("Could not allocate memory for index array.");
            return -1;
        }

        /* The best point and trial counter should only be initialized once,
         * and thus be retained across a restart.
         */
        best_perf = HUGE_VAL;
        curr.id = 1;
    }

    /* Initialization for subsequent calls to strategy_init(). */
    if (strategy_cfg(space) != 0)
        return -1;

    if (session_setcfg(CFGKEY_CONVERGED, "0") != 0) {
        session_error("Could not set " CFGKEY_CONVERGED " config variable.");
        return -1;
    }
    return 0;
}

int strategy_cfg(hspace_t* space)
{
    const char* cfgstr;
    int i;

    remaining_passes = hcfg_int(session_cfg, CFGKEY_PASSES);
    if (remaining_passes < 0) {
        session_error("Invalid value for " CFGKEY_PASSES ".");
        return -1;
    }

    cfgstr = hcfg_get(session_cfg, CFGKEY_INIT_POINT);
    if (cfgstr) {
        if (hpoint_parse(&curr, cfgstr, space) != 0) {
            session_error("Error parsing point from " CFGKEY_INIT_POINT ".");
            return -1;
        }

        if (hpoint_align(&curr, space) != 0) {
            session_error("Error aligning point to parameter space.");
            return -1;
        }

        for (i = 0; i < space->len; ++i)
            idx[i] = hrange_index(&space->dim[i], &curr.term[i]);
    }
    else {
        for (i = 0; i < N; ++i) {
            hval_t* v = &curr.term[i];

            v->type = dim[i].type;
            switch (dim[i].type) {
            case HVAL_INT:  v->value.i = dim[i].bounds.i.min; break;
            case HVAL_REAL: v->value.r = dim[i].bounds.r.min; break;
            case HVAL_STR:  v->value.s = dim[i].bounds.e.set[0]; break;
            default:
                session_error("Invalid range detected during strategy init.");
                return -1;
            }
        }
        memset(idx, 0, N * sizeof(unsigned long));
    }

    return 0;
}

/*
 * Generate a new candidate configuration.
 */
int strategy_generate(hflow_t* flow, hpoint_t* point)
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

    /* every time we send out a point that's before
       the final point, increment the numebr of points
       we're waiting for results from */
    if(! final_id || curr.id <= final_id)
      outstanding_points++;

    flow->status = HFLOW_ACCEPT;
    return 0;
}

/*
 * Regenerate a point deemed invalid by a later plug-in.
 */
int strategy_rejected(hflow_t* flow, hpoint_t* point)
{
    hpoint_t* hint = &flow->point;
    int orig_id = point->id;

    if (hint && hint->id) {
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

int increment(void)
{
    int i, n_overflows, next_i;
    double next_r;

    if (remaining_passes <= 0)
        return 0;

    for (i = 0; i < N; ++i) {
        ++idx[i];
        switch (dim[i].type) {
        case HVAL_INT:
            curr.term[i].value.i += dim[i].bounds.i.step;
            if (curr.term[i].value.i > dim[i].bounds.i.max) {
                curr.term[i].value.i = dim[i].bounds.i.min;
                idx[i] = 0;
                continue;  // Overflow detected.
            }
            break;

        case HVAL_REAL:
            next_r = (dim[i].bounds.r.step > 0.0)
                ? dim[i].bounds.r.min + (idx[i] * dim[i].bounds.r.step)
                : nextafter(curr.term[i].value.r, HUGE_VAL);

            if (next_r > dim[i].bounds.r.max ||
                next_r == curr.term[i].value.r)
            {
                curr.term[i].value.r = dim[i].bounds.r.min;
                idx[i] = 0;
                continue;  // Overflow detected.
            }
            else {
                curr.term[i].value.r = next_r;
            }
            break;

        case HVAL_STR:
            if (idx[i] >= dim[i].bounds.e.len) {
                curr.term[i].value.s = dim[i].bounds.e.set[0];
                idx[i] = 0;
                continue;  // Overflow detected.
            }
            curr.term[i].value.s = dim[i].bounds.e.set[ idx[i] ];
            break;

        default:
            session_error("Invalid range detected.");
            return -1;
        }

        // No overflow detected. Look ahead one point,
        // to figure out the final point id before it's generated
        for(i = 0, n_overflows = 0;i < N;i++) {
          switch (dim[i].type) {
            case HVAL_INT:
              next_i = curr.term[i].value.i + dim[i].bounds.i.step;
              if(next_i > dim[i].bounds.i.max)
                n_overflows++;
              break;
            case HVAL_REAL:
              next_r = (dim[i].bounds.r.step > 0.0)
                ? dim[i].bounds.r.min + (idx[i] * dim[i].bounds.r.step)
                : nextafter(curr.term[i].value.r, HUGE_VAL);
              if(next_r > dim[i].bounds.r.max ||
                 next_r == curr.term[i].value.r)
                n_overflows++;
              break;
            case HVAL_STR:
              if(idx[i] + 1 >= dim[i].bounds.e.len)
                n_overflows++;
              break;
            default:
              return -1;
          }
        }
        if(remaining_passes - 1 <= 0 && n_overflows >= N) {
          final_id = curr.id;
        }
        return 0;
    }

    // We only reach this point if all values overflowed.
    if (--remaining_passes <= 0)
        final_id = curr.id - 1;

    return 0;
}
