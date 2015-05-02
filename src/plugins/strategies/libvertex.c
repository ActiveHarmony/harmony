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

#include "libvertex.h"
#include "hsignature.h"
#include "hpoint.h"
#include "hperf.h"
#include "session-core.h"
#include "hcfg.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Forward definitions of internal helper functions. */
int    internal_vertex_min(vertex_t* v);
int    internal_vertex_max(vertex_t* v);
long   max_int(int_bounds_t* b);
double max_real(real_bounds_t* b);
int    max_str(str_bounds_t* b);
int    simplex_fit(simplex_t* s);
void   unit_simplex(simplex_t* s);
int    rotate_simplex(simplex_t* s);
int    rotate(simplex_t* s);
int    rparam_sort(const void* _a, const void* _b);

/* Global variables which must be set via libvertex_init(). */
static int N, P;
static hrange_t* range;
static vertex_t* vmin;
static vertex_t* vmax;
static long sizeof_vertex;

int libvertex_init(hsignature_t* sig)
{
    if (range != sig->range) {
        range = sig->range;
        N = sig->range_len;
        P = hcfg_int(session_cfg, CFGKEY_PERF_COUNT);
        sizeof_vertex = sizeof(vertex_t) + N * sizeof(double);

        free(vmin);
        vmin = vertex_alloc();
        if (!vmin || internal_vertex_min(vmin) != 0)
            return -1;
        hperf_reset(vmin->perf);

        free(vmax);
        vmax = vertex_alloc();
        if (!vmax || internal_vertex_max(vmax) != 0)
            return -1;
        hperf_reset(vmax->perf);
    }
    return 0;
}

vertex_t* vertex_alloc()
{
    vertex_t* v;

    v = malloc(sizeof_vertex);
    if (!v)
        return NULL;
    v->id = 0;
    v->perf = hperf_alloc(P);
    if (!v->perf) {
        free(v);
        return NULL;
    }
    return v;
}

void vertex_reset(vertex_t* v)
{
    v->id = 0;
    hperf_reset(v->perf);
}

int vertex_copy(vertex_t* dst, const vertex_t* src)
{
    if (dst != src) {
        dst->id = src->id;
        memcpy(dst->term, src->term, N * sizeof(double));
        hperf_copy(dst->perf, src->perf);
    }
    return 0;
}

void vertex_free(vertex_t* v)
{
    hperf_fini(v->perf);
    free(v);
}

const vertex_t* vertex_min(void)
{
    return vmin;
}

int internal_vertex_min(vertex_t* v)
{
    int i;

    vertex_reset(v);
    for (i = 0; i < N; ++i) {
        switch (range[i].type) {
        case HVAL_INT:  v->term[i] = range[i].bounds.i.min; break;
        case HVAL_REAL: v->term[i] = range[i].bounds.r.min; break;
        case HVAL_STR:  v->term[i] = 0.0; break;
        default: return -1;
        }
    }
    return 0;
}

const vertex_t* vertex_max(void)
{
    return vmax;
}

int internal_vertex_max(vertex_t* v)
{
    int i;

    vertex_reset(v);
    for (i = 0; i < N; ++i) {
        int idx = hrange_max_idx(&range[i]);

        switch (range[i].type) {
        case HVAL_INT:
            v->term[i] = hrange_int_value(&range[i].bounds.i, idx);
            break;

        case HVAL_REAL:
            if (range[i].bounds.r.step > 0.0)
                v->term[i] = hrange_real_value(&range[i].bounds.r, idx);
            else
                v->term[i] = range[i].bounds.r.max;
            break;

        case HVAL_STR:
            v->term[i] = range[i].bounds.s.set_len - 1;
            break;

        default:
            return -1;
        }
    }
    return 0;
}

int vertex_center(vertex_t* v)
{
    int i;

    vertex_copy(v, vmin);
    for (i = 0; i < N; ++i)
        v->term[i] += (vmax->term[i] - vmin->term[i]) / 2;

    hperf_reset(v->perf);
    return 0;
}

int vertex_percent(vertex_t* v, double percent)
{
    int i;
    for (i = 0; i < N; ++i)
        v->term[i] = (vmax->term[i] - vmin->term[i]) * percent;

    hperf_reset(v->perf);
    return 0;
}

int vertex_rand(vertex_t* v)
{
    return vertex_rand_trim(v, 0.0);
}

