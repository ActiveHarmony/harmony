/*
 * Copyright 2003-2012 Jeffrey K. Hollingsworth
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

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------
 * Harmony structure that encapsulate configurations.
 *
 * These are stored as index values, and require an hsession_t to
 * determine the point's intended value.
 */
typedef struct hpoint {
    int id;
    int step; /* Only used for old Tcl-based strategies. */
    int *idx;
    int idx_cap;
} hpoint_t;

extern hpoint_t HPOINT_INITIALIZER;

int  hpoint_init(hpoint_t *pt, int cap);
void hpoint_fini(hpoint_t *pt);
int  hpoint_copy(hpoint_t *dst, const hpoint_t *src);
int  hpoint_serialize(char **buf, int *buflen, const hpoint_t *pt);
int  hpoint_deserialize(hpoint_t *pt, char *buf);

#ifdef __cplusplus
}
#endif

#endif
