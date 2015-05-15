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

#ifndef __HSIGNATURE_H__
#define __HSIGNATURE_H__

#include "hval.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct int_bounds {
    long min;
    long max;
    long step;
} int_bounds_t;

typedef struct real_bounds {
    double min;
    double max;
    double step;
} real_bounds_t;

typedef struct str_bounds {
    char** set;
    int set_len;
    int set_cap;
} str_bounds_t;

/*
 * Bundle structure: Holds metadata about Harmony controlled variables
 * in the client application.
 */
typedef struct hrange {
    char* name;
    hval_type type;
    union {
        int_bounds_t  i;
        real_bounds_t r;
        str_bounds_t  s;
    } bounds;
} hrange_t;
extern const hrange_t HRANGE_INITIALIZER;

/*
 * Harmony session signature: Clients provide this on join to verify
 * compatibility with the session they wish to join.
 */
typedef struct hsignature {
    char* name;
    hrange_t* range;
    int range_len;
    int range_cap;
} hsignature_t;
extern const hsignature_t HSIGNATURE_INITIALIZER;

/* Harmony range functions */
unsigned long hrange_max_idx(hrange_t* range);

unsigned long hrange_int_max_idx(int_bounds_t* bound);
unsigned long hrange_int_index(int_bounds_t* bound, long val);
long          hrange_int_value(int_bounds_t* bound, unsigned long idx);
long          hrange_int_nearest(int_bounds_t* bound, long val);

unsigned long hrange_real_max_idx(real_bounds_t* bound);
unsigned long hrange_real_index(real_bounds_t* bound, double val);
double        hrange_real_value(real_bounds_t* bound, unsigned long idx);
double        hrange_real_nearest(real_bounds_t* bound, double val);

unsigned long hrange_str_max_idx(str_bounds_t* bound);
unsigned long hrange_str_index(str_bounds_t* bound, const char* val);
const char*   hrange_str_value(str_bounds_t* bound, unsigned long idx);

/* Harmony signature functions */
int  hsignature_copy(hsignature_t* dst, const hsignature_t* src);
void hsignature_fini(hsignature_t* sig);
int  hsignature_equal(const hsignature_t* sig_a, const hsignature_t* sig_b);
int  hsignature_match(const hsignature_t* sig_a, const hsignature_t* sig_b);
int  hsignature_name(hsignature_t* sig, const char* name);
int  hsignature_int(hsignature_t* sig, const char* name,
                    long min, long max, long step);
int  hsignature_real(hsignature_t* sig, const char* name,
                     double min, double max, double step);
int  hsignature_enum(hsignature_t* sig, const char* name, const char* value);
int  hsignature_serialize(char** buf, int* buflen, const hsignature_t* sig);
int  hsignature_deserialize(hsignature_t* sig, char* buf);

/* General helper function */
hrange_t* hrange_find(hsignature_t* sig, const char* name);
hrange_t* hrange_add(hsignature_t* sig, const char* name);

#ifdef __cplusplus
}
#endif

#endif /* __HSIGNATURE_H__ */
