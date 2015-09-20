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

#ifndef __HRANGE_H__
#define __HRANGE_H__

#include "hval.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Integer domain range type.
 */
typedef struct range_int {
    long min;
    long max;
    long step;
} range_int_t;

unsigned long range_int_index(range_int_t* bounds, long val);
long          range_int_value(range_int_t* bounds, unsigned long idx);
long          range_int_nearest(range_int_t* bounds, long val);
int           range_int_parse(range_int_t* bounds, const char* buf,
                              const char** err);

/*
 * Real domain range type.
 */
typedef struct range_real {
    double min;
    double max;
    double step;
} range_real_t;

unsigned long range_real_index(range_real_t* bounds, double val);
double        range_real_value(range_real_t* bounds, unsigned long idx);
double        range_real_nearest(range_real_t* bounds, double val);
int           range_real_parse(range_real_t* bounds, const char* buf,
                               const char** err);

/*
 * Enumerated domain range type.
 */
typedef struct range_enum {
    char** set;
    int    len;
    int    cap;
} range_enum_t;

int           range_enum_add_string(range_enum_t* bounds, char* str);
unsigned long range_enum_index(range_enum_t* bounds, const char* val);
const char*   range_enum_value(range_enum_t* bounds, unsigned long idx);
int           range_enum_parse(range_enum_t* bounds, const char* buf,
                               const char** err);

/*
 * Range type to describe a single dimension of the tuning search space.
 */
typedef struct hrange {
    char* name;
    hval_type type;
    union {
        range_int_t  i;
        range_real_t r;
        range_enum_t e;
    } bounds;
} hrange_t;
#define HRANGE_INITIALIZER {0}
extern const hrange_t hrange_zero;

/* Harmony range functions */
void          hrange_fini(hrange_t* range);
int           hrange_copy(hrange_t* dst, const hrange_t* src);
unsigned long hrange_max_idx(hrange_t* range);

int hrange_serialize(char** buf, int* buflen, const hrange_t* range);
int hrange_deserialize(hrange_t* range, char* buf);

#ifdef __cplusplus
}
#endif

#endif /* __HRANGE_H__ */
