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
#include "hpoint.h"
#include "hsession.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Forward definitions of internal helper functions. */
int    internal_vertex_min(vertex_t *v);
int    internal_vertex_max(vertex_t *v);
long   max_int(int_bounds_t *b);
double max_real(real_bounds_t *b);
int    max_str(str_bounds_t *b);
int    next_int(double *val, int_bounds_t *b);
int    next_real(double *val, real_bounds_t *b);
int    next_str(double *val, str_bounds_t *b);
long   random_int(int_bounds_t *b);
double random_real(real_bounds_t *b);
int    random_str(str_bounds_t *b);
long   closest_int(double val, double max, int_bounds_t *b);
double closest_real(double val, double max, real_bounds_t *b);
int    closest_str(double val, double max, str_bounds_t *b);
int    simplex_fit(simplex_t *s);

/* Global variables which must be set via libvertex_init(). */
int N;
hrange_t *range;
vertex_t *vmin;
vertex_t *vmax;
long sizeof_vertex;

int libvertex_init(hsignature_t *sig)
{
    N = sig->range_len;
    range = sig->range;
    sizeof_vertex = sizeof(vertex_t) + N * sizeof(double);

    free(vmin);
    vmin = vertex_alloc();
    if (!vmin || internal_vertex_min(vmin) != 0)
        return -1;

    free(vmax);
    vmax = vertex_alloc();
    if (!vmax || internal_vertex_max(vmax) != 0)
        return -1;

    return 0;
}

vertex_t *vertex_alloc()
{
    vertex_t *v;

    v = (vertex_t *) malloc(sizeof_vertex);
    if (!v)
        return NULL;
    v->id = 0;
    v->perf = INFINITY;

    return v;
}

int vertex_copy(vertex_t *dst, const vertex_t *src)
{
    if (dst != src)
        memcpy(dst, src, sizeof_vertex);
    return 0;
}

void vertex_free(vertex_t *v)
{
    free(v);
}

int vertex_min(vertex_t *v)
{
    return vertex_copy(v, vmin);
}

int internal_vertex_min(vertex_t *v)
{
    int i;

    for (i = 0; i < N; ++i) {
        switch (range[i].type) {
        case HVAL_INT:  v->term[i] = range[i].bounds.i.min; break;
        case HVAL_REAL: v->term[i] = range[i].bounds.r.min; break;
        case HVAL_STR:  v->term[i] = 0.0; break;
        default: return -1;
        }
    }
    v->perf = INFINITY;
    return 0;
}

int vertex_center(vertex_t *v)
{
    int i;

    vertex_copy(v, vmin);
    for (i = 0; i < N; ++i)
        v->term[i] += (vmax->term[i] - vmin->term[i]) / 2;

    v->perf = INFINITY;
    return 0;
}

int vertex_max(vertex_t *v)
{
    return vertex_copy(v, vmax);
}

int internal_vertex_max(vertex_t *v)
{
    int i;

    for (i = 0; i < N; ++i) {
        switch (range[i].type) {
        case HVAL_INT:  v->term[i] = max_int(&range[i].bounds.i);  break;
        case HVAL_REAL: v->term[i] = max_real(&range[i].bounds.r); break;
        case HVAL_STR:  v->term[i] = max_str(&range[i].bounds.s);  break;
        default: return -1;
        }
    }
    v->perf = INFINITY;
    return 0;
}

long max_int(int_bounds_t *b)
{
    long val;

    val  = b->max - b->min;
    val -= val % b->step;
    val += b->min;

    return val;
}

double max_real(real_bounds_t *b)
{
    double val;

    val = b->max;
    if (b->step > 0.0) {
        val -= b->min;
        val  = b->step * floor(val / b->step);
        val += b->min;
    }
    return val;
}

int max_str(str_bounds_t *b)
{
    return b->set_len - 1;
}

int vertex_incr(vertex_t *v)
{
    int i, of;

    v->perf = INFINITY;
    for (i = 0; i < N; ++i) {
        switch (range[i].type) {
        case HVAL_INT:  of = next_int(&v->term[i], &range[i].bounds.i);  break;
        case HVAL_REAL: of = next_real(&v->term[i], &range[i].bounds.r); break;
        case HVAL_STR:  of = next_str(&v->term[i], &range[i].bounds.s);  break;
        default: return -1;
        }
        if (!of)
            return 0;
    }
    return 1;
}

int next_int(double *val, int_bounds_t *b)
{
    *val += b->step;
    if (*val > b->max) {
        *val = b->min;
        return 1;
    }
    return 0;
}

int next_real(double *val, real_bounds_t *b)
{
    if (b->step > 0.0) {
        *val -= b->min;
        *val  = b->step * (round(*val / b->step) + 1);
        *val += b->min;
    }
    else {
        *val = nextafter(*val, INFINITY);
    }

    if (*val > b->max) {
        *val = b->min;
        return 1;
    }
    return 0;
}

