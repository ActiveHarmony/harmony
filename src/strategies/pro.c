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
 * \page pro Parallel Rank Order (pro.so)
 *
 * This search strategy uses a simplex-based method similar to the
 * Nelder-Mead algorithm.  It improves upon the Nelder-Mead algorithm
 * by allowing the simultaneous search of all simplex points at each
 * step of the algorithm.  As such, it is ideal for a parallel search
 * utilizing multiple nodes, for instance when integrated in OpenMP or
 * MPI programs.
 *
 * **Configuration Variables**
 * Key           | Type    | Default      | Description
 * --------------| ------- | ------------ | -----------
 * SIMPLEX_SIZE  | Integer | N+1          | Number of vertices in the simplex.  Defaults to the number of tuning variables + 1.
 * RANDOM_SEED   | Integer | time(NULL)   | Value to seed the pseudo-random number generator.  Default is to seed the random generator by time.
 * INIT_METHOD   | String  | point        | Initial simplex generation method.  Valid values are "point", "point_fast", and "random" (without quotes).
 * INIT_PERCENT  | Real    | 0.35         | Initial simplex size as a percentage of the total search space.  Only for "point" and "point_fast" initial simplex methods.
 * REJECT_METHOD | String  | penalty      | How to choose a replacement when dealing with rejected points:<br> **Penalty** - Use this method if the chance of point rejection is relatively low.  It applies an infinite penalty factor for invalid points, allowing the PRO algorithm to select a sensible next point.  However, if the entire simplex is comprised of invalid points, an infinite loop of invalid points may occur.<br> **Random** - Use this method if the chance of point rejection is high.  It reduces the risk of infinitely selecting invalid points at the cost of increasing the risk of deforming the simplex.
 * REFLECT       | Real    | 1.0          | Multiplicative coefficient for simplex reflection step.
 * EXPAND        | Real    | 2.0          | Multiplicative coefficient for simplex expansion step.
 * SHRINK        | Real    | 0.5          | Multiplicative coefficient for simplex shrink step.
 * FVAL_TOL      | Real    | 0.0001       | Convergence test succeeds if difference between all vertex performance values fall below this value.
 * SIZE_TOL      | Real    | 0.05*r       | Convergence test succeeds if simplex size falls below this value.  Default is 5% of the initial simplex radius.
 */

#include "strategy.h"
#include "session-core.h"
#include "hsession.h"
#include "hutil.h"
#include "hcfg.h"
#include "defaults.h"
#include "libvertex.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <time.h>
#include <math.h>

hpoint_t best;
double   best_perf;

hpoint_t curr;

typedef enum simplex_init {
    SIMPLEX_INIT_UNKNOWN = 0,
    SIMPLEX_INIT_RANDOM,
    SIMPLEX_INIT_POINT,
    SIMPLEX_INIT_POINT_FAST,

    SIMPLEX_INIT_MAX
} simplex_init_t;

typedef enum reject_method {
    REJECT_METHOD_UNKNOWN = 0,
    REJECT_METHOD_PENALTY,
    REJECT_METHOD_RANDOM,

    REJECT_METHOD_MAX
} reject_method_t;

typedef enum simplex_state {
    SIMPLEX_STATE_UNKNONW = 0,
    SIMPLEX_STATE_INIT,
    SIMPLEX_STATE_REFLECT,
    SIMPLEX_STATE_EXPAND_ONE,
    SIMPLEX_STATE_EXPAND_ALL,
    SIMPLEX_STATE_SHRINK,
    SIMPLEX_STATE_CONVERGED,

    SIMPLEX_STATE_MAX
} simplex_state_t;

/* Forward function definitions. */
int  strategy_cfg(hsignature_t *sig);
int  init_by_random(void);
int  init_by_point(int fast);
int  pro_algorithm(void);
int  pro_next_state(const simplex_t *input, int best_in);
int  pro_next_simplex(simplex_t *output);
void check_convergence(void);

