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

#include "strategy.h"
#include "session-core.h"
#include "hsig.h"
#include "hperf.h"
#include "hutil.h"
#include "hcfg.h"
#include "libvertex.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
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

hpoint_t best = HPOINT_INITIALIZER;
hperf_t* best_perf;

hpoint_t curr;

typedef struct value_range {
    double min;
    double max;
} value_range_t;

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
int angel_init_simplex(void);
int strategy_cfg(hsig_t* sig);
int angel_phase_incr(void);
void simplex_update_index(void);
void simplex_update_centroid(void);
int  nm_algorithm(void);
int  nm_state_transition(void);
int  nm_next_vertex(void);
void check_convergence(void);
double calc_perf(double* perf);

/* Variables to control search properties. */
simplex_init_t  init_method  = SIMPLEX_INIT_CENTER;
vertex_t*       init_point;
double          init_percent = 0.35;
reject_method_t reject_type  = REJECT_METHOD_PENALTY;

double reflect  = 1.0;
double expand   = 2.0;
double contract = 0.5;
double shrink   = 0.5;
double fval_tol = 1e-8;
double size_tol;
double dist_tol = NAN;
double move_len;
int simplex_size;
int tol_cnt;

/* Variables to track current search state. */
simplex_state_t state;
vertex_t* centroid;
vertex_t* test;
simplex_t* base;
simplex_t* init;
vertex_t* next;

int phase = -1;
vertex_t* simplex_best;
vertex_t* simplex_worst;
int curr_idx; /* for INIT or SHRINK */
int next_id;

int perf_n;
double* thresh;
value_range_t* range;

/* Option variables */
double* leeway;
double mult;
int anchor;
int loose;
int samesimplex;

int angel_init_simplex()
{
    switch (init_method) {
    case SIMPLEX_INIT_CENTER: vertex_center(init_point); break;
    case SIMPLEX_INIT_RANDOM: vertex_rand_trim(init_point,
                                               init_percent); break;
    case SIMPLEX_INIT_POINT:  assert(0 && "Not yet implemented."); break;
    default:
        session_error("Invalid initial search method.");
        return -1;
    }
    return simplex_from_vertex(init_point, init_percent, init);
}

int angel_phase_incr(void)
{
    int i;
    double min_dist, curr_dist;
    char intbuf[16];

    if (phase >= 0) {
        /* Calculate the threshold for the previous phase. */
        thresh[phase] = range[phase].min + (range[phase].max -
                                            range[phase].min) * leeway[phase];
    }

    ++phase;
    snprintf(intbuf, sizeof(intbuf), "%d", phase);
    session_setcfg(CFGKEY_ANGEL_PHASE, intbuf);

    if (!samesimplex) {
        /* Re-initialize the initial simplex, if needed. */
        if (angel_init_simplex() != 0) {
            session_error("Could not reinitialize the initial simplex.");
            return -1;
        }
    }
    simplex_copy(base, init);

    if (best.id > 0) {
        if (anchor) {
            int idx = -1;
            min_dist = HUGE_VAL;
            for (i = 0; i < simplex_size; ++i) {
                vertex_from_hpoint(&best, test);
                curr_dist = vertex_dist(test, base->vertex[i]);
                if (min_dist > curr_dist) {
                    min_dist = curr_dist;
                    idx = i;
                }
            }
            vertex_copy(base->vertex[idx], test);
        }

        hperf_reset(best_perf);
        best.id = 0;
    }

    state = SIMPLEX_STATE_INIT;
    curr_idx = 0;
    return 0;
}

/*
 * Invoked once on strategy load.
 */
int strategy_init(hsig_t* sig)
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

    best_perf = hperf_alloc(perf_n);

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

    init = simplex_alloc(simplex_size);
    if (!init) {
        session_error("Could not allocate memory for initial simplex.");
        return -1;
    }

    if (angel_init_simplex() != 0) {
        session_error("Could not initialize initial simplex.");
        return -1;
    }

    base = simplex_alloc(simplex_size);
    if (!base) {
        session_error("Could not allocate memory for base simplex.");
        return -1;
    }

    if (session_setcfg(CFGKEY_CONVERGED, "0") != 0) {
        session_error("Could not set " CFGKEY_CONVERGED " config variable.");
        return -1;
    }

    next_id = 1;
    if (angel_phase_incr() != 0)
        return -1;

    if (nm_next_vertex() != 0) {
        session_error("Could not initiate test vertex.");
        return -1;
    }

    return 0;
}

