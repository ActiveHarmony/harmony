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
 * \page pro Parallel Rank Order (pro.so)
 *
 * This search strategy uses a simplex-based method similar to the
 * Nelder-Mead algorithm.  It improves upon the Nelder-Mead algorithm
 * by allowing the simultaneous search of all simplex points at each
 * step of the algorithm.  As such, it is ideal for a parallel search
 * utilizing multiple nodes, for instance when integrated in OpenMP or
 * MPI programs.
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
      "Point used to initialize the search simplex.  If this key is left "
      "undefined, the simplex will be initialized in the center of the "
      "search space." },
    { CFGKEY_INIT_PERCENT, "0.50",
      "Percent of the regular simplex expanded from the initial point as a "
      "fraction of the search space percent." },
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

hpoint_t best;
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
    SIMPLEX_STATE_EXPAND_ONE,
    SIMPLEX_STATE_EXPAND_ALL,
    SIMPLEX_STATE_SHRINK,
    SIMPLEX_STATE_CONVERGED,

    SIMPLEX_STATE_MAX
} simplex_state_t;

/* Forward function definitions. */
static int config_strategy(void);
static int pro_algorithm(void);
static int pro_next_state(void);
static int pro_next_simplex(void);
static int check_convergence(void);

/* Variables to control search properties. */
vertex_t        init_point;
double          init_percent;
reject_method_t reject_type;

double reflect_val;
double expand_val;
double shrink_val;
double fval_tol;
double size_tol;

/* Variables to track current search state. */
hspace_t*       space;
simplex_t       simplex;
simplex_t       reflect;
simplex_t       expand;
vertex_t        centroid;
simplex_state_t state;

simplex_t* next;
int        best_base;
int        best_test;
int        best_reflect;
int        next_id;
int        send_idx;
int        reported;

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
    if (space != space_ptr) {
        if (simplex_init(&simplex, space_ptr->len) != 0) {
            session_error("Could not initialize base simplex");
            return -1;
        }

        if (simplex_init(&reflect, space_ptr->len) != 0) {
            session_error("Could not initialize reflection simplex");
            return -1;
        }

        if (simplex_init(&expand, space_ptr->len) != 0) {
            session_error("Could not initialize expansion simplex");
            return -1;
        }
        space = space_ptr;
    }

    if (config_strategy() != 0)
        return -1;

    if (simplex_set(&simplex, space, &init_point, init_percent) != 0) {
        session_error("Could not generate initial simplex");
        return -1;
    }

    if (session_setcfg(CFGKEY_CONVERGED, "0") != 0) {
        session_error("Could not set " CFGKEY_CONVERGED " config variable");
        return -1;
    }

    send_idx = 0;
    state = SIMPLEX_STATE_INIT;
    if (pro_next_simplex() != 0) {
        session_error("Could not initiate the simplex");
        return -1;
    }

    reported = 0;
    return 0;
}

/*
 * Generate a new candidate configuration point.
 */