/* Variables to control search properties. */
simplex_init_t  init_method  = SIMPLEX_INIT_POINT;
vertex_t *      init_point;
double          init_percent = 0.35;
reject_method_t reject_type  = REJECT_METHOD_PENALTY;

double reflect  = 1.0;
double expand   = 2.0;
double contract = 0.5;
double shrink   = 0.5;
double fval_tol = 1e-4;
double size_tol;
int simplex_size;

/* Variables to track current search state. */
simplex_state_t state;
simplex_t *base;
simplex_t *test;

int best_base;
int best_test;
int best_stash;
int next_id;
int send_idx;
int reported;

/*
 * Invoked once on strategy load.
 */
int strategy_init(hsignature_t *sig)
{
    if (libvertex_init(sig) != 0) {
        session_error("Could not initialize vertex library.");
        return -1;
    }

    init_point = vertex_alloc();
    if (!init_point) {
        session_error("Could not allocate memory for initial point.");
        return -1;
    }

    if (strategy_cfg(sig) != 0)
        return -1;

    best = HPOINT_INITIALIZER;
    best_perf = INFINITY;

    if (hpoint_init(&curr, sig->range_len) < 0)
        return -1;

    test = simplex_alloc(simplex_size);
    if (!test) {
        session_error("Could not allocate memory for candidate simplex.");
        return -1;
    }

    base = simplex_alloc(simplex_size);
    if (!base) {
        session_error("Could not allocate memory for reference simplex.");
        return -1;
    }

    /* Default stopping criteria: 0.5% of dist(vertex_min, vertex_max). */
    if (size_tol == 0.0) {
        vertex_min(base->vertex[0]);
        vertex_max(base->vertex[1]);
        size_tol = vertex_dist(base->vertex[0], base->vertex[1]) * 0.005;
    }

    switch (init_method) {
    case SIMPLEX_INIT_RANDOM:     init_by_random(); break;
    case SIMPLEX_INIT_POINT:      init_by_point(0); break;
    case SIMPLEX_INIT_POINT_FAST: init_by_point(1); break;
    default:
        session_error("Invalid initial search method.");
        return -1;
    }

    next_id = 1;
    state = SIMPLEX_STATE_INIT;

    if (session_setcfg(CFGKEY_STRATEGY_CONVERGED, "0") != 0) {
        session_error("Could not set "
                      CFGKEY_STRATEGY_CONVERGED " config variable.");
        return -1;
    }

    if (pro_next_simplex(test) != 0) {
        session_error("Could not initiate the simplex.");
        return -1;
    }

    return 0;
}

