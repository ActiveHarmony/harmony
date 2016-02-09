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

#include "strategy.h"
#include "session-core.h"
#include "hcfg.h"
#include "hspace.h"
#include "hpoint.h"
#include "hperf.h"
#include "hutil.h"
#include "libvertex.h"

#include <string.h> // For strcmp().
#include <math.h>   // For log(), isnan(), NAN, and HUGE_VAL.

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
    { CFGKEY_DIST_TOL, NULL,
      "Convergence test succeeds if the simplex moves (via reflection) "
      "a distance less than or equal to this percentage of the total "
      "search space for TOL_CNT consecutive steps.  Total search space "
      "is measured from minimum to maximum point.  This method overrides "
      "the default size/fval method." },
    { CFGKEY_TOL_CNT, "3",
      "The number of consecutive reflection steps which travel at or "
      "below DIST_TOL before the search is considered converged." },
    { CFGKEY_ANGEL_LOOSE, "False",
      "When all leeways cannot be satisfied simultaneously, attempt to "
      "satisfy as many leeways as possible, not necessarily favoring "
      "objectives with higher priority.  If false, ANGEL will satisfy "
      "as many higher priority objectives as possible before allowing "
      "violations in lower priority objectives." },
    { CFGKEY_ANGEL_MULT, "1.0",
      "Multiplicative factor for penalty function." },
    { CFGKEY_ANGEL_ANCHOR, "True",
      "Transfer the best known solution across search phases." },
    { CFGKEY_ANGEL_SAMESIMPLEX, "True",
      "Use the same initial simplex to begin each search phase.  This "
      "reduces the total number of evaluations when combined with the "
      "caching layer." },
    { CFGKEY_ANGEL_LEEWAY, NULL,
      "Comma (or whitespace) separated list of N-1 leeway values, "
      "where N is the number of objectives.  Each value may range "
      "from 0.0 to 1.0 (inclusive), and specifies how much the search "
      "may stray from its objective's minimum value." },
    { NULL }
};

hpoint_t best;
hperf_t  best_perf;

typedef struct span {
    double min;
    double max;
} span_t;

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

// Internal helper functions.
static int  allocate_structures(void);
static int  config_strategy(void);
static void check_convergence(void);
static int  increment_phase(void);
static int  nm_algorithm(void);
static int  nm_next_vertex(void);
static int  nm_state_transition(void);
static int  make_initial_simplex(void);
static int  update_centroid(void);

/* Variables to control search properties. */
vertex_t        init_point;
double          init_radius;
reject_method_t reject_type  = REJECT_METHOD_PENALTY;

double reflect_val;
double expand_val;
double contract_val;
double shrink_val;
double fval_tol;
double size_tol;
double dist_tol = NAN;
double move_len;
double space_size;
int    tol_cnt;

/* Variables to track current search state. */
hspace_t*       space;
simplex_state_t state;
vertex_t        centroid;
vertex_t        reflect;
vertex_t        expand;
vertex_t        contract;
simplex_t       init_simplex;
simplex_t       simplex;

vertex_t* next;
int       index_best;
int       index_worst;
int       index_curr; /* for INIT or SHRINK */
int       next_id = 1;

int     phase = -1;
int     perf_n;
double* thresh;
span_t* span;

/* Option variables */
double* leeway;
double  mult;
int     anchor;
int     loose;
int     samesimplex;

/*
 * Invoked once on strategy load.
 */
