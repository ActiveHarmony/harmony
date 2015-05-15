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

#ifndef __HVAL_H__
#define __HVAL_H__

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------
 * Harmony structures that encapsulate values and configurations
 */
typedef enum {
    HVAL_UNKNOWN = 0,
    HVAL_INT,  /* Integer domain value */
    HVAL_REAL, /* Real domain value    */
    HVAL_STR,  /* String value         */

    HVAL_MAX
} hval_type;

typedef struct hval {
    hval_type type;
    union {
        long i;
        double r;
        const char* s;
    } value;
} hval_t;

extern const hval_t HVAL_INITIALIZER;

int hval_parse(hval_t* val, const char* buf);
int hval_serialize(char** buf, int* buflen, const hval_t* val);
int hval_deserialize(hval_t* val, char* buf);

#ifdef __cplusplus
}
#endif

#endif
