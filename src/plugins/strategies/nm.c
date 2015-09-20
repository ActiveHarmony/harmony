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
 */

#include "strategy.h"
#include "session-core.h"
#include "hsig.h"
#include "hperf.h"
#include "hutil.h"
#include "hcfg.h"
#include "libvertex.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <assert.h>

/*
 * Configuration variables used in this plugin.
 * These will automatically be registered by session-core upon load.
 */
hcfg_info_t plugin_keyinfo[] = {
    { CFGKEY_SIMPLEX_SIZE, NULL,
      "Number of vertices in the simplex.  Defaults to the number of tuning "
      "variables + 1." },
    { CFGKEY_INIT_METHOD, "center",
      "Initial simplex generation method. Valid values are "
      "point, center, and random." },
    { CFGKEY_INIT_PERCENT, "0.35",
      "Initial simplex size as a percentage of the total search space. "
      "Only for point initial simplex methods." },
    { CFGKEY_REJECT_METHOD, "penalty",
      "How to choose a replacement when dealing with rejected points. "
      "Penalty: Use this method if the chance of point rejection is "
      "relatively low. It applies an infinite penalty factor for invalid "
      "points, allowing the Nelder-Mead algorithm to select a sensible "
      "next point.  However, if the entire simplex is comprised of invalid "
      "points, an infinite loop of invalid points may occur. Random: Use "
      "this method if the chance of point rejection is high.  It reduces "
      "the risk of infinitely selecting invalid points at the cost of "
      "increasing the risk of deforming the simplex." },
    { CFGKEY_REFLECT, "1.0",
      "Multiplicative coefficient for simplex reflection step." },
    { CFGKEY_EXPAND, "2.0",
      "Multiplicative coefficient for simplex expansion step." },
    { CFGKEY_CONTRACT, "0.5",
      "Multiplicative coefficient for simplex contraction step." },
    { CFGKEY_SHRINK, "0.5",
      "Multiplicative coefficient for simplex shrink step." },
    { CFGKEY_FVAL_TOL, "0.0001",
      "Convergence test succeeds if difference between all vertex "
      "performance values fall below this value." },
    { CFGKEY_SIZE_TOL, "0.005",
      "Convergence test succeeds if the simplex radius becomes smaller "
      "than this percentage of the total search space.  Simplex radius "
      "is measured from centroid to furthest vertex.  Total search space "
      "is measured from minimum to maximum point." },
    { NULL }
};

hpoint_t best = HPOINT_INITIALIZER;
double   best_perf;