int next_str(double *val, str_bounds_t *b)
{
    ++(*val);
    if (*val >= b->set_len) {
        *val = 0;
        return 1;
    }
    return 0;
}

int vertex_rand(vertex_t *v)
{
    int i;

    for (i = 0; i < N; ++i) {
        switch (range[i].type) {
        case HVAL_INT:  v->term[i] = random_int(&range[i].bounds.i);  break;
        case HVAL_REAL: v->term[i] = random_real(&range[i].bounds.r); break;
        case HVAL_STR:  v->term[i] = random_str(&range[i].bounds.s);  break;
        default: return -1;
        }
    }
    v->perf = INFINITY;
    return 0;
}

long random_int(int_bounds_t *b)
{
    long ridx;

    ridx = rand() % (((b->max - b->min) / b->step) + 1);
    return b->min + b->step * ridx;
}

double random_real(real_bounds_t *b)
{
    double rval;
    double ridx;

    rval = ((double)rand()) / RAND_MAX;
    if (b->step > 0.0) {
        ridx = round(rval * floor((b->max - b->min) / b->step));
        return b->min + b->step * ridx;
    }
    return b->min + rval * (b->max - b->min);
}

int random_str(str_bounds_t *b)
{
    return rand() % b->set_len;
}

double vertex_dist(const vertex_t *v1, const vertex_t *v2)
{
    int i;
    double dist;

    dist = 0;
    for (i = 0; i < N; ++i)
        dist += (v1->term[i] - v2->term[i]) * (v1->term[i] - v2->term[i]);

    return sqrt(dist);
}

void vertex_transform(const vertex_t *src, const vertex_t *wrt,
                      double coefficient, vertex_t *result)
{
    int i;

    for (i = 0; i < N; ++i)
        result->term[i] =
            coefficient * src->term[i] + (1.0 - coefficient) * wrt->term[i];
    result->perf = INFINITY;
}

int vertex_outofbounds(const vertex_t *v)
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

int vertex_regrid(vertex_t *v)
{
    int i;

    if (vertex_outofbounds(v))
        return -1;

    for (i = 0; i < N; ++i) {
        switch (range[i].type) {
        case HVAL_INT:
            v->term[i] = closest_int(v->term[i], vmax->term[i],
                                     &range[i].bounds.i);
            break;

        case HVAL_REAL:
            v->term[i] = closest_real(v->term[i], vmax->term[i],
                                      &range[i].bounds.r);
            break;

        case HVAL_STR:
            v->term[i] = closest_str(v->term[i], vmax->term[i],
                                     &range[i].bounds.s);
            break;

        default:
            return -1;
        }
    }

    v->id = 0;
    v->perf = INFINITY;
    return 0;
}

long closest_int(double val, double max, int_bounds_t *b)
{
    int neg;

    if (val < b->min)
        return b->min;

    if (val > max)
        return (long)max;

    neg = (val < 0.0);
    val = (val - b->min) / b->step;
    val = (neg ?  ceil(val - 0.5)
               : floor(val + 0.5));
    return b->min + b->step * (long)val;
}

double closest_real(double val, double max, real_bounds_t *b)
{
    int neg;

    if (val < b->min)
        return b->min;

    if (val > max)
        return max;

    if (b->step > 0.0) {
        neg = (val < 0.0);
        val = (val - b->min) / b->step;
        val = (neg ?  ceil(val - 0.5)
                   : floor(val + 0.5));
        val = b->min + b->step * val;
    }
    return val;
}

int closest_str(double val, double max, str_bounds_t *b)
{
    if (val < 0)   return 0;
    if (val > max) return (int)max;
    return lround(val);
}

int vertex_to_hpoint(const vertex_t *v, hpoint_t *result)
{
    int i;

    hpoint_init(result, N);
    for (i = 0; i < N; ++i) {
        result->val[i].type = range[i].type;
        switch (range[i].type) {
        case HVAL_INT:
            result->val[i].value.i = closest_int(v->term[i], vmax->term[i],
                                                 &range[i].bounds.i);
            break;

        case HVAL_REAL:
            result->val[i].value.r = closest_real(v->term[i], vmax->term[i],
                                                  &range[i].bounds.r);
            break;

        case HVAL_STR:
            result->val[i].value.s =
                range[i].bounds.s.set[closest_str(v->term[i], vmax->term[i],
                                                  &range[i].bounds.s)];
            break;

        default:
            return -1;
        }
    }

    result->id = v->id;
    return 0;
}

int vertex_from_hpoint(const hpoint_t *pt, vertex_t *result)
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

simplex_t *simplex_alloc(int m)
{
    int i;
    simplex_t *s;
    unsigned char *buf;

    if (m <= N)
        return NULL;

    s = (simplex_t *) malloc(sizeof(simplex_t) + m * sizeof(vertex_t *));
    if (!s)
        return NULL;
    s->len = m;

    buf = (unsigned char *) malloc(m * sizeof_vertex);
    for (i = 0; i < m; ++i)
        s->vertex[i] = (vertex_t *)(buf + i * sizeof_vertex);

    return s;
}