int vertex_rand_trim(vertex_t* v, double trim_percentage)
{
    int i;
    double rval;

    if (vertex_percent(v, trim_percentage) != 0)
        return -1;

    for (i = 0; i < N; ++i) {
        switch (range[i].type) {
        case HVAL_INT: {
            int_bounds_t* b = &range[i].bounds.i;
            rval = (b->max - b->min - v->term[i]) * drand48();
            rval += b->min + (v->term[i] / 2);
            v->term[i] = hrange_int_nearest(b, rval);
            break;
        }
        case HVAL_REAL: {
            real_bounds_t* b = &range[i].bounds.r;
            rval = (b->max - b->min - v->term[i]) * drand48();
            rval += b->min + (v->term[i] / 2);
            v->term[i] = hrange_real_nearest(b, rval);
            break;
        }
        case HVAL_STR: {
            str_bounds_t* b = &range[i].bounds.s;
            rval = (b->set_len - 1 - v->term[i]) * drand48();
            rval += v->term[i] / 2;
            v->term[i] = rval;
            break;
        }
        default:
            return -1;
        }
    }
    hperf_reset(v->perf);
    return 0;
}

double vertex_dist(const vertex_t* v1, const vertex_t* v2)
{
    int i;
    double dist;

    dist = 0;
    for (i = 0; i < N; ++i)
        dist += (v1->term[i] - v2->term[i]) * (v1->term[i] - v2->term[i]);

    return sqrt(dist);
}

void vertex_transform(const vertex_t* src, const vertex_t* wrt,
                      double coefficient, vertex_t* result)
{
    int i;

    for (i = 0; i < N; ++i)
        result->term[i] =
            coefficient * src->term[i] + (1.0 - coefficient) * wrt->term[i];
    hperf_reset(result->perf);
}

int vertex_outofbounds(const vertex_t* v)
{
    int i, out;

    for (i = 0; i < N; ++i) {
        switch (range[i].type) {
        case HVAL_INT:  out = (v->term[i] < range[i].bounds.i.min); break;
        case HVAL_REAL: out = (v->term[i] < range[i].bounds.r.min); break;
        case HVAL_STR:  out = (v->term[i] < 0.0); break;
        default: return -1;
        }
        if (out || v->term[i] > vmax->term[i])
            return 1;
    }
    return 0;
}

int vertex_regrid(vertex_t* v)
{
    int i;

    if (vertex_outofbounds(v))
        return -1;

    for (i = 0; i < N; ++i) {
        switch (range[i].type) {
        case HVAL_INT:
            v->term[i] = hrange_int_nearest(&range[i].bounds.i, v->term[i]);
            break;

        case HVAL_REAL:
            v->term[i] = hrange_real_nearest(&range[i].bounds.r, v->term[i]);
            break;

        case HVAL_STR:
            if (v->term[i] < 0.0)
                v->term[i] = 0.0;
            if (v->term[i] > range[i].bounds.s.set_len - 1)
                v->term[i] = range[i].bounds.s.set_len - 1;
            v->term[i] = (unsigned long)(v->term[i] + 0.5);
            break;

        default:
            return -1;
        }
    }

    vertex_reset(v);
    return 0;
}

int vertex_to_hpoint(const vertex_t* v, hpoint_t* result)
{
    int i;

    hpoint_init(result, N);
    for (i = 0; i < N; ++i) {
        hval_t* val = &result->val[i];

        val->type = range[i].type;
        switch (range[i].type) {
        case HVAL_INT:
            val->value.i = hrange_int_nearest(&range[i].bounds.i, v->term[i]);
            break;
        case HVAL_REAL:
            val->value.r = hrange_real_nearest(&range[i].bounds.r, v->term[i]);
            break;
        case HVAL_STR: {
            unsigned long idx = v->term[i];

            idx = v->term[i] + 0.5;
            if (idx < 0)
                idx = 0;
            if (idx > range[i].bounds.s.set_len - 1)
                idx = range[i].bounds.s.set_len - 1;

            val->value.s = hrange_str_value(&range[i].bounds.s, idx);
            break;
        }
        default:
            return -1;
        }
    }

    result->id = v->id;
    return 0;
}

int vertex_from_hpoint(const hpoint_t* pt, vertex_t* result)
{
    int i, j;

    for (i = 0; i < N; ++i) {
        switch (range[i].type) {
        case HVAL_INT:  result->term[i] = pt->val[i].value.i; break;
        case HVAL_REAL: result->term[i] = pt->val[i].value.r; break;
        case HVAL_STR:
            for (j = 0; j < range[i].bounds.s.set_len; ++j) {
                if (strcmp(pt->val[i].value.s, range[i].bounds.s.set[j]) == 0)
                    break;
            }
            result->term[i] = j;
            break;
        default:
            return -1;
        }
    }
    return 0;
}