typedef enum simplex_init {
    SIMPLEX_INIT_UNKNOWN = 0,
    SIMPLEX_INIT_CENTER,
    SIMPLEX_INIT_RANDOM,
    SIMPLEX_INIT_POINT,

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
int  strategy_cfg(hsig_t* sig);
int  init_by_random(void);
int  init_by_maxval(void);
int  init_by_point(int fast);
int  init_by_specified_point(const char* str);
void simplex_update_index(void);
void simplex_update_centroid(void);
int  nm_algorithm(void);
int  nm_state_transition(void);
int  nm_next_vertex(void);
void check_convergence(void);

/* Variables to control search properties. */
simplex_init_t  init_method;
vertex_t*       init_point;
double          init_percent = 0.35;
reject_method_t reject_type  = REJECT_METHOD_PENALTY;

double reflect;
double expand;
double contract;
double shrink;
double fval_tol;
double size_tol;
int simplex_size;

/* Variables to track current search state. */
simplex_state_t state;
vertex_t* centroid;
vertex_t* test;
simplex_t* base;
vertex_t* next;

int index_best;
int index_worst;
int index_curr; /* for INIT or SHRINK */
int next_id;
int coords; /* number of coordinates for each vertex, from sig->range */

/*
 * Invoked once on strategy load.
 */
int strategy_init(hsig_t* sig)
{
    if (libvertex_init(sig) != 0) {
        session_error("Could not initialize vertex library.");
        return -1;
    }

    if (!base) {
        /* One time memory allocation and/or initialization. */

        /* Make sure the simplex size is N+1 or greater. */
        simplex_size = hcfg_int(session_cfg, CFGKEY_SIMPLEX_SIZE);
        if (simplex_size < sig->range_len + 1)
            simplex_size = sig->range_len + 1;

        init_point = vertex_alloc();
        if (!init_point) {
            session_error("Could not allocate memory for initial point.");
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

        base = simplex_alloc(simplex_size);
        if (!base) {
            session_error("Could not allocate memory for base simplex.");
            return -1;
        }

        /* The best point and trial counter should only be initialized once,
         * and thus be retained across a restart.
         */
        best_perf = HUGE_VAL;
        next_id = 1;
    }

    /* Initialization for subsequent calls to strategy_init(). */
    vertex_reset(init_point);
    vertex_reset(centroid);
    vertex_reset(test);
    simplex_reset(base);
    coords = sig->range_len;

    if (strategy_cfg(sig) != 0)
        return -1;

    switch (init_method) {
    case SIMPLEX_INIT_CENTER:
        vertex_center(init_point);
        break;

    case SIMPLEX_INIT_RANDOM:
        vertex_rand_trim(init_point, init_percent);
        break;

    case SIMPLEX_INIT_POINT:
        vertex_from_string(hcfg_get(session_cfg, CFGKEY_INIT_POINT),
                           sig, init_point);
        break;

    default:
        session_error("Invalid initial search method.");
        return -1;
    }
    simplex_from_vertex(init_point, init_percent, base);

    if (session_setcfg(CFGKEY_CONVERGED, "0") != 0) {
        session_error("Could not set " CFGKEY_CONVERGED " config variable.");
        return -1;
    }

    index_curr = 0;
    state = SIMPLEX_STATE_INIT;
    if (nm_next_vertex() != 0) {
        session_error("Could not initiate test vertex.");
        return -1;
    }

    return 0;
}

int strategy_cfg(hsig_t* sig)
{
    const char* cfgval;

    init_method = SIMPLEX_INIT_CENTER; /* Default value. */
    cfgval = hcfg_get(session_cfg, CFGKEY_INIT_METHOD);
    if (cfgval) {
        if (strcasecmp(cfgval, "center") == 0) {
            init_method = SIMPLEX_INIT_CENTER;
        }
        else if (strcasecmp(cfgval, "random") == 0) {
            init_method = SIMPLEX_INIT_RANDOM;
        }
        else if (strcasecmp(cfgval, "point") == 0) {
            init_method = SIMPLEX_INIT_POINT;
        }
        else {
            session_error("Invalid value for "
                          CFGKEY_INIT_METHOD " configuration key.");
            return -1;
        }
    }

    /* CFGKEY_INIT_POINT will override CFGKEY_INIT_METHOD if necessary. */
    cfgval = hcfg_get(session_cfg, CFGKEY_INIT_POINT);
    if (cfgval) {
        init_method = SIMPLEX_INIT_POINT;
    }
    else if (init_method == SIMPLEX_INIT_POINT) {
        session_error("Initial point must be provided in configuration key "
                      CFGKEY_INIT_POINT " if " CFGKEY_INIT_METHOD
                      " is 'point'.");
        return -1;
    }

    init_percent = hcfg_real(session_cfg, CFGKEY_INIT_PERCENT);
    if (isnan(init_percent) || init_percent <= 0 || init_percent > 1) {
        session_error("Configuration key " CFGKEY_INIT_PERCENT
                      " must be between 0.0 and 1.0 (exclusive).");
        return -1;
    }

    cfgval = hcfg_get(session_cfg, CFGKEY_REJECT_METHOD);
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

    reflect = hcfg_real(session_cfg, CFGKEY_REFLECT);
    if (isnan(reflect) || reflect <= 0.0) {
        session_error("Configuration key " CFGKEY_REFLECT
                      " must be positive.");
        return -1;
    }

    expand = hcfg_real(session_cfg, CFGKEY_EXPAND);
    if (isnan(expand) || expand <= reflect) {
        session_error("Configuration key " CFGKEY_EXPAND
                      " must be greater than the reflect coefficient.");
        return -1;
    }

    contract = hcfg_real(session_cfg, CFGKEY_CONTRACT);
    if (isnan(contract) || contract <= 0.0 || contract >= 1.0) {
        session_error("Configuration key " CFGKEY_CONTRACT
                      " must be between 0.0 and 1.0 (exclusive).");
        return -1;
    }

    shrink = hcfg_real(session_cfg, CFGKEY_SHRINK);
    if (isnan(shrink) || shrink <= 0.0 || shrink >= 1.0) {
        session_error("Configuration key " CFGKEY_SHRINK
                      " must be between 0.0 and 1.0 (exclusive).");
        return -1;
    }

    fval_tol = hcfg_real(session_cfg, CFGKEY_FVAL_TOL);
    if (isnan(fval_tol)) {
        session_error("Configuration key " CFGKEY_FVAL_TOL " is invalid.");
        return -1;
    }

    size_tol = hcfg_real(session_cfg, CFGKEY_SIZE_TOL);
    if (isnan(size_tol) || size_tol <= 0.0 || size_tol >= 1.0) {
        session_error("Configuration key " CFGKEY_SIZE_TOL
                      " must be between 0.0 and 1.0 (exclusive).");
        return -1;
    }
    size_tol *= vertex_dist(vertex_min(), vertex_max());

    return 0;
}

/*
 * Generate a new candidate configuration point.
 */
int strategy_generate(hflow_t* flow, hpoint_t* point)
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
int strategy_rejected(hflow_t* flow, hpoint_t* point)
{
    hpoint_t* hint = &flow->point;

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
            hperf_reset(&next->perf);
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
int strategy_analyze(htrial_t* trial)
{
    double perf = hperf_unify(&trial->perf);

    if (trial->point.id != next->id) {
        return 0;
    }
    hperf_copy(&next->perf, &trial->perf);

    if (nm_algorithm() != 0) {
        session_error("Internal error: Nelder-Mead algorithm failure.");
        return -1;
    }

    /* Update the best performing point, if necessary. */
    if (best_perf > perf) {
        best_perf = perf;
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
int strategy_best(hpoint_t* point)
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

    } while (vertex_outofbounds(next));

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
        if (hperf_cmp(&test->perf, &base->vertex[index_best]->perf) < 0) {
            /* Reflected test vertex has best known performance.
             * Replace the worst performing simplex vertex with the
             * reflected test vertex, and attempt expansion.
             */
            vertex_copy(base->vertex[index_worst], test);
            state = SIMPLEX_STATE_EXPAND;
        }
        else if (hperf_cmp(&test->perf,
                           &base->vertex[index_worst]->perf) < 0)
        {
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
        if (hperf_cmp(&test->perf, &base->vertex[index_best]->perf) < 0) {
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
        if (hperf_cmp(&test->perf, &base->vertex[index_worst]->perf) < 0) {
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
        if (hperf_cmp(&base->vertex[i]->perf,
                      &base->vertex[index_best]->perf) < 0)
            index_best = i;
        if (hperf_cmp(&base->vertex[i]->perf,
                      &base->vertex[index_worst]->perf) > 0)
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
    double fval_err, size_max;
    double avg_perf = hperf_unify(&centroid->perf);

    if (simplex_collapsed(base) == 1)
        goto converged;

    fval_err = 0.0;
    for (i = 0; i < simplex_size; ++i) {
        double vert_perf = hperf_unify(&base->vertex[i]->perf);
        fval_err += ((vert_perf - avg_perf) * (vert_perf - avg_perf));
    }
    fval_err /= simplex_size;

    size_max = 0.0;
    for (i = 0; i < simplex_size; ++i) {
        double dist = vertex_dist(base->vertex[i], centroid);
        if (size_max < dist)
            size_max = dist;
    }

    if (fval_err < fval_tol && size_max < size_tol)
        goto converged;

    return;

  converged:
    state = SIMPLEX_STATE_CONVERGED;
    session_setcfg(CFGKEY_CONVERGED, "1");
}
