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

#include "hsig.h"
#include "hrange.h"
#include "hmesg.h"
#include "hutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

const hsig_t hsig_zero = HSIG_INITIALIZER;

/* Internal helper function prototypes. */
static int       add_range(hsig_t* sig, hrange_t* range);
static hrange_t* find_range(hsig_t* sig, const char* name);

/*
 * Signature-related function implementations.
 */
int hsig_copy(hsig_t* dst, const hsig_t* src)
{
    int i = 0;
    hsig_scrub(dst);

    dst->id = src->id;
    dst->name = stralloc(src->name);
    if (!dst->name)
        goto error;

    if (dst->range_cap != src->range_cap) {
        hrange_t* newbuf = realloc(dst->range,
                                   src->range_cap * sizeof(*src->range));
        if (!newbuf)
            goto error;

        memset(newbuf + src->range_len, 0,
               (src->range_cap - src->range_len) * sizeof(*src->range));

        dst->range = newbuf;
        dst->range_cap = src->range_cap;
    }
    dst->range_len = src->range_len;

    while (i < src->range_len) {
        if (hrange_copy(&dst->range[i], &src->range[i]) != 0)
            goto error;
        ++i;
    }
    dst->owner = NULL;

    return 0;

  error:
    while (i > 0)
        hrange_fini(&dst->range[--i]);
    free(dst->range);
    free(dst->name);
    *dst = hsig_zero;

    return -1;
}

void hsig_scrub(hsig_t* sig)
{
    sig->id = 0;
    if (!hmesg_owner(sig->owner, sig->name)) {
        free(sig->name);
        sig->name = NULL;
    }

    for (int i = 0; i < sig->range_len; ++i)
        hrange_fini(&sig->range[i]);
    sig->range_len = 0;
}

void hsig_fini(hsig_t* sig)
{
    hsig_scrub(sig);
    free(sig->range);
}

int hsig_equal(const hsig_t* sig_a, const hsig_t* sig_b)
{
    if (strcmp(sig_a->name, sig_b->name) != 0)
        return 0;

    if (sig_a->range_len != sig_b->range_len)
        return 0;

    for (int i = 0; i < sig_a->range_len; ++i) {
        hrange_t* range_a = &sig_a->range[i];
        hrange_t* range_b = &sig_b->range[i];

        if (strcmp(range_a->name, range_b->name) != 0)
            return 0;
        if (range_a->type != range_b->type)
            return 0;

        switch (range_a->type) {
        case HVAL_INT:
            if (range_a->bounds.i.min  != range_b->bounds.i.min ||
                range_a->bounds.i.max  != range_b->bounds.i.max ||
                range_a->bounds.i.step != range_b->bounds.i.step)
                return 0;
            break;
        case HVAL_REAL:
            if (range_a->bounds.r.min  != range_b->bounds.r.min ||
                range_a->bounds.r.max  != range_b->bounds.r.max ||
                range_a->bounds.r.step != range_b->bounds.r.step)
                return 0;
            break;
        case HVAL_STR:
            if (range_a->bounds.e.len != range_b->bounds.e.len)
                return 0;
            for (int j = 0; j < range_a->bounds.e.len; ++j) {
                if (strcmp(range_a->bounds.e.set[j],
                           range_b->bounds.e.set[j]) != 0)
                    return 0;
            }
            break;
        default:
            return 0;
        }
    }
    return 1;
}

int hsig_name(hsig_t* sig, const char* name)
{
    if (sig->name && !hmesg_owner(sig->owner, sig->name))
        free(sig->name);

    sig->name = stralloc(name);
    if (!sig->name)
        return -1;

    return 0;
}

int hsig_int(hsig_t* sig, const char* name, long min, long max, long step)
{
    if (max < min || step < 1) {
        errno = EINVAL;
        return -1;
    }

    hrange_t range;
    range.type = HVAL_INT;
    range.bounds.i.min = min;
    range.bounds.i.max = max;
    range.bounds.i.step = step;
    range.name = stralloc(name);
    range.owner = NULL;
    if (!range.name)
        return -1;

    if (add_range(sig, &range) != 0) {
        free(range.name);
        return -1;
    }

    ++sig->id;
    return 0;
}

int hsig_real(hsig_t* sig, const char* name,
              double min, double max, double step)
{
    if (max < min || step <= 0.0) {
        errno = EINVAL;
        return -1;
    }

    hrange_t range;
    range.type = HVAL_REAL;
    range.bounds.r.min = min;
    range.bounds.r.max = max;
    range.bounds.r.step = step;
    range.name = stralloc(name);
    range.owner = NULL;
    if (!range.name)
        return -1;

    if (add_range(sig, &range) != 0) {
        free(range.name);
        return -1;
    }

    ++sig->id;
    return 0;
}

