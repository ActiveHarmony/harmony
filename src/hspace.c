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
#define _XOPEN_SOURCE 500 // Needed for drand48().

#include "hspace.h"
#include "hrange.h"
#include "hpoint.h"
#include "hmesg.h"
#include "hutil.h"

#include <string.h> // For memset().

const hspace_t hspace_zero = HSPACE_INITIALIZER;

// Internal helper function prototypes.
static int       add_dim(hspace_t* sig, hrange_t* dim, const char** errptr);
static hrange_t* find_dim(hspace_t* sig, const char* name);

//
// Base structure management implementation.
//
int hspace_copy(hspace_t* dst, const hspace_t* src)
{
    int i = 0;
    hspace_scrub(dst);

    dst->id = src->id;
    dst->name = stralloc(src->name);
    if (!dst->name)
        goto error;

    if (dst->cap != src->cap) {
        hrange_t* newbuf = realloc(dst->dim, src->cap * sizeof(*src->dim));
        if (!newbuf)
            goto error;

        memset(newbuf + src->len, 0,
               (src->cap - src->len) * sizeof(*src->dim));

        dst->dim = newbuf;
        dst->cap = src->cap;
    }
    dst->len = src->len;

    while (i < src->len) {
        if (hrange_copy(&dst->dim[i], &src->dim[i]) != 0)
            goto error;
        ++i;
    }
    dst->owner = NULL;

    return 0;

  error:
    while (i > 0)
        hrange_fini(&dst->dim[--i]);
    free(dst->dim);
    free(dst->name);
    *dst = hspace_zero;

    return -1;
}

void hspace_scrub(hspace_t* space)
{
    space->id = 0;
    if (!hmesg_owner(space->owner, space->name)) {
        free(space->name);
        space->name = NULL;
    }

    for (int i = 0; i < space->len; ++i)
        hrange_fini(&space->dim[i]);
    space->len = 0;
}

void hspace_fini(hspace_t* space)
{
    hspace_scrub(space);
    free(space->dim);
}

//
// Search space definition implementation.
//
int hspace_name(hspace_t* space, const char* name)
{
    if (space->name && !hmesg_owner(space->owner, space->name))
        free(space->name);

    space->name = stralloc(name);
    if (!space->name)
        return -1;

    return 0;
}

int hspace_int(hspace_t* space, const char* name,
               long min, long max, long step)
{
    if (max < min || step < 1)
        return -1;

    long remain;
    remain  = max;
    remain -= min;
    remain %= step;

    hrange_t range;
    range.type = HVAL_INT;
    range.bounds.i.min = min;
    range.bounds.i.max = max - remain;
    range.bounds.i.step = step;
    range.name = stralloc(name);
    range.owner = NULL;
    if (!range.name)
        return -1;

    if (add_dim(space, &range, NULL) != 0) {
        free(range.name);
        return -1;
    }

    ++space->id;
    return 0;
}

int hspace_real(hspace_t* space, const char* name,
                double min, double max, double step)
{
    if (max < min || step < 0.0)
        return -1;

    if (step > 0.0) {
        double bins;

        bins  = max;
        bins -= min;
        bins /= step;

        max  = step;
        max *= (unsigned long) bins;
        max += min;
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

    if (add_dim(space, &range, NULL) != 0) {
        free(range.name);
        return -1;
    }

    ++space->id;
    return 0;
}

int hspace_enum(hspace_t* space, const char* name, const char* value)
{
    hrange_t* ptr = find_dim(space, name);
    if (ptr && ptr->type != HVAL_STR) {
            return -1;
    }
    else if (!ptr) {
        hrange_t range;
        range.type = HVAL_STR;
        range.bounds.e.set = NULL;
        range.bounds.e.len = 0;
        range.bounds.e.cap = 0;
        range.name = stralloc(name);
        range.owner = NULL;
        if (!range.name)
            return -1;

        if (add_dim(space, &range, NULL) != 0) {
            free(range.name);
            return -1;
        }
        ptr = space->dim + space->len - 1;
    }

    char* vcopy = stralloc(value);
    if (!vcopy)
        return -1;

    if (range_enum_add_value(&ptr->bounds.e, vcopy) != 0) {
        free(vcopy);
        return -1;
    }

    ++space->id;
    return 0;
}

//
// Search space comparison implementation.
//
int hspace_equal(const hspace_t* a, const hspace_t* b)
{
    if (strcmp(a->name, b->name) != 0)
        return 0;

    if (a->len != b->len)
        return 0;

    for (int i = 0; i < a->len; ++i) {
        hrange_t* range_a = &a->dim[i];
        hrange_t* range_b = &b->dim[i];

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

//
// Data transmission implementation.
//
int hspace_pack(char** buf, int* buflen, const hspace_t* space)
{
    int count, total;

    count = snprintf_serial(buf, buflen, " space:%u", space->id);
    if (count < 0) return -1;
    total = count;

    if (space->id) {
        count = printstr_serial(buf, buflen, space->name);
        if (count < 0) return -1;
        total += count;

        count = snprintf_serial(buf, buflen, " %d", space->len);
        if (count < 0) return -1;
        total += count;

        for (int i = 0; i < space->len; ++i) {
            count = hrange_pack(buf, buflen, &space->dim[i]);
            if (count < 0) return -1;
            total += count;
        }
    }
    return total;
}

int hspace_unpack(hspace_t* space, char* buf)
{
    int count, total;

    if (sscanf(buf, " space:%u%n", &space->id, &count) < 1)
        return -1;
    total = count;

    if (space->id) {
        count = scanstr_serial((const char**)&space->name, buf + total);
        if (count < 0) return -1;
        total += count;

        if (sscanf(buf + total, " %d%n", &space->len, &count) < 1)
            return -1;
        total += count;

        if (space->cap < space->len) {
            hrange_t* newbuf = realloc(space->dim,
                                       space->len * sizeof(*space->dim));
            if (!newbuf) return -1;
            space->dim = newbuf;
            space->cap = space->len;
        }

        for (int i = 0; i < space->len; ++i) {
            space->dim[i] = hrange_zero;
            space->dim[i].owner = space->owner;
            count = hrange_unpack(&space->dim[i], buf + total);
            if (count < 0) return -1;
            total += count;
        }
    }
    return total;
}

int hspace_parse(hspace_t* space, const char* buf, const char** errptr)
{
    const char* errstr;
    hrange_t range;

    int retval = hrange_parse(&range, buf, &errstr);
    if (retval < 0)
        goto error;
    if (retval == 0)
        return 0;

    if (add_dim(space, &range, &errstr) != 0) {
        hrange_fini(&range);
        goto error;
    }

    ++space->id;
    return 1;

  error:
    if (errptr) *errptr = errstr;
    return -1;
}

//
// Internal helper function implementation.
//
int add_dim(hspace_t* space, hrange_t* dim, const char** errptr)
{
    const char* errstr;

    if (find_dim(space, dim->name) != NULL) {
        errstr = "Search space dimension name conflict";
        goto error;
    }

    // Grow dimension array, if needed.
    if (space->len == space->cap) {
        if (array_grow(&space->dim, &space->cap, sizeof(*space->dim)) < 0) {
            errstr = "Could not grow search space dimension list";
            goto error;
        }
    }

    space->dim[ space->len++ ] = *dim;
    return 0;

  error:
    if (errptr) *errptr = errstr;
    return -1;
}

hrange_t* find_dim(hspace_t* space, const char* name)
{
    for (int i = 0; i < space->len; ++i)
        if (strcmp(name, space->dim[i].name) == 0)
            return &space->dim[i];

    return NULL;
}
