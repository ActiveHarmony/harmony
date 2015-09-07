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

#ifndef __HSIG_H__
#define __HSIG_H__

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
typedef struct hsig {
    char* name;
    hrange_t* range;
    int range_len;
    int range_cap;
} hsig_t;
extern const hsig_t HSIG_INITIALIZER;

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
int  hsig_copy(hsig_t* dst, const hsig_t* src);
void hsig_fini(hsig_t* sig);
int  hsig_equal(const hsig_t* sig_a, const hsig_t* sig_b);
int  hsig_match(const hsig_t* sig_a, const hsig_t* sig_b);
int  hsig_name(hsig_t* sig, const char* name);
int  hsig_int(hsig_t* sig, const char* name,
              long min, long max, long step);
int  hsig_real(hsig_t* sig, const char* name,
               double min, double max, double step);
int  hsig_enum(hsig_t* sig, const char* name, const char* value);
int  hsig_serialize(char** buf, int* buflen, const hsig_t* sig);
int  hsig_deserialize(hsig_t* sig, char* buf);
int  hsig_parse(hsig_t* sig, const char* buf, const char** errptr);

#ifdef __cplusplus
}
#endif

#endif /* __HSIG_H__ */