int strategy_cfg(hsignature_t *sig)
{
    const char *cfgval;
    char *endp;

    /* Make sure the simplex size is N+1 or greater. */
    cfgval = session_getcfg(CFGKEY_SIMPLEX_SIZE);
    if (cfgval)
        simplex_size = atoi(cfgval);

    if (simplex_size < sig->range_len + 1)
        simplex_size = sig->range_len * 2;

    cfgval = session_getcfg(CFGKEY_RANDOM_SEED);
    if (cfgval && *cfgval) {
        srand(atoi(cfgval));
    }
    else {
        srand(time(NULL));
    }

    cfgval = session_getcfg(CFGKEY_INIT_METHOD);
    if (cfgval) {
        if (strcasecmp(cfgval, "random") == 0) {
            init_method = SIMPLEX_INIT_RANDOM;
        }
        else if (strcasecmp(cfgval, "point") == 0) {
            init_method = SIMPLEX_INIT_POINT;
        }
        else if (strcasecmp(cfgval, "point_fast") == 0) {
            init_method = SIMPLEX_INIT_POINT_FAST;
        }
        else {
            session_error("Invalid value for "
                          CFGKEY_INIT_METHOD " configuration key.");
            return -1;
        }
    }

    cfgval = session_getcfg(CFGKEY_INIT_PERCENT);
    if (cfgval) {
        init_percent = strtod(cfgval, &endp);
        if (*endp != '\0') {
            session_error("Invalid value for " CFGKEY_INIT_PERCENT
                          " configuration key.");
            return -1;
        }
        if (init_percent <= 0 || init_percent > 1) {
            session_error("Configuration key " CFGKEY_INIT_PERCENT
                          " must be between 0.0 and 1.0 (exclusive).");
            return -1;
        }
    }

    cfgval = session_getcfg(CFGKEY_REJECT_METHOD);
    if (cfgval) {
        if (strcasecmp(cfgval, "penalty") == 0) {
            reject_type = REJECT_METHOD_PENALTY;
        }
        else if (strcasecmp(cfgval, "random") == 0) {
            reject_type = REJECT_METHOD_RANDOM;
        }
        else {
            session_error("Invalid value for "
                          CFGKEY_REJECT_METHOD " configuration key.");
            return -1;
        }
    }

    cfgval = session_getcfg(CFGKEY_REFLECT);
    if (cfgval) {
        reflect = strtod(cfgval, &endp);
        if (*endp != '\0') {
            session_error("Invalid value for " CFGKEY_REFLECT
                          " configuration key.");
            return -1;
        }
        if (reflect <= 0) {
            session_error("Configuration key " CFGKEY_REFLECT
                          " must be positive.");
            return -1;
        }
    }

    cfgval = session_getcfg(CFGKEY_EXPAND);
    if (cfgval) {
        expand = strtod(cfgval, &endp);
        if (*endp != '\0') {
            session_error("Invalid value for " CFGKEY_EXPAND
                          " configuration key.");
            return -1;
        }
        if (reflect <= 1) {
            session_error("Configuration key " CFGKEY_EXPAND
                          " must be greater than 1.0.");
            return -1;
        }
    }

    cfgval = session_getcfg(CFGKEY_CONTRACT);
    if (cfgval) {
        contract = strtod(cfgval, &endp);
        if (*endp != '\0') {
            session_error("Invalid value for " CFGKEY_CONTRACT
                          " configuration key.");
            return -1;
        }
        if (reflect <= 0 || reflect >= 1) {
            session_error("Configuration key " CFGKEY_CONTRACT
                          " must be between 0.0 and 1.0 (exclusive).");
            return -1;
        }
    }

    cfgval = session_getcfg(CFGKEY_SHRINK);
    if (cfgval) {
        shrink = strtod(cfgval, &endp);
        if (*endp != '\0') {
            session_error("Invalid value for " CFGKEY_SHRINK
                          " configuration key.");
            return -1;
        }
        if (reflect <= 0 || reflect >= 1) {
            session_error("Configuration key " CFGKEY_SHRINK
                          " must be between 0.0 and 1.0 (exclusive).");
            return -1;
        }
    }

    cfgval = session_getcfg(CFGKEY_FVAL_TOL);
    if (cfgval) {
        fval_tol = strtod(cfgval, &endp);
        if (*endp != '\0') {
            session_error("Invalid value for " CFGKEY_FVAL_TOL
                          " configuration key.");
            return -1;
        }
    }

    cfgval = session_getcfg(CFGKEY_SIZE_TOL);
    if (cfgval) {
        size_tol = strtod(cfgval, &endp);
        if (*endp != '\0') {
            session_error("Invalid value for " CFGKEY_SIZE_TOL
                          " configuration key.");
            return -1;
        }
    }
    return 0;
}

int init_by_random(void)
{
    int i;

    for (i = 0; i < simplex_size; ++i)
        vertex_rand(base->vertex[i]);

    return 0;
}

int init_by_point(int fast)
{
    if (init_point->id == 0)
        vertex_center(init_point);

    if (fast)
        return simplex_from_vertex_fast(init_point, init_percent, base);

    return simplex_from_vertex(init_point, init_percent, base);
}

/*
 * Generate a new candidate configuration point.
 */
int strategy_generate(hflow_t *flow, hpoint_t *point)
{
    if (send_idx == simplex_size) {
        flow->status = HFLOW_WAIT;
        return 0;
    }

    test->vertex[send_idx]->id = next_id;
    if (vertex_to_hpoint(test->vertex[send_idx], point) != 0) {
        session_error( strerror(errno) );
        return -1;
    }
    ++next_id;
    ++send_idx;

    flow->status = HFLOW_ACCEPT;
    return 0;
}

