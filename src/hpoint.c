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

#include "hpoint.h"
#include "hmesg.h"
#include "hutil.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

const hpoint_t hpoint_zero = HPOINT_INITIALIZER;

int hpoint_init(hpoint_t* point, int len)
{
    if (point->len != len) {
        hval_t *newbuf = realloc(point->val, sizeof(*point->val) * len);
        if (!newbuf)
            return -1;
        point->val = newbuf;
        point->len = len;
    }
    return 0;
}

int hpoint_copy(hpoint_t* dst, const hpoint_t* src)
{
    hpoint_scrub(dst);
    if (hpoint_init(dst, src->len) != 0)
        return -1;

    dst->id = src->id;
    memcpy(dst->val, src->val, sizeof(*src->val) * src->len);
    for (int i = 0; i < src->len; ++i) {
        if (src->val[i].type == HVAL_STR) {
            dst->val[i].value.s = stralloc(src->val[i].value.s);
            if (!dst->val[i].value.s)
                return -1;
        }
    }
    dst->owner = NULL;

    return 0;
}

void hpoint_scrub(hpoint_t* point)
{
    for (int i = 0; i < point->len; ++i) {
        if (point->val[i].type == HVAL_STR) {
            if (!hmesg_owner(point->owner, point->val[i].value.s))
                free((char*)point->val[i].value.s);
        }
        point->val[i].type = HVAL_UNKNOWN;
    }
    point->len = 0;
}

void hpoint_fini(hpoint_t* point)
{
    hpoint_scrub(point);
    free(point->val);
}

int hpoint_align(hpoint_t* point, hspace_t* space)
{
    if (point->len != space->len)
        return -1;

    for (int i = 0; i < point->len; ++i) {
        unsigned long index;

        index         = hrange_index(&space->dim[i], &point->val[i]);
        point->val[i] = hrange_value(&space->dim[i], index);

        if (point->val[i].type == HVAL_UNKNOWN)
            return -1;
    }
    return 0;
}

int hpoint_parse(hpoint_t* point, hspace_t* space, const char* buf)
{
    if (point->len != space->len) {
        hpoint_fini(point);
        hpoint_init(point, space->len);
    }

    for (int i = 0; i < point->len; ++i) {
        int span = hval_parse(&point->val[i], space->dim[i].type, buf);
        if (span < 0)
            return -1;
        buf += span;

        /* Skip whitespace and a single separator character. */
        while (isspace(*buf)) ++buf;
        if (*buf == ',' || *buf == ';')
            ++buf;
    }
    if (*buf != '\0')
        return -1;

    return 0;
}

int hpoint_pack(char** buf, int* buflen, const hpoint_t* point)
{
    int i, count, total;

    count = snprintf_serial(buf, buflen, " point:%u", point->id);
    if (count < 0) goto invalid;
    total = count;

    if (point->id) {
        count = snprintf_serial(buf, buflen, " %d", point->len);
        if (count < 0) goto invalid;
        total += count;

        for (i = 0; i < point->len; ++i) {
            count = hval_pack(buf, buflen, &point->val[i]);
            if (count < 0) goto error;
            total += count;
        }
    }
    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}

int hpoint_unpack(hpoint_t* point, char* buf)
{
    int count, total;

    if (sscanf(buf, " point:%u%n", &point->id, &total) < 1)
        goto invalid;

    if (point->id) {
        int newlen;
        if (sscanf(buf + total, " %d%n", &newlen, &count) < 1)
            goto invalid;
        total += count;

        if (point->len != newlen) {
            hval_t* newbuf = realloc(point->val, sizeof(*newbuf) * newlen);
            if (!newbuf)
                goto error;
            point->val = newbuf;
            point->len = newlen;
        }

        for (int i = 0; i < point->len; ++i) {
            count = hval_unpack(&point->val[i], buf + total);
            if (count < 0) goto error;
            total += count;
        }
    }
    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}
