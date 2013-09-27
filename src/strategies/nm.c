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
 * \page nm Nelder-Mead (nm.so)
 *
 * This search strategy uses a simplex-based method to estimate the
 * relative slope of a search space without calculating gradients.  It
 * functions by evaluating the performance for each point of the
 * simplex, and systematically replacing the worst performing point
 * with a reflection, expansion, or contraction in relation to the
 * simplex centroid.  In some cases, the entire simplex may also be
 * shrunken.
 *
 * \note Due to the nature of the underlying algorithm, this strategy
 * is best suited for serial tuning tasks.  It often waits on a single
 * performance report before a new point may be generated.
 *
 * For details of the algorithm, see:
 * > Nelder, John A.; R. Mead (1965). "A simplex method for function minimization".
 * > Computer Journal 7: 308–313. doi:10.1093/comjnl/7.4.308
 *
 * **Configuration Variables**
 * Key           | Type    | Default      | Description
 * --------------| ------- | ------------ | -----------
 * SIMPLEX_SIZE  | Integer | N+1          | Number of vertices in the simplex.  Defaults to the number of tuning variables + 1.
 * RANDOM_SEED   | Integer | time(NULL)   | Value to seed the pseudo-random number generator.  Default is to seed the random generator by time.
 * INIT_METHOD   | String  | point        | Initial simplex generation method.  Valid values are "point", "point_fast", and "random" (without quotes).
 * INIT_PERCENT  | Real    | 0.35         | Initial simplex size as a percentage of the total search space.  Only for "point" and "point_fast" initial simplex methods.
 * REJECT_METHOD | String  | penalty      | How to choose a replacement when dealing with rejected points:<br> **Penalty** - Use this method if the chance of point rejection is relatively low.  It applies an infinite penalty factor for invalid points, allowing the Nelder-Mead algorithm to select a sensible next point.  However, if the entire simplex is comprised of invalid points, an infinite loop of invalid points may occur.<br> **Random** - Use this method if the chance of point rejection is high.  It reduces the risk of infinitely selecting invalid points at the cost of increasing the risk of deforming the simplex.
 * REFLECT       | Real    | 1.0          | Multiplicative coefficient for simplex reflection step.
 * EXPAND        | Real    | 2.0          | Multiplicative coefficient for simplex expansion step.
 * SHRINK        | Real    | 0.5          | Multiplicative coefficient for simplex shrink step.
 * FVAL_TOL      | Real    | 0.0001       | Convergence test succeeds if difference between all vertex performance values fall below this value.
 * SIZE_TOL      | Real    | 0.05*r       | Convergence test succeeds if simplex size falls below this value.  Default is 5% of the initial simplex radius.
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
    SIMPLEX_INIT_MAXVAL,
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
    SIMPLEX_STATE_EXPAND,
    SIMPLEX_STATE_CONTRACT,
    SIMPLEX_STATE_SHRINK,
    SIMPLEX_STATE_CONVERGED,

    SIMPLEX_STATE_MAX
} simplex_state_t;

/* Forward function definitions. */
int  strategy_cfg(hsignature_t *sig);
int  init_by_random(void);
int  init_by_maxval(void);
int  init_by_point(int fast);
void simplex_update_index(void);
void simplex_update_centroid(void);
int  nm_algorithm(void);
int  nm_state_transition(void);
int  nm_next_vertex(void);
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
vertex_t *centroid;
vertex_t *test;
simplex_t *base;
vertex_t *next;

