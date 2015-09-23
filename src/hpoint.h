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

#ifndef __HPOINT_H__
#define __HPOINT_H__

#include "hval.h"
#include "hsig.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------
 * Harmony structure that represents a point within a search space.
 *
 */
typedef struct hpoint {
    unsigned id;
    hval_t*  val;
    int      len;

    void* owner;
} hpoint_t;
#define HPOINT_INITIALIZER {0}
extern const hpoint_t hpoint_zero;

int  hpoint_init(hpoint_t* pt, int n);
int  hpoint_copy(hpoint_t* dst, const hpoint_t* src);
void hpoint_scrub(hpoint_t* pt);
void hpoint_fini(hpoint_t* pt);

int  hpoint_align(hpoint_t* pt, hsig_t* sig);

int  hpoint_pack(char** buf, int* buflen, const hpoint_t* pt);
int  hpoint_unpack(hpoint_t* pt, char* buf);
int  hpoint_parse(hpoint_t* pt, hsig_t* sig, const char* buf);

#ifdef __cplusplus
}
#endif

#endif
