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

#include "hperf.h"
#include "hutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

const hperf_t hperf_zero = HPERF_INITIALIZER;

int hperf_init(hperf_t* perf, int len)
{
    if (perf->len != len) {
        double* newbuf = realloc(perf->obj, sizeof(*perf->obj) * len);
        if (!newbuf)
            return -1;

        perf->obj = newbuf;
        perf->len = len;
    }
    return 0;
}

void hperf_reset(hperf_t* perf)
{
    for (int i = 0; i < perf->len; ++i)
        perf->obj[i] = HUGE_VAL;
}

int hperf_copy(hperf_t* dst, const hperf_t* src)
{
    if (dst->len != src->len) {
        if (hperf_init(dst, src->len) != 0)
            return -1;
    }
    memcpy(dst->obj, src->obj, sizeof(*dst->obj) * src->len);
    return 0;
}

void hperf_fini(hperf_t* perf)
{
    free(perf->obj);
}

int hperf_cmp(const hperf_t* a, const hperf_t* b)
{
    double sum_a = 0.0, sum_b = 0.0;
    if (a->len != b->len)
        return b->len - a->len;

    for (int i = 0; i < a->len; ++i) {
        sum_a += a->obj[i];
        sum_b += b->obj[i];
    }
    return (sum_a > sum_b) - (sum_a < sum_b);
}

double hperf_unify(const hperf_t* perf)
{
    int i;
    double retval = 0.0;

    for (i = 0; i < perf->len; ++i)
        retval += perf->obj[i];
    return retval;
}

int hperf_pack(char** buf, int* buflen, const hperf_t* perf)
{
    int i, count, total;

    count = snprintf_serial(buf, buflen, " perf:%d", perf->len);
    if (count < 0) goto invalid;
    total = count;

    for (i = 0; i < perf->len; ++i) {
        count = snprintf_serial(buf, buflen, " %la", perf->obj[i]);
        if (count < 0) goto error;
        total += count;
    }
    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}

int hperf_unpack(hperf_t* perf, char* buf)
{
    int count, total, len;

    if (sscanf(buf, " perf:%d%n", &len, &count) < 1)
        goto invalid;
    total = count;

    if (hperf_init(perf, len) != 0)
        goto error;

    for (int i = 0; i < len; ++i) {
        if (sscanf(buf + total, " %la%n", &perf->obj[i], &count) < 1)
            goto invalid;
        total += count;
    }
    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}
