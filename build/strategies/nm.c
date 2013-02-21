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
#include "hsession.h"
#include "hutil.h"
#include "hcfg.h"
#include "defaults.h"
#include "libvertex.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <math.h>

hpoint_t strategy_best;
double strategy_best_perf;

hpoint_t curr;

typedef enum simplex_init {
    SIMPLEX_INIT_UNKNOWN = 0,
    SIMPLEX_INIT_RANDOM,
    SIMPLEX_INIT_MAXVAL,

    SIMPLEX_INIT_MAX
} simplex_init_t;

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
int  strategy_cfg(hmesg_t *mesg);
int  init_by_random(void);
int  init_by_maxval(void);
void simplex_update_index(void);
void simplex_update_centroid(void);
int  nm_algorithm(void);
int  nm_next_state(const vertex_t *input);
int  nm_next_vertex(vertex_t *output);
void check_convergence(void);

/* Variables to control search properties. */
simplex_init_t init_method  = SIMPLEX_INIT_MAXVAL;
double reflect_coefficient  = 1.0;
double expand_coefficient   = 2.0;
double contract_coefficient = 0.5;
double shrink_coefficient   = 0.5;
double converge_fv_tol      = 1e-4;
double converge_sz_tol;
int simplex_size;

/* Variables to track current search state. */
simplex_state_t state;
vertex_t *centroid;
vertex_t *test;
simplex_t *base;

int index_best;
int index_worst;
int index_curr;
int next_id;

/*
 * Invoked once on strategy load.
 */
int strategy_init(hmesg_t *mesg)
{
    if (libvertex_init(sess) != 0) {
        mesg->data.string = "Could not initialize vertex library.";
        return -1;
    }

    if (strategy_cfg(mesg) != 0)
        return -1;

    strategy_best = HPOINT_INITIALIZER;
    strategy_best_perf = INFINITY;

    if (hpoint_init(&curr, simplex_size) < 0)
        return -1;

    centroid = vertex_alloc();
    if (!centroid) {
        mesg->data.string = "Could not allocate memory for centroid vertex.";
        return -1;
    }

    test = vertex_alloc();
    if (!test) {
        mesg->data.string = "Could not allocate memory for testing vertex.";
        return -1;
    }

    /* Default stopping criteria: 0.5% of dist(vertex_min, vertex_max). */
    if (converge_sz_tol == 0) {
        vertex_min(test);
        vertex_max(centroid);
        converge_sz_tol = vertex_dist(test, centroid) * 0.005;
    }

    base = simplex_alloc(simplex_size);
    if (!base) {
        mesg->data.string = "Could not allocate memory for base simplex.";
        return -1;
    }

    switch (init_method) {
    case SIMPLEX_INIT_RANDOM: init_by_random(); break;
    case SIMPLEX_INIT_MAXVAL: init_by_maxval(); break;
    default:
        mesg->data.string = "Invalid initial search method.";
        return -1;
    }

    next_id = 1;
    state = SIMPLEX_STATE_INIT;

    if (hcfg_set(sess->cfg, CFGKEY_STRATEGY_CONVERGED, "0") < 0) {
        mesg->data.string =
            "Could not set " CFGKEY_STRATEGY_CONVERGED " config variable.";
        return -1;
    }

    if (nm_next_vertex(test) != 0) {
        mesg->data.string = "Could not initiate test vertex.";
        return -1;
    }

    return 0;
}