int strategy_generate(hflow_t* flow, hpoint_t* point)
{
    if (send_idx > space->len || state == SIMPLEX_STATE_CONVERGED) {
        flow->status = HFLOW_WAIT;
        return 0;
    }

    next->vertex[send_idx].id = next_id;
    if (vertex_point(&next->vertex[send_idx], space, point) != 0) {
        session_error("Could not copy point during generate");
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
int strategy_rejected(hflow_t* flow, hpoint_t* point)
{
    int reject_idx;
    hpoint_t* hint = &flow->point;

    // Find the rejected vertex.
    for (reject_idx = 0; reject_idx <= space->len; ++reject_idx) {
        if (next->vertex[reject_idx].id == point->id)
            break;
    }
    if (reject_idx > space->len) {
        session_error("Could not find rejected point");
        return -1;
    }

    if (hint && hint->id) {
        // Update our state to include the hint point.
        if (vertex_set(&next->vertex[reject_idx], space, hint) != 0) {
            session_error("Could not copy hint into simplex during reject");
            return -1;
        }

        // Return the hint point.
        if (hpoint_copy(point, hint) != 0) {
            session_error("Could not return hint during reject");
            return -1;
        }
    }
    else {
        if (reject_type == REJECT_METHOD_PENALTY) {
            /* Apply an infinite penalty to the invalid point and
             * allow the algorithm to determine the next point to try.
             */
            hperf_reset(&next->vertex[reject_idx].perf);
            ++reported;

            if (reported > space->len) {
                if (pro_algorithm() != 0) {
                    session_error("PRO algorithm failure");
                    return -1;
                }
                reported = 0;
                send_idx = 0;
            }

            if (send_idx > space->len) {
                flow->status = HFLOW_WAIT;
                return 0;
            }

            next->vertex[send_idx].id = next_id;
            if (vertex_point(&next->vertex[send_idx], space, point) != 0) {
                session_error("Could not convert vertex during reject");
                return -1;
            }

            ++next_id;
            ++send_idx;
        }
        else if (reject_type == REJECT_METHOD_RANDOM) {
            /* Replace the rejected point with a random point. */
            if (vertex_random(&next->vertex[reject_idx], space, 1.0) != 0) {
                session_error("Could not make random point during reject");
                return -1;
            }

            if (vertex_point(&next->vertex[reject_idx], space, point) != 0) {
                session_error("Could not convert vertex during reject");
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
    int report_idx;
    for (report_idx = 0; report_idx <= space->len; ++report_idx) {
        if (next->vertex[report_idx].id == trial->point.id)
            break;
    }
    if (report_idx > space->len) {
        /* Ignore rouge vertex reports. */
        return 0;
    }

    ++reported;
    hperf_copy(&next->vertex[report_idx].perf, &trial->perf);
    if (hperf_cmp(&next->vertex[report_idx].perf,
                  &next->vertex[best_test].perf) < 0)
        best_test = report_idx;

    if (reported > space->len) {
        if (pro_algorithm() != 0) {
            session_error("PRO algorithm failure");
            return -1;
        }
        reported = 0;
        send_idx = 0;
    }

    /* Update the best performing point, if necessary. */
    if (hperf_cmp(&best_perf, &trial->perf) < 0) {
        if (hperf_copy(&best_perf, &trial->perf) != 0) {
            session_error("Could not store best performance during analyze");
            return -1;
        }

        if (hpoint_copy(&best, &trial->point) != 0) {
            session_error("Could not copy best point during analyze");
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
        session_error("Could not copy best point during strategy_best()");
        return -1;
    }
    return 0;
}

//
// Internal helper function implementation.
//
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

    init_percent = hcfg_real(session_cfg, CFGKEY_INIT_PERCENT);
    if (isnan(init_percent) || init_percent <= 0 || init_percent > 1) {
        session_error("Configuration key " CFGKEY_INIT_PERCENT
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

    // Use the first two verticies of the base simplex as temporaries
    // to calculate the size tolerance.
    vertex_t* vertex = simplex.vertex;
    if (vertex_minimum(&vertex[0], space) != 0 ||
        vertex_maximum(&vertex[1], space) != 0)
        return -1;

    size_tol *= vertex_norm(&vertex[0], &vertex[1], VERTEX_NORM_L2);
    return 0;
}

int pro_algorithm(void)
{
    do {
        if (state == SIMPLEX_STATE_CONVERGED)
            break;

        if (pro_next_state() != 0)
            return -1;

        if (state == SIMPLEX_STATE_REFLECT) {
            if (check_convergence() != 0)
                return -1;
        }

        if (pro_next_simplex() != 0)
            return -1;

    } while (!simplex_inbounds(next, space));

    return 0;
}

int pro_next_state(void)
{
    switch (state) {
    case SIMPLEX_STATE_INIT:
    case SIMPLEX_STATE_SHRINK:
        /* Simply accept the candidate simplex and prepare to reflect. */
        best_base = best_test;
        state = SIMPLEX_STATE_REFLECT;
        break;

    case SIMPLEX_STATE_REFLECT:
        if (hperf_cmp(&reflect.vertex[best_test].perf,
                      &simplex.vertex[best_base].perf) < 0)
        {
            /* Reflected simplex has best known performance.
             * Attempt a trial expansion.
             */
            best_reflect = best_test;
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
        if (hperf_cmp(&expand.vertex[0].perf,
                      &reflect.vertex[best_reflect].perf) < 0)
        {
            /* Trial expansion has found the best known vertex thus far.
             * We are now free to expand the entire reflected simplex.
             */
            state = SIMPLEX_STATE_EXPAND_ALL;
        }
        else {
            /* Expanded test vertex does not improve performance.
             * Accept the original (unexpanded) reflected simplex and
             * attempt another reflection.
             */
            if (simplex_copy(&simplex, &reflect) != 0) {
                session_error("Could not copy reflection simplex"
                              " for next state");
                return -1;
            }
            best_base = best_reflect;
            state = SIMPLEX_STATE_REFLECT;
        }
        break;

    case SIMPLEX_STATE_EXPAND_ALL:
        if (simplex_inbounds(&expand, space)) {
            /* If the entire expanded simplex is valid (in bounds),
             * accept it as the reference simplex.
             */
            if (simplex_copy(&simplex, &expand) != 0) {
                session_error("Could not copy expanded simplex"
                              " for next state");
                return -1;
            }
            best_base = best_test;
        }
        else {
            /* Otherwise, accept the original (unexpanded) reflected
             * simplex as the reference simplex.
             */
            if (simplex_copy(&simplex, &reflect) != 0) {
                session_error("Could not copy reflected simplex"
                              " for next state");
                return -1;
            }
            best_base = best_reflect;
        }

        /* Either way, test reflection next. */
        state = SIMPLEX_STATE_REFLECT;
        break;

    default:
        return -1;
    }
    return 0;
}

int pro_next_simplex(void)
{
    switch (state) {
    case SIMPLEX_STATE_INIT:
        /* Bootstrap the process by testing the reference simplex. */
        next = &simplex;
        break;

    case SIMPLEX_STATE_REFLECT:
        // Each simplex vertex is translated to a position opposite
        // its origin (best performing) vertex.  This corresponds to a
        // coefficient that is -1 * (1.0 + <reflect coefficient>).
        simplex_transform(&simplex, &simplex.vertex[best_base],
                          -(1.0 + reflect_val), &reflect);
        next = &reflect;
        break;

    case SIMPLEX_STATE_EXPAND_ONE:
        /* Next simplex should have one vertex extending the best.
         * And the rest should be copies of the best known vertex.
         */
        vertex_transform(&simplex.vertex[best_reflect],
                         &simplex.vertex[best_base],
                         -(1.0 + expand_val), &expand.vertex[0]);

        for (int i = 1; i < expand.len; ++i)
            vertex_copy(&expand.vertex[i], &simplex.vertex[best_base]);

        next = &expand;
        break;

    case SIMPLEX_STATE_EXPAND_ALL:
        /* Expand all original simplex vertices away from the best
         * known vertex thus far. */
        simplex_transform(&simplex, &simplex.vertex[best_base],
                          -(1.0 + expand_val), &expand);
        next = &expand;
        break;

    case SIMPLEX_STATE_SHRINK:
        /* Shrink all original simplex vertices towards the best
         * known vertex thus far. */
        simplex_transform(&simplex, &simplex.vertex[best_base],
                          -shrink_val, &simplex);
        next = &simplex;
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

int check_convergence(void)
{
    if (simplex_centroid(&simplex, &centroid) != 0)
        return -1;

    if (simplex_collapsed(&simplex, space))
        goto converged;

    double avg_perf = hperf_unify(&centroid.perf);
    double fval_err = 0.0;
    for (int i = 0; i < simplex.len; ++i) {
        double vert_perf = hperf_unify(&simplex.vertex[i].perf);
        fval_err += ((vert_perf - avg_perf) * (vert_perf - avg_perf));
    }
    fval_err /= simplex.len;

    double size_max = 0.0;
    for (int i = 0; i < simplex.len; ++i) {
        double dist = vertex_norm(&simplex.vertex[i],
                                  &centroid, VERTEX_NORM_L2);
        if (size_max < dist)
            size_max = dist;
    }

    if (fval_err < fval_tol && size_max < size_tol)
        goto converged;

    return 0;

  converged:
    state = SIMPLEX_STATE_CONVERGED;
    session_setcfg(CFGKEY_CONVERGED, "1");
    return 0;
}
