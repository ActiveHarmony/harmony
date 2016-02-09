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
 * > Nelder, John A.; R. Mead (1965). "A simplex method for function
 * >  minimization". Computer Journal 7: 308–313. doi:10.1093/comjnl/7.4.308
 */

#include "strategy.h"
#include "session-core.h"
#include "hcfg.h"
#include "hspace.h"
#include "hpoint.h"
#include "hperf.h"
#include "hutil.h"
#include "libvertex.h"

#include <string.h> // For strcmp().
#include <math.h>   // For isnan().

/*
 * Configuration variables used in this plugin.
 * These will automatically be registered by session-core upon load.
 */
const hcfg_info_t plugin_keyinfo[] = {
    { CFGKEY_INIT_POINT, NULL,
      "Centroid point used to initialize the search simplex.  If this key "
      "is left undefined, the simplex will be initialized in the center of "
      "the search space." },
    { CFGKEY_INIT_RADIUS, "0.50",
      "Size of the initial simplex, specified as a fraction of the total "
      "search space radius." },
    { CFGKEY_REJECT_METHOD, "penalty",
      "How to choose a replacement when dealing with rejected points. "
      "    penalty: Use this method if the chance of point rejection is "
      "relatively low. It applies an infinite penalty factor for invalid "
      "points, allowing the strategy to select a sensible next point.  "
      "However, if the entire simplex is comprised of invalid points, an "
      "infinite loop of rejected points may occur.\n"
      "    random: Use this method if the chance of point rejection is "
      "high.  It reduces the risk of infinitely selecting invalid points "
      "at the cost of increasing the risk of deforming the simplex." },
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

hpoint_t best_point;
hperf_t  best_perf;

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

/*
 * Internal helper function prototypes.
 */
static void check_convergence(void);
static int  config_strategy(void);
static int  nm_algorithm(void);
static int  nm_state_transition(void);
static int  nm_next_vertex(void);
static int  update_centroid(void);

/*
 * Variables to control search properties.
 */
vertex_t        init_point;
double          init_radius;
reject_method_t reject_type;

double reflect_val;
double expand_val;
double contract_val;
double shrink_val;
double fval_tol;
double size_tol;

/*
 * Variables to track current search state.
 */
hspace_t*       space;
vertex_t        centroid;
vertex_t        reflect;
vertex_t        expand;
vertex_t        contract;
simplex_t       simplex;
simplex_state_t state;

vertex_t* next;
int index_best;
int index_worst;
int index_curr; // For INIT or SHRINK.
int next_id;

/*
 * Invoked once on strategy load.
 */
int strategy_init(hspace_t* space_ptr)
{
    if (!space) {
        // One time memory allocation and/or initialization.
        next_id = 1;
    }

    // Initialization for subsequent calls to strategy_init().
    space = space_ptr;
    if (config_strategy() != 0)
        return -1;

    if (simplex_set(&simplex, space, &init_point, init_radius) != 0) {
        session_error("Could not generate initial simplex");
        return -1;
    }

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

/*
 * Generate a new candidate configuration point.
 */
int strategy_generate(hflow_t* flow, hpoint_t* point)
{
    if (next->id == next_id) {
        flow->status = HFLOW_WAIT;
        return 0;
    }

    next->id = next_id;
    if (vertex_point(next, space, point) != 0) {
        session_error("Could not make point from vertex during generate");
        return -1;
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

    if (hint->id) {
        // Update our state to include the hint point.
        hint->id = point->id;
        if (vertex_set(next, space, hint) != 0) {
            session_error("Could not copy hint into simplex during reject");
            return -1;
        }

        if (hpoint_copy(point, hint) != 0) {
            session_error("Could not return hint during reject");
            return -1;
        }
    }
    else if (reject_type == REJECT_METHOD_PENALTY) {
        // Apply an infinite penalty to the invalid point and
        // allow the algorithm to determine the next point to try.
        //
        hperf_reset(&next->perf);
        if (nm_algorithm() != 0) {
            session_error("Nelder-Mead algorithm failure");
            return -1;
        }

        next->id = next_id;
        if (vertex_point(next, space, point) != 0) {
            session_error("Could not copy next point during reject");
            return -1;
        }
    }
    else if (reject_type == REJECT_METHOD_RANDOM) {
        // Replace the rejected point with a random point.
        if (vertex_random(next, space, 1.0) != 0) {
            session_error("Could not randomize point during reject");
            return -1;
        }

        next->id = next_id;
        if (vertex_point(next, space, point) != 0) {
            session_error("Could not copy random point during reject");
            return -1;
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
    if (trial->point.id != next->id)
        return 0;

    if (hperf_copy(&next->perf, &trial->perf) != 0) {
        session_error("Could not copy performance to vertex");
        return -1;
    }

    if (nm_algorithm() != 0) {
        session_error("Nelder-Mead algorithm failure");
        return -1;
    }

    // Update the best performing point, if necessary.
    if (hperf_cmp(&best_perf, &trial->perf) > 0) {
        if (hperf_copy(&best_perf, &trial->perf) != 0) {
            session_error("Could not store best performance");
            return -1;
        }

        if (hpoint_copy(&best_point, &trial->point) != 0) {
            session_error("Could not copy best point during analyze");
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
    if (hpoint_copy(point, &best_point) != 0) {
        session_error("Could not copy best point during strategy_best()");
        return -1;
    }
    return 0;
}

/*
 * Internal helper function implementations.
 */
void check_convergence(void)
{
    double fval_err, size_max;
    double avg_perf = hperf_unify(&centroid.perf);

    if (simplex_collapsed(&simplex, space))
        goto converged;

    fval_err = 0.0;
    for (int i = 0; i < simplex.len; ++i) {
        double point_perf = hperf_unify(&simplex.vertex[i].perf);
        fval_err += ((point_perf - avg_perf) * (point_perf - avg_perf));
    }
    fval_err /= simplex.len;

    size_max = 0.0;
    for (int i = 0; i < simplex.len; ++i) {
        double dist = vertex_norm(&simplex.vertex[i], &centroid,
                                  VERTEX_NORM_L2);
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

int config_strategy(void)
{
    const char* cfgval;

    cfgval = hcfg_get(session_cfg, CFGKEY_INIT_POINT);
    if (cfgval) {
        if (vertex_parse(&init_point, space, cfgval) != 0) {
            session_error("Could not convert initial point to vertex");
            return -1;
        }
    }
    else {
        if (vertex_center(&init_point, space) != 0) {
            session_error("Could not create central vertex");
            return -1;
        }
    }

    init_radius = hcfg_real(session_cfg, CFGKEY_INIT_RADIUS);
    if (isnan(init_radius) || init_radius <= 0 || init_radius > 1) {
        session_error("Configuration key " CFGKEY_INIT_RADIUS
                      " must be between 0.0 and 1.0 (exclusive).");
        return -1;
    }

    cfgval = hcfg_get(session_cfg, CFGKEY_REJECT_METHOD);
    if (cfgval) {
        if (strcmp(cfgval, "penalty") == 0) {
            reject_type = REJECT_METHOD_PENALTY;
        }
        else if (strcmp(cfgval, "random") == 0) {
            reject_type = REJECT_METHOD_RANDOM;
        }
        else {
            session_error("Invalid value for "
                          CFGKEY_REJECT_METHOD " configuration key.");
            return -1;
        }
    }

    reflect_val = hcfg_real(session_cfg, CFGKEY_REFLECT);
    if (isnan(reflect_val) || reflect_val <= 0.0) {
        session_error("Configuration key " CFGKEY_REFLECT
                      " must be positive.");
        return -1;
    }

    expand_val = hcfg_real(session_cfg, CFGKEY_EXPAND);
    if (isnan(expand_val) || expand_val <= reflect_val) {
        session_error("Configuration key " CFGKEY_EXPAND
                      " must be greater than the reflect coefficient.");
        return -1;
    }

    contract_val = hcfg_real(session_cfg, CFGKEY_CONTRACT);
    if (isnan(contract_val) || contract_val <= 0.0 || contract_val >= 1.0) {
        session_error("Configuration key " CFGKEY_CONTRACT
                      " must be between 0.0 and 1.0 (exclusive).");
        return -1;
    }

    shrink_val = hcfg_real(session_cfg, CFGKEY_SHRINK);
    if (isnan(shrink_val) || shrink_val <= 0.0 || shrink_val >= 1.0) {
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

    // Use the expand and reflect vertex variables as temporaries to
    // calculate the size tolerance.
    if (vertex_minimum(&expand, space) != 0 ||
        vertex_maximum(&reflect, space) != 0)
        return -1;

    size_tol *= vertex_norm(&expand, &reflect, VERTEX_NORM_L2);
    return 0;
}

int nm_algorithm(void)
{
    do {
        if (state == SIMPLEX_STATE_CONVERGED)
            break;

        if (nm_state_transition() != 0)
            return -1;

        if (state == SIMPLEX_STATE_REFLECT) {
            if (update_centroid() != 0)
                return -1;

            check_convergence();
        }

        if (nm_next_vertex() != 0)
            return -1;

    } while (!vertex_inbounds(next, space));

    return 0;
}

int nm_state_transition(void)
{
    switch (state) {
    case SIMPLEX_STATE_INIT:
    case SIMPLEX_STATE_SHRINK:
        // Simplex vertex performance value.
        if (++index_curr == space->len + 1)
            state = SIMPLEX_STATE_REFLECT;

        break;

    case SIMPLEX_STATE_REFLECT:
        if (hperf_cmp(&reflect.perf, &simplex.vertex[index_best].perf) < 0) {
            // Reflected point performs better than all simplex points.
            // Attempt expansion.
            //
            state = SIMPLEX_STATE_EXPAND;
        }
        else if (hperf_cmp(&reflect.perf,
                           &simplex.vertex[index_worst].perf) < 0)
        {
            // Reflected point performs better than worst simplex point.
            // Replace the worst simplex point with reflected point
            // and attempt reflection again.
            //
            if (vertex_copy(&simplex.vertex[index_worst], &reflect) != 0)
                return -1;
        }
        else {
            // Reflected point does not improve the current simplex.
            // Attempt contraction.
            //
            state = SIMPLEX_STATE_CONTRACT;
        }
        break;

    case SIMPLEX_STATE_EXPAND:
        if (hperf_cmp(&expand.perf, &reflect.perf) < 0) {
            // Expanded point performs even better than reflected point.
            // Replace the worst simplex point with the expanded point
            // and attempt reflection again.
            //
            if (vertex_copy(&simplex.vertex[index_worst], &expand) != 0)
                return -1;
        }
        else {
            // Expanded point did not result in improved performance.
            // Replace the worst simplex point with the original
            // reflected point and attempt reflection again.
            //
            if (vertex_copy(&simplex.vertex[index_worst], &reflect) != 0)
                return -1;
        }
        state = SIMPLEX_STATE_REFLECT;
        break;

    case SIMPLEX_STATE_CONTRACT:
        if (hperf_cmp(&contract.perf,
                      &simplex.vertex[index_worst].perf) < 0)
        {
            // Contracted point performs better than the worst simplex point.
            //
            // Replace the worst simplex point with contracted point
            // and attempt reflection.
            //
            if (vertex_copy(&simplex.vertex[index_worst], &contract) != 0)
                return -1;

            state = SIMPLEX_STATE_REFLECT;
        }
        else {
            // Contracted test vertex has worst known performance.
            // Shrink the entire simplex towards the best point.
            //
            index_curr = -1; // Indicates the beginning of SHRINK.
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
        // Test individual vertices of the initial simplex.
        next = &simplex.vertex[index_curr];
        break;

    case SIMPLEX_STATE_REFLECT:
        // Test a vertex reflected from the worst performing vertex
        // through the centroid point.
        //
        if (vertex_transform(&centroid, &simplex.vertex[index_worst],
                             reflect_val, &reflect) != 0)
            return -1;

        next = &reflect;
        break;

    case SIMPLEX_STATE_EXPAND:
        // Test a vertex that expands the reflected vertex even
        // further from the the centroid point.
        //
        if (vertex_transform(&centroid, &simplex.vertex[index_worst],
                             expand_val, &expand) != 0)
            return -1;

        next = &expand;
        break;

    case SIMPLEX_STATE_CONTRACT:
        // Test a vertex contracted from the worst performing vertex
        // towards the centroid point.
        //
        if (vertex_transform(&simplex.vertex[index_worst], &centroid,
                             -contract_val, &contract) != 0)
            return -1;

        next = &contract;
        break;

    case SIMPLEX_STATE_SHRINK:
        if (index_curr == -1) {
            // Shrink the entire simplex towards the best known vertex
            // thus far.
            //
            if (simplex_transform(&simplex, &simplex.vertex[index_best],
                                  -shrink_val, &simplex) != 0)
                return -1;

            index_curr = 0;
        }

        // Test individual vertices of the initial simplex.
        next = &simplex.vertex[index_curr];
        break;

    case SIMPLEX_STATE_CONVERGED:
        // Simplex has converged.  Nothing to do.
        // In the future, we may consider new search at this point.
        //
        next = &simplex.vertex[index_best];
        break;

    default:
        return -1;
    }

    hperf_reset(&next->perf);
    return 0;
}

int update_centroid(void)
{
    index_best = 0;
    index_worst = 0;

    for (int i = 1; i < simplex.len; ++i) {
        if (hperf_cmp(&simplex.vertex[i].perf,
                      &simplex.vertex[index_best].perf) < 0)
            index_best = i;

        if (hperf_cmp(&simplex.vertex[i].perf,
                      &simplex.vertex[index_worst].perf) > 0)
            index_worst = i;
    }

    unsigned stashed_id = simplex.vertex[index_worst].id;
    simplex.vertex[index_worst].id = 0;
    if (simplex_centroid(&simplex, &centroid) != 0)
        return -1;

    simplex.vertex[index_worst].id = stashed_id;
    return 0;
}
