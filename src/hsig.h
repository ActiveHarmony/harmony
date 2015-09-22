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

#include "hrange.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Harmony session signature: Clients provide this on join to verify
 * compatibility with the session they wish to join.
 */
typedef struct hsig {
    char*     name;
    hrange_t* range;
    int       range_len;
    int       range_cap;

    void*     owner;
} hsig_t;
#define HSIG_INITIALIZER {0}
extern const hsig_t hsig_zero;

/* Harmony signature functions */
int  hsig_copy(hsig_t* dst, const hsig_t* src);
void hsig_scrub(hsig_t* sig);
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