int vertex_from_string(const char* str, hsignature_t* sig, vertex_t* result)
{
    int retval = 0;
    hpoint_t pt = HPOINT_INITIALIZER;

    if (!str) {
        session_error("Cannot convert null string to vertex");
        return -1;
    }

    if (hpoint_init(&pt, sig->range_len) != 0) {
        session_error("Error initializing temporary hpoint");
        return -1;
    }

    if (hpoint_parse(&pt, sig, str) != 0) {
        session_error("Error parsing point string");
        retval = -1;
        goto cleanup;
    }

    if (hpoint_align(&pt, sig) != 0) {
        session_error("Error aligning point to session signature");
        retval = -1;
        goto cleanup;
    }

    if (vertex_from_hpoint(&pt, result) != 0) {
        session_error("Error converting point to vertex");
        retval = -1;
    }

  cleanup:
    hpoint_fini(&pt);
    return retval;
}

simplex_t* simplex_alloc(int m)
{
    int i;
    simplex_t* s;
    unsigned char* buf;

    if (m <= N)
        return NULL;

    s = malloc(sizeof(simplex_t) + m * sizeof(vertex_t*));
    if (!s)
        return NULL;
    s->len = m;

    buf = malloc(m * sizeof_vertex);
    for (i = 0; i < m; ++i) {
        s->vertex[i] = (vertex_t*)(buf + i * sizeof_vertex);
        s->vertex[i]->perf = hperf_alloc(P);
        if (!s->vertex[i]->perf)
            return NULL;

        vertex_reset(s->vertex[i]);
    }

    return s;
}

void simplex_reset(simplex_t* s)
{
    int i;
    for (i = 0; i < s->len; ++i)
        vertex_reset(s->vertex[i]);
}

int simplex_copy(simplex_t* dst, const simplex_t* src)
{
    int i;

    if (dst == src)
        return 0;

    if (dst->len != src->len)
        return -1;

    for (i = 0; i < src->len; ++i)
        vertex_copy(dst->vertex[i], src->vertex[i]);

    return 0;
}

void simplex_free(simplex_t* s)
{
    if (s) {
        free(s->vertex);
        free(s);
    }
}

/*
 * General centroid formula for vertex points x1 through xN:
 *
 *     sum(x1, x2, x3, ... , xN) / N
 *
 * This function will ignore any vertices with id < 0.
 */
void simplex_centroid(const simplex_t* s, vertex_t* v)
{
    int i, j, count;

    count = 0;
    memset(v->term, 0, N * sizeof(double));
    memset(v->perf->p, 0, P * sizeof(double));

    for (i = 0; i < s->len; ++i) {
        if (s->vertex[i]->id < 0)
            continue;

        for (j = 0; j < N; ++j)
            v->term[j] += s->vertex[i]->term[j];

        for (j = 0; j < P; ++j)
            v->perf->p[j] += s->vertex[i]->perf->p[j];

        ++count;
    }

    for (j = 0; j < N; ++j)
        v->term[j] /= count;

    for (j = 0; j < P; ++j)
        v->perf->p[j] /= count;
}

void simplex_transform(const simplex_t* src, const vertex_t* wrt,
                       double coefficient, simplex_t* result)
{
    int i;

    for (i = 0; i < src->len; ++i) {
        if (src->vertex[i] == wrt) {
            vertex_copy(result->vertex[i], src->vertex[i]);
            continue;
        }
        vertex_transform(src->vertex[i], wrt, coefficient, result->vertex[i]);
    }
}

int simplex_outofbounds(const simplex_t* s)
{
    int i;

    for (i = 0; i < s->len; ++i) {
        if (vertex_outofbounds(s->vertex[i]))
            return 1;
    }
    return 0;
}

int simplex_regrid(simplex_t* s)
{
    int i;

    for (i = 0; i < s->len; ++i) {
        if (vertex_regrid(s->vertex[i]) != 0)
            return -1;
    }
    return 0;
}

int simplex_from_vertex(const vertex_t* v, double percent, simplex_t* s)
{
    int i, j;
    vertex_t* size = vertex_alloc();

    if (!size)
        return -1;

    if (vertex_percent(size, percent / 2) != 0) {
        vertex_free(size);
        return -1;
    }

    /* Generate a simplex of unit length. */
    unit_simplex(s);

    /* Pseudo-randomly rotate the unit simplex. */
    rotate(s);

    /* Grow and translate the simplex. */
    for (i = 0; i <= N; ++i) {
        for (j = 0; j < N; ++j) {
            s->vertex[i]->term[j] *= size->term[j];
            s->vertex[i]->term[j] += v->term[j];
        }
    }

    if (simplex_fit(s) != 0) {
        vertex_free(size);
        return -1;
    }

    /* Fill any remaining points with random vertices. */
    for (i = N+1; i < s->len; ++i)
        vertex_rand(s->vertex[i]);

    vertex_free(size);
    return 0;
}

