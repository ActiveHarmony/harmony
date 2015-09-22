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

int hpoint_init(hpoint_t* pt, int len)
{
    pt->id = 0;
    if (pt->len != len) {
        hval_t *newbuf = realloc(pt->val, sizeof(*pt->val) * len);
        if (!newbuf)
            return -1;
        pt->val = newbuf;
        pt->len = len;
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

void hpoint_scrub(hpoint_t* pt)
{
    for (int i = 0; i < pt->len; ++i) {
        if (pt->val[i].type == HVAL_STR) {
            if (!hmesg_owner(pt->owner, pt->val[i].value.s))
                free((char*)pt->val[i].value.s);
        }
        pt->val[i].type = HVAL_UNKNOWN;
    }
    pt->len = 0;
}

void hpoint_fini(hpoint_t* pt)
{
    hpoint_scrub(pt);
    free(pt->val);
}

int hpoint_align(hpoint_t* pt, hsig_t* sig)
{
    int i;

    if (pt->len != sig->range_len)
        return -1;

    for (i = 0; i < pt->len; ++i) {
        hval_t* val = &pt->val[i];

        if (val->type != sig->range[i].type)
            return -1;

        switch (sig->range[i].type) {
        case HVAL_INT:
            val->value.i = range_int_nearest(&sig->range[i].bounds.i,
                                             val->value.i);
            break;

        case HVAL_REAL:
            val->value.r = range_real_nearest(&sig->range[i].bounds.r,
                                              val->value.r);
            break;

        case HVAL_STR: {
            unsigned long idx = range_enum_index(&sig->range[i].bounds.e,
                                                 val->value.s);
            if (!hmesg_owner(pt->owner, val->value.s))
                free((char*)val->value.s);
            val->value.s = stralloc(sig->range[i].bounds.e.set[idx]);
            break;
        }

        default:
            return -1;
        }
    }
    return 0;
}

int hpoint_parse(hpoint_t* pt, hsig_t* sig, const char* buf)
{
    if (pt->len != sig->range_len) {
        hpoint_fini(pt);
        hpoint_init(pt, sig->range_len);
    }

    for (int i = 0; i < pt->len; ++i) {
        pt->val[i].type = sig->range[i].type;

        int span = hval_parse(&pt->val[i], buf);
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

int hpoint_serialize(char** buf, int* buflen, const hpoint_t* pt)
{
    int i, count, total;

    count = snprintf_serial(buf, buflen, "hpoint:%u ", pt->id);
    if (count < 0) goto invalid;
    total = count;

    if (pt->id) {
        count = snprintf_serial(buf, buflen, "%d ", pt->len);
        if (count < 0) goto invalid;
        total += count;

        for (i = 0; i < pt->len; ++i) {
            count = hval_serialize(buf, buflen, &pt->val[i]);
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

int hpoint_deserialize(hpoint_t* pt, char* buf)
{
    int count, total;

    if (sscanf(buf, " hpoint:%u%n", &pt->id, &count) < 1)
        goto invalid;
    total = count;

    if (pt->id) {
        int newlen;
        if (sscanf(buf + total, " %d%n", &newlen, &count) < 1)
            goto invalid;
        total += count;

        if (pt->len != newlen) {
            hval_t* newbuf = realloc(pt->val, sizeof(*newbuf) * newlen);
            if (!newbuf)
                goto error;
            pt->val = newbuf;
            pt->len = newlen;
        }

        for (int i = 0; i < pt->len; ++i) {
            count = hval_deserialize(&pt->val[i], buf + total);
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