int strategy_init(hspace_t* space_ptr)
{
    if (!space) {
        // One time memory allocation and/or initialization.
        next_id = 1;
    }
    space = space_ptr;

    if (config_strategy() != 0)
        return -1;

    if (make_initial_simplex() != 0) {
        session_error("Could not initialize initial simplex.");
        return -1;
    }

    if (session_setcfg(CFGKEY_CONVERGED, "0") != 0) {
        session_error("Could not set " CFGKEY_CONVERGED " config variable.");
        return -1;
    }

    next_id = 1;
    if (increment_phase() != 0)
        return -1;

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

    if (hint && hint->id) {
        /* Update our state to include the hint point. */
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
        /* Apply an infinite penalty to the invalid point and
         * allow the algorithm to determine the next point to try.
         */
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
        /* Replace the rejected point with a random point. */
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
    if (trial->point.id != next->id) {
        session_error("Rouge points not supported.");
        return -1;
    }
    hperf_copy(&next->perf, &trial->perf);

    /* Update the observed value ranges. */
    for (int i = 0; i < perf_n; ++i) {
        if (span[i].min > next->perf.obj[i])
            span[i].min = next->perf.obj[i];

        if (span[i].max < next->perf.obj[i] && next->perf.obj[i] < HUGE_VAL)
            span[i].max = next->perf.obj[i];
    }

    double penalty = 0.0;
    double penalty_base = 1.0;
    for (int i = phase-1; i >= 0; --i) {
        if (next->perf.obj[i] > thresh[i]) {
            if (!loose) {
                penalty += penalty_base;
            }
            penalty += 1.0 / (1.0 - log((next->perf.obj[i] - thresh[i]) /
                                        (span[i].max       - thresh[i])));
        }
        penalty_base *= 2;
    }

    if (penalty > 0.0) {
        if (loose) {
            penalty += 1.0;
        }
        next->perf.obj[phase] += penalty * (span[phase].max -
                                            span[phase].min) * mult;
    }

    /* Update the best performing point, if necessary. */
    if (!best_perf.len || best_perf.obj[phase] > next->perf.obj[phase]) {
        if (hperf_copy(&best_perf, &next->perf) != 0) {
            session_error("Could not store best performance");
            return -1;
        }

        if (hpoint_copy(&best, &trial->point) != 0) {
            session_error("Could not copy best point during analyze");
            return -1;
        }
    }

    if (nm_algorithm() != 0) {
        session_error("Nelder-Mead algorithm failure");
        return -1;
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
        session_error("Could not copy best point during strategy_best()");
        return -1;
    }
    return 0;
}

//
// Internal helper function implementation.
//
int allocate_structures(void)
{
    void* newbuf;

    if (simplex_init(&init_simplex, space->len) != 0) {
        session_error("Could not allocate initial simplex");
        return -1;
    }

    for (int i = 0; i < init_simplex.len; ++i) {
        if (hperf_init(&init_simplex.vertex[i].perf, perf_n) != 0) {
            session_error("Could not allocate initial simplex performance");
            return -1;
        }
    }

    if (simplex_init(&simplex, space->len) != 0) {
        session_error("Could not allocate base simplex");
        return -1;
    }

    for (int i = 0; i < simplex.len; ++i) {
        if (hperf_init(&simplex.vertex[i].perf, perf_n) != 0) {
            session_error("Could not allocate initial simplex performance");
            return -1;
        }
    }

    if (hpoint_init(&best, space->len) != 0 ||
        hperf_init(&best_perf, perf_n) != 0)
    {
        session_error("Could not allocate best point");
        return -1;
    }

    if (vertex_init(&reflect, space->len) != 0 ||
        hperf_init(&reflect.perf, perf_n) != 0)
    {
        session_error("Could not allocate reflection vertex");
        return -1;
    }

    if (vertex_init(&expand, space->len) != 0 ||
        hperf_init(&expand.perf, perf_n) != 0)
    {
        session_error("Could not allocate expansion vertex");
        return -1;
    }

    if (vertex_init(&contract, space->len) != 0 ||
        hperf_init(&contract.perf, perf_n) != 0)
    {
        session_error("Could not allocate contraction vertex");
        return -1;
    }

    newbuf = realloc(leeway, (perf_n - 1) * sizeof(*leeway));
    if (!newbuf) {
        session_error("Could not allocate leeway vector");
        return -1;
    }
    leeway = newbuf;

    newbuf = realloc(span, perf_n * sizeof(*span));
    if (!newbuf) {
        session_error("Could not allocate span container");
        return -1;
    }
    span = newbuf;

    newbuf = realloc(thresh, (perf_n - 1) * sizeof(*thresh));
    if (!newbuf) {
        session_error("Could not allocate threshold container");
        return -1;
    }
    thresh = newbuf;

    return 0;
}

int config_strategy(void)
{
    const char* cfgval;

    loose = hcfg_bool(session_cfg, CFGKEY_ANGEL_LOOSE);
    mult = hcfg_real(session_cfg, CFGKEY_ANGEL_MULT);
    if (isnan(mult)) {
        session_error("Invalid value for " CFGKEY_ANGEL_MULT
                      " configuration key.");
        return -1;
    }

    anchor = hcfg_bool(session_cfg, CFGKEY_ANGEL_ANCHOR);
    samesimplex = hcfg_bool(session_cfg, CFGKEY_ANGEL_SAMESIMPLEX);

    init_radius = hcfg_real(session_cfg, CFGKEY_INIT_RADIUS);
    if (init_radius <= 0 || init_radius > 1) {
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

    perf_n = hcfg_int(session_cfg, CFGKEY_PERF_COUNT);
    if (perf_n < 1) {
        session_error("Invalid value for " CFGKEY_PERF_COUNT
                      " configuration key.");
        return -1;
    }

    if (allocate_structures() != 0)
        return -1;

    // Use the expand and reflect vertex variables as temporaries to
    // calculate the size tolerance.
    if (vertex_minimum(&expand, space) != 0 ||
        vertex_maximum(&reflect, space) != 0)
        return -1;
    space_size = vertex_norm(&expand, &reflect, VERTEX_NORM_L2);

    dist_tol = hcfg_real(session_cfg, CFGKEY_DIST_TOL);
    if (!isnan(dist_tol)) {
        if (dist_tol <= 0.0 || dist_tol >= 1.0) {
            session_error("Configuration key " CFGKEY_DIST_TOL
                          " must be between 0.0 and 1.0 (exclusive).");
            return -1;
        }
        dist_tol *= space_size;

        tol_cnt = hcfg_int(session_cfg, CFGKEY_TOL_CNT);
        if (tol_cnt < 1) {
            session_error("Configuration key " CFGKEY_TOL_CNT
                          " must be greater than zero");
            return -1;
        }
    }
    else {
        // CFGKEY_DIST_TOL is not defined.  Use the size/fval method.
        fval_tol = hcfg_real(session_cfg, CFGKEY_FVAL_TOL);
        if (isnan(fval_tol)) {
            session_error("Invalid value for " CFGKEY_FVAL_TOL
                          " configuration key.");
            return -1;
        }

        size_tol = hcfg_real(session_cfg, CFGKEY_SIZE_TOL);
        if (isnan(size_tol) || size_tol <= 0.0 || size_tol >= 1.0) {
            session_error("Configuration key " CFGKEY_SIZE_TOL
                          " must be between 0.0 and 1.0 (exclusive).");
            return -1;
        }
        size_tol *= space_size;
    }

    if (hcfg_get(session_cfg, CFGKEY_ANGEL_LEEWAY)) {
        if (hcfg_arr_len(session_cfg, CFGKEY_ANGEL_LEEWAY) != perf_n - 1) {
            session_error("Incorrect number of leeway values provided.");
            return -1;
        }

        for (int i = 0; i < perf_n - 1; ++i) {
            leeway[i] = hcfg_arr_real(session_cfg, CFGKEY_ANGEL_LEEWAY, i);
            if (isnan(leeway[i])) {
                session_error("Invalid value for " CFGKEY_ANGEL_LEEWAY
                              " configuration key.");
                return -1;
            }
        }
    }
    else {
        session_error(CFGKEY_ANGEL_LEEWAY " must be defined.");
        return -1;
    }

    for (int i = 0; i < perf_n; ++i) {
        span[i].min =  HUGE_VAL;
        span[i].max = -HUGE_VAL;
    }

    return 0;
}

void check_convergence(void)
{
    static int flat_cnt;

    // Converge if all simplex objective values remain the same after 3 moves.
    int flat = 1;
    double perf_0 = simplex.vertex[0].perf.obj[phase];
    for (unsigned i = 1; i < simplex.len; ++i) {
        double perf_i = simplex.vertex[i].perf.obj[phase];

        if (perf_i < perf_0 || perf_0 < perf_i) {
            flat = 0;
            break;
        }
    }
    if (flat) {
        if (++flat_cnt >= 3) {
            flat_cnt = 0;
            goto converged;
        }
    }
    else flat_cnt = 0;

    // Converge if all simplex verticies map to the same underlying hpoint.
    if (simplex_collapsed(&simplex, space))
        goto converged;

    // Converge if the simplex moves via reflection below a distance tolerance.
    if (!isnan(dist_tol)) {
        static int cnt;

        if (move_len < dist_tol) {
            if (++cnt >= tol_cnt) {
                cnt = 0;
                goto converged;
            }
        }
        else cnt = 0;
    }
    // If a dist_tol is not set, converge based on simplex size and flatness.
    else {
        double fval_err = 0;
        double base_val = centroid.perf.obj[phase];
        for (int i = 0; i < simplex.len; ++i) {
            fval_err += ((simplex.vertex[i].perf.obj[phase] - base_val) *
                         (simplex.vertex[i].perf.obj[phase] - base_val));
        }
        fval_err /= simplex.len;

        double size_max = 0;
        for (int i = 0; i < simplex.len; ++i) {
            double dist = vertex_norm(&simplex.vertex[i], &centroid,
                                      VERTEX_NORM_L2);
            if (size_max < dist)
                size_max = dist;
        }

        if (fval_err < fval_tol && size_max < size_tol)
            goto converged;
    }
    return;

  converged:
    if (phase == perf_n - 1) {
        state = SIMPLEX_STATE_CONVERGED;
        session_setcfg(CFGKEY_CONVERGED, "1");
    }
    else {
        increment_phase();
    }
}

int increment_phase(void)
{
    if (phase >= 0) {
        /* Calculate the threshold for the previous phase. */
        thresh[phase] = span[phase].min + (span[phase].max -
                                           span[phase].min) * leeway[phase];
    }
    ++phase;

    char intbuf[16];
    snprintf(intbuf, sizeof(intbuf), "%d", phase);
    session_setcfg(CFGKEY_ANGEL_PHASE, intbuf);

    // Use the centroid to store the previous phase's best vertex.
    if (vertex_copy(&centroid, &simplex.vertex[index_best]) != 0) {
        session_error("Could not copy best vertex during phase increment");
        return -1;
    }

    if (!samesimplex) {
        /* Re-initialize the initial simplex, if needed. */
        if (make_initial_simplex() != 0) {
            session_error("Could not reinitialize the initial simplex.");
            return -1;
        }
    }
    simplex_copy(&simplex, &init_simplex);

    if (best.id > 0) {
        if (anchor) {
            double min_dist = HUGE_VAL;
            int idx = -1;
            for (int i = 0; i < simplex.len; ++i) {
                double curr_dist = vertex_norm(&centroid, &simplex.vertex[i],
                                               VERTEX_NORM_L2);
                if (min_dist > curr_dist) {
                    min_dist = curr_dist;
                    idx = i;
                }
            }

            if (vertex_copy(&simplex.vertex[idx], &centroid) != 0) {
                session_error("Could not anchor simplex to best point");
                return -1;
            }
        }
    }
    hperf_reset(&best_perf);
    best.id = 0;

    state = SIMPLEX_STATE_INIT;
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
        /* Simplex vertex performance value. */
        if (++index_curr == space->len + 1) {
            update_centroid();
            state = SIMPLEX_STATE_REFLECT;
            index_curr = 0;
        }
        break;

    case SIMPLEX_STATE_REFLECT:
        if (reflect.perf.obj[phase] <
            simplex.vertex[index_best].perf.obj[phase])
        {
            /* Reflected point performs better than all simplex
             * points.  Attempt expansion.
             */
            state = SIMPLEX_STATE_EXPAND;
        }
        else if (reflect.perf.obj[phase] <
                 simplex.vertex[index_worst].perf.obj[phase])
        {
            /* Reflected point performs better than worst simplex
             * point.  Replace the worst simplex point with reflected
             * point and attempt reflection again.
             */
            if (vertex_copy(&simplex.vertex[index_worst], &reflect) != 0)
                return -1;

            update_centroid();
        }
        else {
            /* Reflected point performs worse than all simplex points.
             * Attempt contraction.
             */
            state = SIMPLEX_STATE_CONTRACT;
        }
        break;

    case SIMPLEX_STATE_EXPAND:
        if (expand.perf.obj[phase] <
            simplex.vertex[index_best].perf.obj[phase])
        {
            /* Expanded point performs even better than reflected
             * point.  Replace the worst simplex point with the
             * expanded point and attempt reflection again.
             */
            if (vertex_copy(&simplex.vertex[index_worst], &expand) != 0)
                return -1;
        }
        else {
            /* Expanded point did not result in improved performance.
             * Replace the worst simplex point with the original
             * reflected point and attempt reflection again.
             */
            if (vertex_copy(&simplex.vertex[index_worst], &reflect) != 0)
                return -1;
        }
        update_centroid();
        state = SIMPLEX_STATE_REFLECT;
        break;

    case SIMPLEX_STATE_CONTRACT:
        if (contract.perf.obj[phase] <
            simplex.vertex[index_worst].perf.obj[phase])
        {
            /* Contracted point performs better than the worst simplex
             * point. Replace the worst simplex point with contracted
             * point and attempt reflection.
             */
            if (vertex_copy(&simplex.vertex[index_worst], &contract) != 0)
                return -1;

            update_centroid();
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
        next = &simplex.vertex[index_curr];
        break;

    case SIMPLEX_STATE_REFLECT:
        /* Test a vertex reflected from the worst performing vertex
         * through the centroid point. */
        if (vertex_transform(&centroid, &simplex.vertex[index_worst],
                             reflect_val, &reflect) != 0)
            return -1;

        move_len = vertex_norm(&simplex.vertex[index_worst], &reflect,
                               VERTEX_NORM_L2) / space_size;

        next = &reflect;
        break;

    case SIMPLEX_STATE_EXPAND:
        /* Test a vertex that expands the reflected vertex even
         * further from the the centroid point. */
        if (vertex_transform(&centroid, &simplex.vertex[index_worst],
                             expand_val, &expand) != 0)
            return -1;

        next = &expand;
        break;

    case SIMPLEX_STATE_CONTRACT:
        /* Test a vertex contracted from the worst performing vertex
         * towards the centroid point. */
        if (vertex_transform(&simplex.vertex[index_worst], &centroid,
                             -contract_val, &contract) != 0)
            return -1;

        next = &contract;
        break;

    case SIMPLEX_STATE_SHRINK:
        if (index_curr == -1) {
          /* Shrink the entire simplex towards the best known vertex
           * thus far. */
            if (simplex_transform(&simplex, &simplex.vertex[index_best],
                                  -shrink_val, &simplex) != 0)
                return -1;

            index_curr = 0;
        }

        // Test individual vertices of the initial simplex.
        next = &simplex.vertex[index_curr];
        break;

    case SIMPLEX_STATE_CONVERGED:
        /* Simplex has converged.  Nothing to do.
         * In the future, we may consider new search at this point. */
        next = &simplex.vertex[index_best];
        break;

    default:
        return -1;
    }

    hperf_reset(&next->perf);
    return 0;
}

int make_initial_simplex(void)
{
    const char* cfgval = hcfg_get(session_cfg, CFGKEY_INIT_POINT);
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

    if (simplex_set(&init_simplex, space, &init_point, init_radius) != 0) {
        session_error("Could not generate initial simplex");
        return -1;
    }

    return 0;
}

int update_centroid(void)
{
    index_best = 0;
    index_worst = 0;

    for (int i = 1; i < simplex.len; ++i) {
        if (simplex.vertex[i].perf.obj[phase] <
            simplex.vertex[index_best].perf.obj[phase])
            index_best = i;

        if (simplex.vertex[i].perf.obj[phase] >
            simplex.vertex[index_worst].perf.obj[phase])
            index_worst = i;
    }

    unsigned stashed_id = simplex.vertex[index_worst].id;
    simplex.vertex[index_worst].id = 0;
    if (simplex_centroid(&simplex, &centroid) != 0)
        return -1;

    simplex.vertex[index_worst].id = stashed_id;
    return 0;
}