int simplex_fit(simplex_t* s)
{
    int i, j;
    double lo, hi;

    /* Process each dimension in isolation. */
    for (i = 0; i < N; ++i) {
        /* Find the lowest and highest simplex value. */
        lo = s->vertex[0]->term[i];
        hi = s->vertex[0]->term[i];
        for (j = 1; j <= N; ++j) {
            if (lo > s->vertex[j]->term[i])
                lo = s->vertex[j]->term[i];
            if (hi < s->vertex[j]->term[i])
                hi = s->vertex[j]->term[i];
        }

        /* Verify that simplex is not larger than the search space. */
        if ((hi - lo) > (vmax->term[i] - vmin->term[i]))
            return -1;

        /* Shift all vertices up, if necessary. */
        if (lo < vmin->term[i]) {
            double shift = vmin->term[i] - lo;
            for (j = 0; j <= N; ++j)
                s->vertex[j]->term[i] += shift;
        }

        /* Shift all vertices down, if necessary. */
        if (hi > vmax->term[i]) {
            double shift = hi - vmax->term[i];
            for (j = 0; j <= N; ++j)
                s->vertex[j]->term[i] -= shift;
        }
    }
    return 0;
}

int simplex_collapsed(const simplex_t* s)
{
    static vertex_t* a;
    static vertex_t* b;
    int i, j;

    if (!a || !b || a->id != N) {
        free(a);
        free(b);
        a = vertex_alloc();
        b = vertex_alloc();
        if (!a || !b)
            return -1;
    }
    vertex_copy(a, s->vertex[0]);
    vertex_regrid(a);
    a->id = N;

    for (i = 1; i < s->len; ++i) {
        vertex_copy(b, s->vertex[i]);
        vertex_regrid(b);
        for (j = 0; j < N; ++j) {
            if (a->term[j] != b->term[j])
                return 0;
        }
    }
    return 1;
}

/*
 * Calculate a unit-length simplex about the origin.
 */
void unit_simplex(simplex_t* s)
{
    int i, j, k;
    double sum;

    // Clear the simplex.
    for (i = 0; i < s->len; ++i) {
        s->vertex[i]->id = 0;
        memset(s->vertex[i]->term, 0, N * sizeof(double));
        hperf_reset(s->vertex[i]->perf);
    }

    // Calculate values for the first N+1 vertices.
    for (i = 0; i < N; ++i) {
        sum = 0.0;
        for (j = 0; j < i; ++j)
            sum += s->vertex[i]->term[j] * s->vertex[i]->term[j];
        s->vertex[i]->term[i] = sqrt(1.0 - sum);

        for (j = i+1; j <= N; ++j) {
            sum = 0.0;
            for (k = 0; k < i; ++k)
                sum += s->vertex[i]->term[k] * s->vertex[j]->term[k];
            s->vertex[j]->term[i] = ((-1.0 / ((double)N) - sum) /
                                     s->vertex[i]->term[i]);
        }
    }
}

/*
 * Data structure used for simplex rotation.
 */
typedef struct {
    unsigned long order;
    int x;
    int y;
} rparam_t;

/*
 * Rotates a simplex about the origin.
 */
int rotate(simplex_t* s)
{
    int i, j, k, combos = (N * (N - 1)) / 2;
    rparam_t* rp;

    rp = malloc(combos * sizeof(rparam_t));
    if (!rp)
        return -1;

    /* Generate a random ordering for all pairs of terms. */
    i = 0;
    for (j = 0; j < N-1; ++j) {
        for (k = j+1; k < N; ++k) {
            rp[i].order  = lrand48();
            rp[i].x = j;
            rp[i].y = k;
            ++i;
        }
    }
    qsort(rp, combos, sizeof(rparam_t), rparam_sort);

    /* Rotate each pair of terms by a random angle. */
    for (i = 0; i < combos; ++i) {
        double theta = drand48() * (2 * M_PI);

        for (j = 0; j < s->len; ++j) {
            double term_x = s->vertex[j]->term[rp[i].x];
            double term_y = s->vertex[j]->term[rp[i].y];

            s->vertex[j]->term[rp[i].x] = (term_x * cos(theta) -
                                           term_y * sin(theta));
            s->vertex[j]->term[rp[i].y] = (term_x * sin(theta) +
                                           term_y * cos(theta));
        }
    }

    free(rp);
    return 0;
}

int rparam_sort(const void* _a, const void* _b)
{
    const rparam_t* a = _a;
    const rparam_t* b = _b;

    return (a->order - b->order);
}