int strategy_cfg(hsig_t* sig)
{
    int i;
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

    /* Make sure the simplex size is N+1 or greater. */
    simplex_size = hcfg_int(session_cfg, CFGKEY_SIMPLEX_SIZE);
    if (simplex_size < sig->range_len + 1)
        simplex_size = sig->range_len + 1;

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

    init_percent = hcfg_real(session_cfg, CFGKEY_INIT_PERCENT);
    if (init_percent <= 0 || init_percent > 1) {
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

    dist_tol = hcfg_real(session_cfg, CFGKEY_DIST_TOL);
    if (!isnan(dist_tol)) {
        if (dist_tol <= 0.0 || dist_tol >= 1.0) {
            session_error("Configuration key " CFGKEY_DIST_TOL
                          " must be between 0.0 and 1.0 (exclusive).");
            return -1;
        }
        dist_tol *= vertex_dist(vertex_min(), vertex_max());
        tol_cnt = hcfg_int(session_cfg, CFGKEY_TOL_CNT);
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
        size_tol *= vertex_dist(vertex_min(), vertex_max());
    }

    perf_n = hcfg_int(session_cfg, CFGKEY_PERF_COUNT);
    if (perf_n < 1) {
        session_error("Invalid value for " CFGKEY_PERF_COUNT
                      " configuration key.");
        return -1;
    }

    leeway = malloc(sizeof(double) * (perf_n - 1));
    if (!leeway) {
        session_error("Could not allocate memory for leeway vector.");
        return -1;
    }

    if (hcfg_get(session_cfg, CFGKEY_ANGEL_LEEWAY)) {
        if (hcfg_arr_len(session_cfg, CFGKEY_ANGEL_LEEWAY) != perf_n - 1) {
            session_error("Incorrect number of leeway values provided.");
            return -1;
        }

        for (i = 0; i < perf_n - 1; ++i) {
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

    range = malloc(sizeof(value_range_t) * perf_n);
    if (!range) {
        session_error("Could not allocate memory for range container.");
        return -1;
    }
    for (i = 0; i < perf_n; ++i) {
        range[i].min =  HUGE_VAL;
        range[i].max = -HUGE_VAL;
    }

    thresh = malloc(sizeof(double) * (perf_n - 1));
    if (!thresh) {
        session_error("Could not allocate memory for threshold container.");
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
            int i;
            /* Apply an infinite penalty to the invalid point and
             * allow the algorithm to determine the next point to try.
             */

            for (i = 0; i < perf_n; ++i)
                next->perf->p[i] = HUGE_VAL;

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
    int i;
    double penalty, penalty_base;

    if (trial->point.id != next->id) {
        session_error("Rouge points not supported.");
        return -1;
    }
    hperf_copy(next->perf, trial->perf);

    /* Update the observed value ranges. */
    for (i = 0; i < perf_n; ++i) {
        if (range[i].min > next->perf->p[i])
            range[i].min = next->perf->p[i];

        if (range[i].max < next->perf->p[i] && next->perf->p[i] < HUGE_VAL)
            range[i].max = next->perf->p[i];
    }

    penalty = 0.0;
    penalty_base = 1.0;
    for (i = phase-1; i >= 0; --i) {
        if (next->perf->p[i] > thresh[i]) {
            if (!loose) {
                penalty += penalty_base;
            }
            penalty += 1.0 / (1.0 - log((next->perf->p[i] - thresh[i]) /
                                        (range[i].max     - thresh[i])));
        }
        penalty_base *= 2;
    }

    if (penalty > 0.0) {
        if (loose) {
            penalty += 1.0;
        }
        next->perf->p[phase] += penalty * (range[phase].max -
                                           range[phase].min) * mult;
    }

    /* Update the best performing point, if necessary. */
    if (best_perf->p[phase] > next->perf->p[phase]) {
        hperf_copy(best_perf, next->perf);
        if (hpoint_copy(&best, &trial->point) != 0) {
            session_error( strerror(errno) );
            return -1;
        }
    }

    if (nm_algorithm() != 0) {
        session_error("Internal error: Nelder-Mead algorithm failure.");
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

        if (nm_next_vertex() != 0)
            return -1;

        if (state == SIMPLEX_STATE_REFLECT)
            check_convergence();

    } while (vertex_outofbounds(next));

    return 0;
}

int nm_state_transition(void)
{
    double perf = next->perf->p[phase];

    switch (state) {
    case SIMPLEX_STATE_INIT:
    case SIMPLEX_STATE_SHRINK:
        /* Simplex vertex performance value. */
        if (++curr_idx == simplex_size) {
            simplex_update_centroid();
            state = SIMPLEX_STATE_REFLECT;
        }
        break;

    case SIMPLEX_STATE_REFLECT:
        if (perf < simplex_best->perf->p[phase]) {
            /* Reflected test vertex has best known performance.
             * Replace the worst performing simplex vertex with the
             * reflected test vertex, and attempt expansion.
             */
            vertex_copy(simplex_worst, test);
            state = SIMPLEX_STATE_EXPAND;
        }
        else if (perf < simplex_worst->perf->p[phase]) {
            /* Reflected test vertex performs better than the worst
             * known simplex vertex.  Replace the worst performing
             * simplex vertex with the reflected test vertex.
             */
            vertex_t* prev_worst;
            vertex_copy(simplex_worst, test);

            prev_worst = simplex_worst;
            simplex_update_centroid();

            /* If the test vertex has become the new worst performing
             * point, attempt contraction.  Otherwise, try reflection.
             */
            if (prev_worst == simplex_worst)
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
        if (perf < simplex_best->perf->p[phase]) {
            /* Expanded test vertex has best known performance.
             * Replace the worst performing simplex vertex with the
             * test vertex, and attempt expansion.
             */
            vertex_copy(simplex_worst, test);
        }

        simplex_update_centroid();
        state = SIMPLEX_STATE_REFLECT;
        break;

    case SIMPLEX_STATE_CONTRACT:
        if (perf < simplex_worst->perf->p[phase]) {
            /* Contracted test vertex performs better than the worst
             * known simplex vertex.  Replace the worst performing
             * simplex vertex with the test vertex.
             */
            vertex_copy(simplex_worst, test);
            simplex_update_centroid();
            state = SIMPLEX_STATE_REFLECT;
        }
        else {
            /* Contracted test vertex has worst known performance.
             * Shrink the entire simplex towards the best point.
             */
            curr_idx = -1; /* to notify the beginning of SHRINK */
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
        next = base->vertex[curr_idx];
        break;

    case SIMPLEX_STATE_REFLECT:
        /* Test a vertex reflected from the worst performing vertex
         * through the centroid point. */
        vertex_transform(simplex_worst, centroid, -reflect, test);
        next = test;
        move_len = 100 * (vertex_dist(simplex_worst, next) /
                          vertex_dist(vertex_min(), vertex_max()));
        break;

    case SIMPLEX_STATE_EXPAND:
        /* Test a vertex that expands the reflected vertex even
         * further from the the centroid point. */
        vertex_transform(simplex_worst, centroid, expand, test);
        next = test;
        break;

    case SIMPLEX_STATE_CONTRACT:
        /* Test a vertex contracted from the worst performing vertex
         * towards the centroid point. */
        vertex_transform(simplex_worst, centroid, contract, test);
        next = test;
        break;

    case SIMPLEX_STATE_SHRINK:
        if (curr_idx == -1) {
          /* Shrink the entire simplex towards the best known vertex
           * thus far. */
          simplex_transform(base, simplex_best, shrink, base);
          curr_idx = 0;
          next = base->vertex[curr_idx];
        } else {
          /* Test individual vertices of the initial simplex. */
          next = base->vertex[curr_idx];
        }
        break;

    case SIMPLEX_STATE_CONVERGED:
        /* Simplex has converged.  Nothing to do.
         * In the future, we may consider new search at this point. */
        next = simplex_best;
        break;

    default:
        return -1;
    }
    return 0;
}

void simplex_update_index(void)
{
    int i;

    simplex_best = base->vertex[0];
    simplex_worst = base->vertex[0];
    for (i = 1; i < simplex_size; ++i) {
        if (base->vertex[i]->perf->p[phase] < simplex_best->perf->p[phase])
            simplex_best = base->vertex[i];

        if (base->vertex[i]->perf->p[phase] > simplex_worst->perf->p[phase])
            simplex_worst = base->vertex[i];
    }
}

void simplex_update_centroid(void)
{
    int stash;

    simplex_update_index();
    stash = simplex_worst->id;
    simplex_worst->id = -1;

    simplex_centroid(base, centroid);

    simplex_worst->id = stash;
}

void check_convergence(void)
{
    static int cnt;

    if (simplex_collapsed(base) == 1)
        goto converged;

    if (!isnan(dist_tol)) {
        if (move_len < dist_tol) {
            if (++cnt >= tol_cnt) {
                cnt = 0;
                goto converged;
            }
        }
        else cnt = 0;
    }
    else {
        int i;
        double fval_err, size_max, base_val;

        fval_err = 0;
        base_val = centroid->perf->p[phase];
        for (i = 0; i < simplex_size; ++i) {
            fval_err += ((base->vertex[i]->perf->p[phase] - base_val) *
                         (base->vertex[i]->perf->p[phase] - base_val));
        }
        fval_err /= simplex_size;

        size_max = 0;
        for (i = 0; i < simplex_size; ++i) {
            double dist = vertex_dist(base->vertex[i], centroid);
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
        angel_phase_incr();
    }
}
