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

#include "hperf.h"
#include "hutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

hperf_t *hperf_alloc(int n)
{
    hperf_t *retval;

    retval = (hperf_t *) malloc(sizeof(hperf_t) + (n * sizeof(double)));
    if (retval) {
        retval->n = n;
        hperf_reset(retval);
    }
    return retval;
}

void hperf_reset(hperf_t *perf)
{
    int i;
    for (i = 0; i < perf->n; ++i)
        perf->p[i] = INFINITY;
}

int hperf_copy(hperf_t *dst, const hperf_t *src)
{
    memcpy(dst, src, sizeof(hperf_t) + (src->n * sizeof(double)));

    return 0;
}

hperf_t *hperf_clone(const hperf_t *perf)
{
    int newsize = sizeof(hperf_t) + (perf->n * sizeof(double));
    hperf_t *retval;

    retval = (hperf_t *) malloc(newsize);
    if (retval) {
        memcpy(retval, perf, newsize);
    }
    return retval;
}

void hperf_fini(hperf_t *perf)
{
    if (perf)
        free(perf);
}

int hperf_cmp(const hperf_t *a, const hperf_t *b)
{
    int i;
    double sum_a = 0.0, sum_b = 0.0;
    if (a->n != b->n)
        return a->n - b->n;

    for (i = 0; i < a->n; ++i) {
        sum_a += a->p[i];
        sum_b += b->p[i];
    }
    return (sum_a > sum_b) - (sum_a < sum_b);
}

double hperf_unify(const hperf_t *perf)
{
    int i;
    double retval = 0.0;

    for (i = 0; i < perf->n; ++i)
        retval += perf->p[i];
    return retval;
}

int hperf_serialize(char **buf, int *buflen, const hperf_t *perf)
{
    int i, count, total;

    count = snprintf_serial(buf, buflen, "perf: %d ", perf->n);
    if (count < 0) goto invalid;
    total = count;

    for (i = 0; i < perf->n; ++i) {
        count = snprintf_serial(buf, buflen, "%la ", perf->p[i]);
        if (count < 0) goto error;
        total += count;
    }
    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}

int hperf_deserialize(hperf_t **perf, char *buf)
{
    int i, n, count, total;

    for (i = 0; isspace(buf[i]); ++i);
    if (strncmp("perf:", buf + i, 5) != 0)
        goto invalid;
    total = i + 5;

    if (sscanf(buf + total, " %d%n", &n, &count) < 1)
        goto invalid;
    total += count;

    *perf = hperf_alloc(n);
    if (!*perf)
        goto error;

    for (i = 0; i < n; ++i) {
        if (sscanf(buf + total, " %la%n", &((*perf)->p[i]), &count) < 1)
            goto invalid;
        total += count;
    }

    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}