int hsig_enum(hsig_t* sig, const char* name, const char* value)
{
    hrange_t* ptr = find_range(sig, name);
    if (ptr) {
        if (ptr->type != HVAL_STR) {
            errno = EINVAL;
            return -1;
        }
    }
    else {
        hrange_t range;
        range.type = HVAL_STR;
        range.bounds.e.set = NULL;
        range.bounds.e.len = 0;
        range.bounds.e.cap = 0;
        range.name = stralloc(name);
        range.owner = NULL;
        if (!range.name)
            return -1;

        if (add_range(sig, &range) != 0) {
            free(range.name);
            return -1;
        }
        ptr = sig->range + sig->range_len - 1;
    }

    char* vcopy = stralloc(value);
    if (!vcopy || range_enum_add_string(&ptr->bounds.e, vcopy) != 0) {
        if (ptr->bounds.e.len == 0) {
            free(ptr->bounds.e.set);
            free(vcopy);
            --sig->range_len;
        }
        return -1;
    }

    ++sig->id;
    return 0;
}

int hsig_pack(char** buf, int* buflen, const hsig_t* sig)
{
    int count, total;

    count = snprintf_serial(buf, buflen, " sig:%u", sig->id);
    if (count < 0) goto invalid;
    total = count;

    if (sig->id) {
        count = printstr_serial(buf, buflen, sig->name);
        if (count < 0) goto error;
        total += count;

        count = snprintf_serial(buf, buflen, " %d", sig->range_len);
        if (count < 0) goto invalid;
        total += count;

        for (int i = 0; i < sig->range_len; ++i) {
            count = hrange_pack(buf, buflen, &sig->range[i]);
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

int hsig_unpack(hsig_t* sig, char* buf)
{
    int count, total;
    hrange_t* newbuf;

    if (sscanf(buf, " sig:%u%n", &sig->id, &count) < 1)
        goto invalid;
    total = count;

    if (sig->id) {
        count = scanstr_serial((const char**)&sig->name, buf + total);
        if (count < 0) goto invalid;
        total += count;

        if (sscanf(buf + total, " %d%n", &sig->range_len, &count) < 1)
            goto invalid;
        total += count;

        if (sig->range_cap < sig->range_len) {
            newbuf = realloc(sig->range, sizeof(hrange_t) * sig->range_len);
            if (!newbuf) goto error;
            sig->range = newbuf;
            sig->range_cap = sig->range_len;
        }

        for (int i = 0; i < sig->range_len; ++i) {
            sig->range[i] = hrange_zero;
            sig->range[i].owner = sig->owner;
            count = hrange_unpack(&sig->range[i], buf + total);
            if (count < 0) goto invalid;
            total += count;
        }
    }
    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}

int hsig_parse(hsig_t* sig, const char* buf, const char** errptr)
{
    hrange_t range = HRANGE_INITIALIZER;
    int id, idlen, bounds = 0, tail = 0;
    const char* errstr;

    while (isspace(*buf)) ++buf;
    if (*buf == '\0')
        return 0;

    sscanf(buf, " int %n%*[^=]%n=%n", &id, &idlen, &bounds);
    if (bounds) {
        int len = range_int_parse(&range.bounds.i, buf + bounds, &errstr);
        if (len == -1)
            goto error;

        range.type = HVAL_INT;
        tail += len;
        goto found;
    }

    sscanf(buf, " real %n%*[^=]%n=%n", &id, &idlen, &bounds);
    if (bounds) {
        int len = range_real_parse(&range.bounds.r, buf + bounds, &errstr);
        if (len == -1)
            goto error;

        range.type = HVAL_REAL;
        tail += len;
        goto found;
    }

    sscanf(buf, " enum %n%*[^=]%n=%n", &id, &idlen, &bounds);
    if (bounds) {
        int len = range_enum_parse(&range.bounds.e, buf + bounds, &errstr);
        if (len == -1)
            goto error;

        range.type = HVAL_STR;
        tail += len;
        goto found;
    }
    errstr = "Unknown tuning variable type";
    goto error;

  found:
    while (isspace(buf[idlen - 1])) --idlen;
    if (!valid_id(&buf[id], idlen - id)) {
        errstr = "Invalid variable name";
        goto error;
    }

    if (buf[bounds + tail] != '\0') {
        errstr = "Invalid trailing characters";
        goto error;
    }

    range.name = sprintf_alloc("%.*s", idlen - id, &buf[id]);
    if (!range.name) {
        errstr = "Cannot copy range name";
        goto error;
    }
    range.owner = NULL;

    if (add_range(sig, &range) != 0) {
        errstr = "Invalid range name";
        goto error;
    }

    ++sig->id;
    if (errptr) *errptr = NULL;
    return 1;

  error:
    hrange_fini(&range);
    if (errptr) *errptr = errstr;
    return -1;

}

/*
 * Internal helper function implementations.
 */
int add_range(hsig_t* sig, hrange_t* range)
{
    if (find_range(sig, range->name) != NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Grow range array, if needed. */
    if (sig->range_len == sig->range_cap) {
        if (array_grow(&sig->range, &sig->range_cap, sizeof(hrange_t)) < 0)
            return -1;
    }
    sig->range[ sig->range_len++ ] = *range;

    return 0;
}

hrange_t* find_range(hsig_t* sig, const char* name)
{
    for (int i = 0; i < sig->range_len; ++i)
        if (strcmp(name, sig->range[i].name) == 0)
            return &sig->range[i];

    return NULL;
}