int index_best;
int index_worst;
int index_curr; /* for INIT or SHRINK */
int next_id;

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

    if (hpoint_init(&curr, simplex_size) != 0) {
        session_error("Could not initialize point structure.");
        return -1;
    }

    centroid = vertex_alloc();
    if (!centroid) {
        session_error("Could not allocate memory for centroid vertex.");
        return -1;
    }

    test = vertex_alloc();
    if (!test) {
        session_error("Could not allocate memory for testing vertex.");
        return -1;
    }

    /* Default stopping criteria: 0.5% of dist(vertex_min, vertex_max). */
    if (size_tol == 0) {
        vertex_min(test);
        vertex_max(centroid);
        size_tol = vertex_dist(test, centroid) * 0.005;
    }

    base = simplex_alloc(simplex_size);
    if (!base) {
        session_error("Could not allocate memory for base simplex.");
        return -1;
    }

    switch (init_method) {
    case SIMPLEX_INIT_RANDOM:     init_by_random(); break;
    case SIMPLEX_INIT_MAXVAL:     init_by_maxval(); break;
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

    if (nm_next_vertex() != 0) {
        session_error("Could not initiate test vertex.");
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
        simplex_size = sig->range_len + 1;

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
        else if (strcasecmp(cfgval, "maxval") == 0) {
            init_method = SIMPLEX_INIT_MAXVAL;
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
            session_error("Invalid value for " CFGKEY_REFLECT
                          " configuration key.");
            return -1;
        }
    }

    cfgval = session_getcfg(CFGKEY_SIZE_TOL);
    if (cfgval) {
        size_tol = strtod(cfgval, &endp);
        if (*endp != '\0') {
            session_error("Invalid value for " CFGKEY_REFLECT
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

int init_by_maxval(void)
{
    int i;
    vertex_t *max;

    max = vertex_alloc();
    if (!max)
        return -1;

    vertex_max(max);
    vertex_min(base->vertex[0]);
    for (i = 1; i < simplex_size; ++i) {
        vertex_copy(base->vertex[i], base->vertex[i - 1]);
        base->vertex[i]->term[i - 1] = max->term[i - 1];
    }

    while (i < simplex_size)
        vertex_rand(base->vertex[i++]);

    free(max);
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
    if (next->id == next_id) {
        flow->status = HFLOW_WAIT;
        return 0;
    }
    else {
        next->id = next_id;
        if (vertex_to_hpoint(next, point) != 0) {
            session_error("Internal error: Could not make point from vertex.");
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

    if (hint && hint->id != -1) {
        int orig_id = point->id;

        /* Update our state to include the hint point. */
        if (vertex_from_hpoint(hint, next) != 0) {
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
            next->perf = INFINITY;
            if (nm_algorithm() != 0) {
                session_error("Internal error: Nelder-Mead"
                              " algorithm failure.");
                return -1;
            }

            next->id = next_id;
            if (vertex_to_hpoint(next, point) != 0) {
                session_error("Internal error: Could not make"
                              " point from vertex.");
                return -1;
            }
        }
        else if (reject_type == REJECT_METHOD_RANDOM) {
            /* Replace the rejected point with a random point. */
            vertex_rand(next);
            if (vertex_to_hpoint(next, point) != 0) {
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
    if (trial->point.id != next->id) {
        session_error("Rouge points not supported.");
        return -1;
    }
    next->perf = trial->perf;

    if (nm_algorithm() != 0) {
        session_error("Internal error: Nelder-Mead algorithm failure.");
        return -1;
    }

    /* Update the best performing point, if necessary. */
    if (best_perf > trial->perf) {
        best_perf = trial->perf;
        if (hpoint_copy(&best, &trial->point) != 0) {
            session_error( strerror(errno) );
            return -1;
        }
    }

    if (state != SIMPLEX_STATE_CONVERGED)
        ++next_id;

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

int nm_algorithm(void)
{
    do {
        if (state == SIMPLEX_STATE_CONVERGED)
            break;

        if (nm_state_transition() != 0)
            return -1;

        if (state == SIMPLEX_STATE_REFLECT)
            check_convergence();

        if (nm_next_vertex() != 0)
            return -1;

    } while (vertex_outofbounds(test));

    return 0;
}

int nm_state_transition(void)
{
    int prev_worst;

    switch (state) {
    case SIMPLEX_STATE_INIT:
    case SIMPLEX_STATE_SHRINK:
        /* Simplex vertex performance value. */
        if (++index_curr == simplex_size) {
            simplex_update_centroid();
            state = SIMPLEX_STATE_REFLECT;
        }
        break;

    case SIMPLEX_STATE_REFLECT:
        if (test->perf < base->vertex[index_best]->perf) {
            /* Reflected test vertex has best known performance.
             * Replace the worst performing simplex vertex with the
             * reflected test vertex, and attempt expansion.
             */
            vertex_copy(base->vertex[index_worst], test);
            state = SIMPLEX_STATE_EXPAND;
        }
        else if (test->perf < base->vertex[index_worst]->perf) {
            /* Reflected test vertex performs better than the worst
             * known simplex vertex.  Replace the worst performing
             * simplex vertex with the reflected test vertex.
             */
            vertex_copy(base->vertex[index_worst], test);

            prev_worst = index_worst;
            simplex_update_centroid();

            /* If the test vertex has become the new worst performing
             * point, attempt contraction.  Otherwise, try reflection.
             */
            if (prev_worst == index_worst)
                state = SIMPLEX_STATE_CONTRACT;
            else
                state = SIMPLEX_STATE_REFLECT;
        }
        else {
            /* Reflected test vertex has worst known performance.
             * Ignore it and attempt contraction.
             */
            simplex_update_centroid();
            state = SIMPLEX_STATE_CONTRACT;
        }
        break;

    case SIMPLEX_STATE_EXPAND:
        if (test->perf < base->vertex[index_best]->perf) {
            /* Expanded test vertex has best known performance.
             * Replace the worst performing simplex vertex with the
             * test vertex, and attempt expansion.
             */
            vertex_copy(base->vertex[index_worst], test);
        }
        simplex_update_centroid();
        state = SIMPLEX_STATE_REFLECT;
        break;

    case SIMPLEX_STATE_CONTRACT:
        if (test->perf < base->vertex[index_worst]->perf) {
            /* Contracted test vertex performs better than the worst
             * known simplex vertex.  Replace the worst performing
             * simplex vertex with the test vertex.
             */
            vertex_copy(base->vertex[index_worst], test);
            simplex_update_centroid();
            state = SIMPLEX_STATE_REFLECT;
        }
        else {
            /* Contracted test vertex has worst known performance.
             * Shrink the entire simplex towards the best point.
             */
            index_curr = -1; /* to notify the beginning of SHRINK */
            state = SIMPLEX_STATE_SHRINK;
        }
        break;

    default:
        return -1;
    }
    return 0;
}

int nm_next_vertex(void)
{
    switch (state) {
    case SIMPLEX_STATE_INIT:
        /* Test individual vertices of the initial simplex. */
        next = base->vertex[index_curr];
        break;

    case SIMPLEX_STATE_REFLECT:
        /* Test a vertex reflected from the worst performing vertex
         * through the centroid point. */
        vertex_transform(base->vertex[index_worst], centroid, -reflect, test);
        next = test;
        break;

    case SIMPLEX_STATE_EXPAND:
        /* Test a vertex that expands the reflected vertex even
         * further from the the centroid point. */
        vertex_transform(base->vertex[index_worst], centroid, expand, test);
        next = test;
        break;

    case SIMPLEX_STATE_CONTRACT:
        /* Test a vertex contracted from the worst performing vertex
         * towards the centroid point. */
        vertex_transform(base->vertex[index_worst], centroid, contract, test);
        next = test;
        break;

    case SIMPLEX_STATE_SHRINK:
        if (index_curr == -1) {
          /* Shrink the entire simplex towards the best known vertex
           * thus far. */
          simplex_transform(base, base->vertex[index_best], shrink, base);
          index_curr = 0;
          next = base->vertex[index_curr];
        } else {
          /* Test individual vertices of the initial simplex. */
          next = base->vertex[index_curr];
        }
        break;

    case SIMPLEX_STATE_CONVERGED:
        /* Simplex has converged.  Nothing to do.
         * In the future, we may consider new search at this point. */
        next = base->vertex[index_best];
        break;

    default:
        return -1;
    }
    return 0;
}

void simplex_update_index(void)
{
    int i;

    index_best = 0;
    index_worst = 0;
    for (i = 0; i < simplex_size; ++i) {
        if (base->vertex[i]->perf < base->vertex[index_best]->perf)
            index_best = i;
        if (base->vertex[i]->perf > base->vertex[index_worst]->perf)
            index_worst = i;
    }
}

void simplex_update_centroid(void)
{
    int stash;

    simplex_update_index();
    stash = base->vertex[index_worst]->id;
    base->vertex[index_worst]->id = -1;
    simplex_centroid(base, centroid);
    base->vertex[index_worst]->id = stash;
}

void check_convergence(void)
{
    int i;
    double fval_err, size_max, dist;

    if (simplex_collapsed(base) == 1)
        goto converged;

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