/*
 * Regenerate a point deemed invalid by a later plug-in.
 */
int strategy_rejected(hflow_t *flow, hpoint_t *point)
{
    int i;
    hpoint_t *hint = &flow->point;

    /* Find the rejected vertex. */
    for (i = 0; i < simplex_size; ++i) {
        if (test->vertex[i]->id == point->id)
            break;
    }
    if (i == simplex_size) {
        session_error("Internal error: Could not find rejected point.");
        return -1;
    }

    if (hint && hint->id != -1) {
        int orig_id = point->id;

        /* Update our state to include the hint point. */
        if (vertex_from_hpoint(hint, test->vertex[i]) != 0) {
            session_error("Internal error: Could not make vertex from point.");
            return -1;
        }

        if (hpoint_copy(point, hint) != 0) {
            session_error("Internal error: Could not copy point.");
            return -1;
        }
        point->id = orig_id;
    }
    else {
        if (reject_type == REJECT_METHOD_PENALTY) {
            /* Apply an infinite penalty to the invalid point and
             * allow the algorithm to determine the next point to try.
             */
            test->vertex[i]->perf = INFINITY;
            ++reported;

            if (reported == simplex_size) {
                if (pro_algorithm() != 0) {
                    session_error("Internal error: PRO algorithm failure.");
                    return -1;
                }
                reported = 0;
                send_idx = 0;
                i = 0;
            }

            if (send_idx == simplex_size) {
                flow->status = HFLOW_WAIT;
                return 0;
            }

            test->vertex[send_idx]->id = next_id;
            if (vertex_to_hpoint(test->vertex[send_idx], point) != 0) {
                session_error("Internal error: Could not make point"
                              " from vertex.");
                return -1;
            }
            ++next_id;
            ++send_idx;
        }
        else if (reject_type == REJECT_METHOD_RANDOM) {
            /* Replace the rejected point with a random point. */
            vertex_rand(test->vertex[i]);
            if (vertex_to_hpoint(test->vertex[i], point) != 0) {
                session_error("Internal error: Could not make point"
                              " from vertex.");
                return -1;
            }
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
    int i;

    for (i = 0; i < simplex_size; ++i) {
        if (test->vertex[i]->id == trial->point.id)
            break;
    }
    if (i == simplex_size) {
        /* Ignore rouge vertex reports. */
        return 0;
    }

    ++reported;
    test->vertex[i]->perf = trial->perf;
    if (test->vertex[i]->perf < test->vertex[best_test]->perf)
        best_test = i;

    if (reported == simplex_size) {
        if (pro_algorithm() != 0) {
            session_error("Internal error: PRO algorithm failure.");
            return -1;
        }
        reported = 0;
        send_idx = 0;
    }

    /* Update the best performing point, if necessary. */
    if (best_perf > trial->perf) {
        best_perf = trial->perf;
        if (hpoint_copy(&best, &trial->point) != 0) {
            session_error( strerror(errno) );
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

int pro_algorithm(void)
{
    do {
        if (state == SIMPLEX_STATE_CONVERGED)
            break;

        if (pro_next_state(test, best_test) != 0)
            return -1;

        if (state == SIMPLEX_STATE_REFLECT)
            check_convergence();

        if (pro_next_simplex(test) != 0)
            return -1;

    } while (simplex_outofbounds(test));

    return 0;
}

int pro_next_state(const simplex_t *input, int best_in)
{
    switch (state) {
    case SIMPLEX_STATE_INIT:
    case SIMPLEX_STATE_SHRINK:
        /* Simply accept the candidate simplex and prepare to reflect. */
        simplex_copy(base, input);
        best_base = best_in;
        state = SIMPLEX_STATE_REFLECT;
        break;

    case SIMPLEX_STATE_REFLECT:
        if (input->vertex[best_in]->perf < base->vertex[best_base]->perf) {
            /* Reflected simplex has best known performance.
             * Accept the reflected simplex, and prepare a trial expansion.
             */
            simplex_copy(base, input);
            best_stash = best_test;
            state = SIMPLEX_STATE_EXPAND_ONE;
        }
        else {
            /* Reflected simplex does not improve performance.
             * Shrink the simplex instead.
             */
            state = SIMPLEX_STATE_SHRINK;
        }
        break;

    case SIMPLEX_STATE_EXPAND_ONE:
        if (input->vertex[0]->perf < base->vertex[best_base]->perf) {
            /* Trial expansion has found the best known vertex thus far.
             * We are now free to expand the entire reflected simplex.
             */
            state = SIMPLEX_STATE_EXPAND_ALL;
        }
        else {
            /* Expanded vertex does not improve performance.
             * Revert to the (unexpanded) reflected simplex.
             */
            best_base = best_in;
            state = SIMPLEX_STATE_REFLECT;
        }
        break;

    case SIMPLEX_STATE_EXPAND_ALL:
        if (input->vertex[best_in]->perf < base->vertex[best_base]->perf) {
            /* Expanded simplex has found the best known vertex thus far.
             * Accept the expanded simplex as the reference simplex. */
            simplex_copy(base, input);
            best_base = best_in;
        }

        /* Expanded simplex may not improve performance over the
         * reference simplex.  In general, this can only happen if the
         * entire expanded simplex is out of bounds.
         *
         * Either way, reflection should be tested next. */
        state = SIMPLEX_STATE_REFLECT;
        break;

    default:
        return -1;
    }
    return 0;
}

int pro_next_simplex(simplex_t *output)
{
    int i;

    switch (state) {
    case SIMPLEX_STATE_INIT:
        /* Bootstrap the process by testing the reference simplex. */
        simplex_copy(output, base);
        break;

    case SIMPLEX_STATE_REFLECT:
        /* Reflect all original simplex vertices around the best known
         * vertex thus far. */
        simplex_transform(base, base->vertex[best_base], -reflect, output);
        break;

    case SIMPLEX_STATE_EXPAND_ONE:
        /* Next simplex should have one vertex extending the best.
         * And the rest should be copies of the best known vertex.
         */
        vertex_transform(test->vertex[best_test], base->vertex[best_base],
                         expand, output->vertex[0]);

        for (i = 1; i < simplex_size; ++i)
            vertex_copy(output->vertex[i], base->vertex[best_base]);
        break;

    case SIMPLEX_STATE_EXPAND_ALL:
        /* Expand all original simplex vertices away from the best
         * known vertex thus far. */
        simplex_transform(base, base->vertex[best_base], expand, output);
        break;

    case SIMPLEX_STATE_SHRINK:
        /* Shrink all original simplex vertices towards the best
         * known vertex thus far. */
        simplex_transform(base, base->vertex[best_base], shrink, output);
        break;

    case SIMPLEX_STATE_CONVERGED:
        /* Simplex has converged.  Nothing to do.
         * In the future, we may consider new search at this point. */
        break;

    default:
        return -1;
    }
    return 0;
}

void check_convergence(void)
{
    int i;
    double fval_err, size_max, dist;
    static vertex_t *centroid;

    if (!centroid)
        centroid = vertex_alloc();

    if (simplex_collapsed(base) == 1)
        goto converged;

    simplex_centroid(base, centroid);

    fval_err = 0;
    for (i = 0; i < simplex_size; ++i) {
        fval_err += ((base->vertex[i]->perf - centroid->perf) *
                     (base->vertex[i]->perf - centroid->perf));
    }
    fval_err /= simplex_size;

    size_max = 0;
    for (i = 0; i < simplex_size; ++i) {
        dist = vertex_dist(base->vertex[i], centroid);
        if (size_max < dist)
            size_max = dist;
    }

    if (fval_err < fval_tol && size_max < size_tol)
        goto converged;

    return;

  converged:
    state = SIMPLEX_STATE_CONVERGED;
    session_setcfg(CFGKEY_STRATEGY_CONVERGED, "1");
}
