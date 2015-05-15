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

#ifndef __HPERF_H__
#define __HPERF_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hperf {
    int n;
#ifdef __cplusplus
    /* XXX - Hack to allow flexible array member in C++. */
    double p[1];
#else
    double p[];
#endif

} hperf_t;

hperf_t* hperf_alloc(int n);
void     hperf_reset(hperf_t* perf);
int      hperf_copy(hperf_t* src, const hperf_t* dst);
hperf_t* hperf_clone(const hperf_t* perf);
void     hperf_fini(hperf_t* perf);

int      hperf_cmp(const hperf_t* a, const hperf_t* b);
double   hperf_unify(const hperf_t* perf);

int hperf_serialize(char** buf, int* buflen, const hperf_t* perf);
int hperf_deserialize(hperf_t** perf, char* buf);

#ifdef __cplusplus
}
#endif

#endif