int strategy_cfg(hmesg_t *mesg)
{
    const char *cfgval;
    char *endp;
    int retval;

    /* Make sure the simplex size is N+1 or greater. */
    cfgval = hcfg_get(sess->cfg, CFGKEY_NM_SIMPLEX_SIZE);
    if (cfgval)
        simplex_size = atoi(cfgval);

    if (simplex_size < sess->sig.range_len + 1)
        simplex_size = sess->sig.range_len + 1;

    /* Make sure the prefetch count is either 0 or 1. */
    cfgval = hcfg_get(sess->cfg, CFGKEY_PREFETCH_COUNT);
    if (cfgval) {
        retval = atoi(cfgval);
        if (retval > 1 || strcasecmp(cfgval, "auto") == 0) {
            hcfg_set(sess->cfg, CFGKEY_PREFETCH_COUNT, "1");
        }
        else if (retval < 0) {
            hcfg_set(sess->cfg, CFGKEY_PREFETCH_COUNT, "0");
        }
    }

    /* Nelder-Mead algorithm requires an atomic prefetch queue. */
    hcfg_set(sess->cfg, CFGKEY_PREFETCH_ATOMIC, "1");

    cfgval = hcfg_get(sess->cfg, CFGKEY_NM_SIMPLEX_INIT);
    if (cfgval) {
        if (strcasecmp(cfgval, "maxval") == 0) {
            init_method = SIMPLEX_INIT_MAXVAL;
        }
        else if (strcasecmp(cfgval, "random") == 0){
            init_method = SIMPLEX_INIT_RANDOM;
        }
        else {
            mesg->data.string = "Invalid value for "
                CFGKEY_NM_SIMPLEX_INIT " configuration key.";
            return -1;
        }
    }

    cfgval = hcfg_get(sess->cfg, CFGKEY_NM_REFLECT);
    if (cfgval) {
        reflect_coefficient = strtod(cfgval, &endp);
        if (*endp != '\0') {
            mesg->data.string = "Invalid value for " CFGKEY_NM_REFLECT
                " configuration key.";
            return -1;
        }
        if (reflect_coefficient <= 0) {
            mesg->data.string = "Configuration key " CFGKEY_NM_REFLECT
                " must be positive.";
            return -1;
        }
    }

    cfgval = hcfg_get(sess->cfg, CFGKEY_NM_EXPAND);
    if (cfgval) {
        expand_coefficient = strtod(cfgval, &endp);
        if (*endp != '\0') {
            mesg->data.string = "Invalid value for " CFGKEY_NM_EXPAND
                " configuration key.";
            return -1;
        }
        if (reflect_coefficient <= 1) {
            mesg->data.string = "Configuration key " CFGKEY_NM_EXPAND
                " must be greater than 1.0.";
            return -1;
        }
    }

    cfgval = hcfg_get(sess->cfg, CFGKEY_NM_CONTRACT);
    if (cfgval) {
        contract_coefficient = strtod(cfgval, &endp);
        if (*endp != '\0') {
            mesg->data.string = "Invalid value for " CFGKEY_NM_CONTRACT
                " configuration key.";
            return -1;
        }
        if (reflect_coefficient <= 0 || reflect_coefficient >= 1) {
            mesg->data.string = "Configuration key " CFGKEY_NM_CONTRACT
                " must be between 0.0 and 1.0 (exclusive).";
            return -1;
        }
    }

    cfgval = hcfg_get(sess->cfg, CFGKEY_NM_SHRINK);
    if (cfgval) {
        shrink_coefficient = strtod(cfgval, &endp);
        if (*endp != '\0') {
            mesg->data.string = "Invalid value for " CFGKEY_NM_SHRINK
                " configuration key.";
            return -1;
        }
        if (reflect_coefficient <= 0 || reflect_coefficient >= 1) {
            mesg->data.string = "Configuration key " CFGKEY_NM_SHRINK
                " must be between 0.0 and 1.0 (exclusive).";
            return -1;
        }
    }

    cfgval = hcfg_get(sess->cfg, CFGKEY_NM_CONVERGE_FV);
    if (cfgval) {
        converge_fv_tol = strtod(cfgval, &endp);
        if (*endp != '\0') {
            mesg->data.string = "Invalid value for " CFGKEY_NM_REFLECT
                " configuration key.";
            return -1;
        }
    }

    cfgval = hcfg_get(sess->cfg, CFGKEY_NM_CONVERGE_SZ);
    if (cfgval) {
        converge_sz_tol = strtod(cfgval, &endp);
        if (*endp != '\0') {
            mesg->data.string = "Invalid value for " CFGKEY_NM_REFLECT
                " configuration key.";
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

/*
 * Generate a new candidate configuration point in the space provided
 * by the hpoint_t parameter.
 */
int strategy_fetch(hmesg_t *mesg)
{
    test->id = next_id;
    if (vertex_to_hpoint(test, &mesg->data.fetch.cand) != 0) {
        mesg->status = HMESG_STATUS_FAIL;
        return -1;
    }

    /* Send best point information, if needed. */
    if (mesg->data.fetch.best.id < strategy_best.id) {
        mesg->data.fetch.best = HPOINT_INITIALIZER;
        if (hpoint_copy(&mesg->data.fetch.best, &strategy_best) < 0) {
            mesg->status = HMESG_STATUS_FAIL;
            return -1;
        }
    }
    else {
        mesg->data.fetch.best = HPOINT_INITIALIZER;
    }

    mesg->status = HMESG_STATUS_OK;
    return 0;
}

/*
 * Inform the search strategy of an observed performance associated with
 * a configuration point.
 */
int strategy_report(hmesg_t *mesg)
{
    if (mesg->data.report.cand.id != test->id)
        return -1; /* Rouge vertex. */
    test->perf = mesg->data.report.perf;

    if (nm_algorithm() != 0) {
        mesg->status = HMESG_STATUS_FAIL;
        mesg->data.string = "Internal error: Nelder-Mead algorithm failure.";
        return -1;
    }

    /* Update the best performing point, if necessary. */
    if (strategy_best_perf > mesg->data.report.perf) {
        strategy_best_perf = mesg->data.report.perf;
        if (hpoint_copy(&strategy_best, &mesg->data.report.cand) < 0) {
            mesg->status = HMESG_STATUS_FAIL;
            mesg->data.string = strerror(errno);
            return -1;
        }
    }

    if (state != SIMPLEX_STATE_CONVERGED)
        ++next_id;

    mesg->status = HMESG_STATUS_OK;
    return 0;
}

int nm_algorithm(void)
{
    do {
        if (state == SIMPLEX_STATE_CONVERGED)
            break;

        if (nm_next_state(test) != 0)
            return -1;

        if (state == SIMPLEX_STATE_REFLECT)
            check_convergence();

        if (nm_next_vertex(test) != 0)
            return -1;

    } while (vertex_outofbounds(test));

    return 0;
}

int nm_next_state(const vertex_t *input)
{
    int prev_worst;

    switch (state) {
    case SIMPLEX_STATE_INIT:
    case SIMPLEX_STATE_SHRINK:
        /* Simplex vertex performance value. */
        vertex_copy(base->vertex[index_curr], input);
        ++index_curr;

        if (index_curr == simplex_size) {
            simplex_update_centroid();
            state = SIMPLEX_STATE_REFLECT;
        }
        break;

    case SIMPLEX_STATE_REFLECT:
        if (input->perf < base->vertex[index_best]->perf) {
            /* Reflected test vertex has best known performance.
             * Replace the worst performing simplex vertex with the
             * reflected test vertex, and attempt expansion.
             */
            vertex_copy(base->vertex[index_worst], input);
            state = SIMPLEX_STATE_EXPAND;
        }
        else if (input->perf < base->vertex[index_worst]->perf) {
            /* Reflected test vertex performs better than the worst
             * known simplex vertex.  Replace the worst performing
             * simplex vertex with the reflected test vertex.
             */
            vertex_copy(base->vertex[index_worst], input);

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
        if (input->perf < base->vertex[index_best]->perf) {
            /* Expanded test vertex has best known performance.
             * Replace the worst performing simplex vertex with the
             * test vertex, and attempt expansion.
             */
            vertex_copy(base->vertex[index_worst], input);
        }
        simplex_update_centroid();
        state = SIMPLEX_STATE_REFLECT;
        break;

    case SIMPLEX_STATE_CONTRACT:
        if (input->perf < base->vertex[index_worst]->perf) {
            /* Contracted test vertex performs better than the worst
             * known simplex vertex.  Replace the worst performing
             * simplex vertex with the test vertex.
             */
            vertex_copy(base->vertex[index_worst], input);
            simplex_update_centroid();
            state = SIMPLEX_STATE_REFLECT;
        }
        else {
            /* Contracted test vertex has worst known performance.
             * Shrink the entire simplex towards the best point.
             */
            simplex_update_centroid();
            state = SIMPLEX_STATE_SHRINK;
        }
        break;

    default:
        return -1;
    }
    return 0;
}

int nm_next_vertex(vertex_t *output)
{
    switch (state) {
    case SIMPLEX_STATE_INIT:
        /* Test individual vertices of the initial simplex. */
        vertex_copy(output, base->vertex[index_curr]);
        break;

    case SIMPLEX_STATE_REFLECT:
        /* Test a vertex reflected from the worst performing vertex
         * through the centroid point. */
        vertex_transform(base->vertex[index_worst], centroid,
                         -reflect_coefficient, output);
        break;

    case SIMPLEX_STATE_EXPAND:
        /* Test a vertex that expands the reflected vertex even
         * further from the the centroid point. */
        vertex_transform(base->vertex[index_worst], centroid,
                         expand_coefficient, output);
        break;

    case SIMPLEX_STATE_CONTRACT:
        /* Test a vertex contracted from the worst performing vertex
         * towards the centroid point. */
        vertex_transform(base->vertex[index_worst], centroid,
                         contract_coefficient, output);
        break;

    case SIMPLEX_STATE_SHRINK:
        /* Shrink the entire simplex towards the best known vertex
         * thus far. */
        simplex_transform(base, base->vertex[index_best],
                          shrink_coefficient, base);
        index_curr = 0;
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
    double fv_err, sz_max, dist;

    if (simplex_collapsed(base) == 1)
        goto converged;

    fv_err = 0;
    for (i = 0; i < simplex_size; ++i) {
        fv_err += ((base->vertex[i]->perf - centroid->perf) *
                   (base->vertex[i]->perf - centroid->perf));
    }
    fv_err /= simplex_size;

    sz_max = 0;
    for (i = 0; i < simplex_size; ++i) {
        dist = vertex_dist(base->vertex[i], centroid);
        if (sz_max < dist)
            sz_max = dist;
    }

    if (fv_err < converge_fv_tol && sz_max < converge_sz_tol)
        goto converged;

    return;

  converged:
    state = SIMPLEX_STATE_CONVERGED;
    hcfg_set(sess->cfg, CFGKEY_STRATEGY_CONVERGED, "1");
}