int simplex_copy(simplex_t *dst, const simplex_t *src)
{
    if (dst == src)
        return 0;

    if (dst->len != src->len)
        return -1;

    memcpy(dst->vertex[0], src->vertex[0], src->len * sizeof_vertex);
    return 0;
}

void simplex_free(simplex_t *s)
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
void simplex_centroid(const simplex_t *s, vertex_t *v)
{
    int i, j, count;

    count = 0;
    memset(v, 0, sizeof_vertex);

    for (i = 0; i < s->len; ++i) {
        v->perf += s->vertex[i]->perf;
        if (s->vertex[i]->id < 0)
            continue;

        for (j = 0; j < N; ++j)
            v->term[j] += s->vertex[i]->term[j];

        ++count;
    }

    v->perf /= s->len;
    for (j = 0; j < N; ++j)
        v->term[j] /= count;
}

void simplex_transform(const simplex_t *src, const vertex_t *wrt,
                       double coefficient, simplex_t *result)
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

int simplex_outofbounds(const simplex_t *s)
{
    int i;

    for (i = 0; i < s->len; ++i) {
        if (vertex_outofbounds(s->vertex[i]))
            return 1;
    }
    return 0;
}

int simplex_regrid(simplex_t *s)
{
    int i;

    for (i = 0; i < s->len; ++i) {
        if (vertex_regrid(s->vertex[i]) != 0)
            return -1;
    }
    return 0;
}

int simplex_from_vertex(const vertex_t *v, double percent, simplex_t *s)
{
    int i, ii, j;
    double sum, dim_len;

    percent /= 2.0;
    memset(s->vertex[0], 0, s->len * sizeof_vertex);
    for (i = 0; i < N; ++i) {
        sum = 0.0;
        for (ii = 0; ii < i; ++ii)
            sum = sum + s->vertex[i]->term[ii] * s->vertex[i]->term[ii];

        s->vertex[i]->term[i] = sqrt(1.0 - sum);

        for (j = i + 1; j < N + 1; ++j) {
            sum = 0.0;
            for (ii = 0; ii < i; ++ii)
                sum = sum + s->vertex[i]->term[ii] * s->vertex[j]->term[ii];
            s->vertex[j]->term[i] = ((-1.0 / ((double)N) - sum) /
                                     s->vertex[i]->term[i]);
        }
    }

    for (i = 0; i < s->len; ++i) {
        for (j = 0; j < N; ++j) {
            dim_len = (vmax->term[j] - vmin->term[j]) * percent;
            s->vertex[i]->term[j] = (v->term[j] +
                                     s->vertex[i]->term[j] * dim_len);
        }
    }

    if (simplex_fit(s) != 0)
        return -1;

    for (i = N + 1; i < s->len; ++i)
        vertex_rand(s->vertex[i]);

    return 0;
}

int simplex_from_vertex_fast(const vertex_t *v, double percent, simplex_t *s)
{
    int i, j;
    double dim_len;
    vertex_t *shift;

    percent /= 2.0;
    memset(s->vertex[0], 0, s->len * sizeof_vertex);
    vertex_copy(s->vertex[N], v);
    for (i = 0; i < N; ++i) {
        vertex_copy(s->vertex[i], v);
        dim_len = (vmax->term[i] - vmin->term[i]) * percent;
        s->vertex[i]->term[i] += dim_len;
        s->vertex[N]->term[i] -= ((sqrt(N + 1) - 1.0) * dim_len)/N;
    }

    shift = vertex_alloc();
    if (!shift)
        return -1;

    simplex_centroid(s, shift);
    for (j = 0; j < N; ++j)
        shift->term[j] = v->term[j] - shift->term[j];

    for (i = 0; i <= N; ++i)
        for (j = 0; j < N; ++j)
            s->vertex[i]->term[j] += shift->term[j];
    free(shift);

    if (simplex_fit(s) != 0)
        return -1;

    for (i = N + 1; i < s->len; ++i)
        vertex_rand(s->vertex[i]);

    return 0;
}

int simplex_fit(simplex_t *s)
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
            for (j = 0; j <= N; ++j) {
                if (s->vertex[j]->term[i] == lo)
                    s->vertex[j]->term[i]  = vmin->term[i];
                else
                    s->vertex[j]->term[i] += vmin->term[i] - lo;
            }
        }

        /* Shift all vertices down, if necessary. */
        if (hi > vmax->term[i]) {
            for (j = 0; j <= N; ++j) {
                if (s->vertex[j]->term[i] == hi)
                    s->vertex[j]->term[i]  = vmax->term[i];
                else
                    s->vertex[j]->term[i] -= hi - vmax->term[i];
            }
        }
    }
    return 0;
}

int simplex_collapsed(const simplex_t *s)
{
    static vertex_t *a, *b;
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
