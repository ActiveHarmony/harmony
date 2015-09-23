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
#include "hmesg.h"
#include "hutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

const hrange_t hrange_zero = HRANGE_INITIALIZER;

/* Internal helper function prototypes. */
static int           range_enum_copy(range_enum_t* dst,
                                     const range_enum_t* src);
static unsigned long range_int_max_idx(range_int_t* bounds);
static unsigned long range_real_max_idx(range_real_t* bounds);
static unsigned long range_enum_max_idx(range_enum_t* bounds);
static int           copy_token(const char* buf, char **token,
                                const char** errptr);

/*
 * Integer domain range related function implementations.
 */
unsigned long range_int_index(range_int_t* bounds, long val)
{
    unsigned long max_idx = range_int_max_idx(bounds);
    double idx;

    idx  = val - bounds->min;
    idx /= bounds->step;
    idx += 0.5;

    if (idx < 0.0)
        idx = 0.0;
    if (idx > max_idx)
        idx = max_idx;

    return (unsigned long)idx;
}

long range_int_value(range_int_t* bounds, unsigned long idx)
{
    unsigned long max_idx = range_int_max_idx(bounds);

    if (idx > max_idx)
        idx = max_idx;

    return bounds->min + idx * bounds->step;
}

long range_int_nearest(range_int_t* bounds, long val)
{
    return bounds->min + range_int_index(bounds, val) * bounds->step;
}

int range_int_parse(range_int_t* bounds, const char* buf, const char** err)
{
    *bounds = (range_int_t){LONG_MIN, LONG_MIN, 1};

    int tail = 0;
    sscanf(buf, " min: %ld max: %ld %nstep: %ld %n",
           &bounds->min,
           &bounds->max, &tail,
           &bounds->step, &tail);

    if (!tail) {
        if (bounds->min == LONG_MIN)
            *err = "Invalid integer-domain minimum range value";
        else
            *err = "Invalid integer-domain maximum range value";
        return -1;
    }
    return tail;
}

/*
 * Real domain range related function implementations.
 */
unsigned long range_real_index(range_real_t* bounds, double val)
{
    unsigned long max_idx = range_real_max_idx(bounds);
    double idx;

    idx  = val - bounds->min;
    idx /= bounds->step;
    idx += 0.5;

    if (idx < 0.0)
        idx = 0.0;
    if (idx > max_idx)
        idx = max_idx;

    return (unsigned long)idx;
}

double range_real_value(range_real_t* bounds, unsigned long idx)
{
    unsigned long max_idx = range_real_max_idx(bounds);

    if (idx > max_idx)
        idx = max_idx;

    return bounds->min + idx * bounds->step;
}

double range_real_nearest(range_real_t* bounds, double val)
{
    if (bounds->step > 0.0) {
        val = bounds->min + range_real_index(bounds, val) * bounds->step;
    }
    else {
        if (val < bounds->min)
            val = bounds->min;
        if (val > bounds->max)
            val = bounds->max;
    }
    return val;
}

int range_real_parse(range_real_t* bounds, const char* buf, const char** err)
{
    *bounds = (range_real_t){NAN, NAN, NAN};

    int tail = 0;
    sscanf(buf, " min: %lf max: %lf step: %lf %n",
           &bounds->min,
           &bounds->max,
           &bounds->step, &tail);

    if (!tail) {
        if (isnan(bounds->min))
            *err = "Invalid real-domain minimum range value";
        else if (isnan(bounds->max))
            *err = "Invalid real-domain minimum range value";
        else
            *err = "Invalid real-domain step value";
        return -1;
    }
    return tail;
}

/*
 * Enumerated domain range related function implementations.
 */
int range_enum_add_string(range_enum_t* bounds, char* str)
{
    for (int i = 0; i < bounds->len; ++i) {
        if (strcmp(bounds->set[i], str) == 0)
            return -1;
    }

    if (bounds->len == bounds->cap) {
        if (array_grow(&bounds->set, &bounds->cap, sizeof(*bounds->set)) != 0)
            return -1;
    }
    bounds->set[ bounds->len++ ] = str;

    return 0;
}

unsigned long range_enum_index(range_enum_t* bounds, const char* val)
{
    unsigned long idx;

    for (idx = 0; idx < bounds->len; ++idx) {
        if (strcmp(bounds->set[idx], val) == 0)
            break;
    }
    if (idx >= bounds->len)
        idx = 0;

    return idx;
}

const char* range_enum_value(range_enum_t* bounds, unsigned long idx)
{
    if (idx > bounds->len - 1)
        idx = bounds->len - 1;

    return bounds->set[idx];
}

int range_enum_parse(range_enum_t* bounds, const char* buf, const char** err)
{
    *bounds = (range_enum_t){NULL, 0, 0};

    int tail = 0;
    while (buf[tail]) {
        char* token;
        int len = copy_token(buf + tail, &token, err);
        if (len == -1)
            return -1;

        tail += len;
        if (token && range_enum_add_string(bounds, token) != 0) {
            *err = "Invalid enumerated-domain string value";
            free(token);
            return -1;
        }
        if (!token && buf[tail] != '\0') {
            *err = "Empty enumerated-domain value";
            return -1;
        }

        while (isspace(buf[tail])) ++tail;
        if (buf[tail] == ',') ++tail;
    }
    if (bounds->len == 0) {
        *err = "No enumerated string tokens found";
        return -1;
    }
    return tail;
}

/*
 * Overall range related function implementations.
 */
void hrange_fini(hrange_t* range)
{
    if (!hmesg_owner(range->owner, range->name))
        free(range->name);

    if (range->type == HVAL_STR) {
        for (int i = 0; i < range->bounds.e.len; ++i) {
            if (!hmesg_owner(range->owner, range->bounds.e.set[i]))
                free(range->bounds.e.set[i]);
        }
        free(range->bounds.e.set);
    }
}

int hrange_copy(hrange_t* dst, const hrange_t* src)
{
    dst->name = stralloc(src->name);
    if (!dst->name)
        return -1;

    dst->type = src->type;
    switch (dst->type) {
    case HVAL_INT:  dst->bounds.i = src->bounds.i; break;
    case HVAL_REAL: dst->bounds.r = src->bounds.r; break;
    case HVAL_STR:
        if (range_enum_copy(&dst->bounds.e, &src->bounds.e) != 0)
            return -1;
        break;

    default:
        return -1;
    }
    dst->owner = NULL;

    return 0;
}

unsigned long hrange_max_idx(hrange_t* range)
{
    switch (range->type) {
    case HVAL_INT:  return range_int_max_idx(&range->bounds.i);
    case HVAL_REAL: return range_real_max_idx(&range->bounds.r);
    case HVAL_STR:  return range_enum_max_idx(&range->bounds.e);
    default:
        return 0;
    }
}

int hrange_pack(char** buf, int* buflen, const hrange_t* range)
{
    int i, count, total;
    const char* type_str;

    count = snprintf_serial(buf, buflen, " range:");
    if (count < 0) goto error;
    total = count;

    count = printstr_serial(buf, buflen, range->name);
    if (count < 0) goto error;
    total += count;

    switch (range->type) {
    case HVAL_INT:  type_str = "INT"; break;
    case HVAL_REAL: type_str = "REAL"; break;
    case HVAL_STR:  type_str = "ENUM"; break;
    default: goto invalid;
    }

    count = snprintf_serial(buf, buflen, " %s", type_str);
    if (count < 0) goto invalid;
    total += count;

    switch (range->type) {
    case HVAL_INT:
        count = snprintf_serial(buf, buflen, " %ld %ld %ld",
                                range->bounds.i.min,
                                range->bounds.i.max,
                                range->bounds.i.step);
        if (count < 0) goto invalid;
        total += count;
        break;

    case HVAL_REAL:
        count = snprintf_serial(buf, buflen, " %la %la %la",
                                range->bounds.r.min,
                                range->bounds.r.max,
                                range->bounds.r.step);
        if (count < 0) goto invalid;
        total += count;
        break;

    case HVAL_STR:
        count = snprintf_serial(buf, buflen, " %d", range->bounds.e.len);
        if (count < 0) goto invalid;
        total += count;

        for (i = 0; i < range->bounds.e.len; ++i) {
            count = printstr_serial(buf, buflen, range->bounds.e.set[i]);
            if (count < 0) goto invalid;
            total += count;
        }
        break;

    default:
        goto invalid;
    }
    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}

int hrange_unpack(hrange_t* range, char* buf)
{
    int count = -1, total;

    sscanf(buf, " range:%n", &count);
    if (count < 0)
        goto invalid;
    total = count;

    count = scanstr_serial((const char**)&range->name, buf + total);
    if (count < 0) goto invalid;
    total += count;

    char type_str[5];
    if (sscanf(buf + total, " %4s%n", type_str, &count) < 1)
        goto invalid;
    total += count;

    if      (strcmp(type_str, "INT") == 0) range->type = HVAL_INT;
    else if (strcmp(type_str, "REAL") == 0) range->type = HVAL_REAL;
    else if (strcmp(type_str, "ENUM") == 0) range->type = HVAL_STR;
    else goto invalid;

    switch (range->type) {
    case HVAL_INT:
        if (sscanf(buf + total, " %ld %ld %ld%n",
                   &range->bounds.i.min,
                   &range->bounds.i.max,
                   &range->bounds.i.step,
                   &count) < 3)
            goto invalid;
        total += count;
        break;

    case HVAL_REAL:
        if (sscanf(buf + total, " %la %la %la%n",
                   &range->bounds.r.min,
                   &range->bounds.r.max,
                   &range->bounds.r.step,
                   &count) < 3)
            goto invalid;
        total += count;
        break;

    case HVAL_STR:
        if (sscanf(buf + total, " %d%n",
                   &range->bounds.e.len, &count) < 1)
            goto invalid;
        total += count;

        if (range->bounds.e.cap < range->bounds.e.len) {
            char** newbuf = realloc(range->bounds.e.set,
                                    sizeof(char*) * range->bounds.e.len);
            if (!newbuf) goto error;
            range->bounds.e.set = newbuf;
            range->bounds.e.cap = range->bounds.e.len;
        }

        for (int i = 0; i < range->bounds.e.len; ++i) {
            count = scanstr_serial((const char**)&range->bounds.e.set[i],
                                   buf + total);
            if (count < 0) goto invalid;
            total += count;
        }
        break;

    default:
        goto invalid;
    }
    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}

/*
 * Internal helper function implementations.
 */
int range_enum_copy(range_enum_t* dst, const range_enum_t* src)
{
    dst->set = malloc(src->cap * sizeof(*dst->set));
    if (!dst->set)
        return -1;

    dst->len = src->len;
    dst->cap = src->cap;
    for (int i = 0; i < src->len; ++i) {
        dst->set[i] = stralloc(src->set[i]);
        if (!dst->set[i])
            return -1;
    }
    return 0;
}

unsigned long range_int_max_idx(range_int_t* bounds)
{
    return (bounds->max - bounds->min) / bounds->step;
}

unsigned long range_real_max_idx(range_real_t* bounds)
{
    if (bounds->step > 0.0)
        return (unsigned long)((bounds->max - bounds->min) / bounds->step);
    return 0;
}

unsigned long range_enum_max_idx(range_enum_t* bounds)
{
    return bounds->len - 1;
}

int copy_token(const char* buf, char **token, const char** err)
{
    const char* src;
    int cap = 0;

    while (1) {
        src = buf;
        // Skip leading whitespace.
        while (isspace(*src)) ++src;

        char* dst = *token;
        int   len = cap;
        char  quote = '\0';
        while (*src) {
            if (!quote) {
                if      (*src == '\'')  { quote = '\''; ++src; continue; }
                else if (*src ==  '"')  { quote =  '"'; ++src; continue; }
                else if (*src ==  ',')  { break; }
                else if (isspace(*src)) { break; }
            }
            else {
                if      (*src == '\\')  { ++src; }
                else if (*src == quote) { quote = '\0'; ++src; continue; }
            }

            if (len-- > 0)
                *(dst++) = *(src++);
            else
                ++src;
        }
        if (len-- > 0)
            *dst = '\0';

        if (quote != '\0') {
            *err = "Non-terminated quote detected";
            return -1;
        }

        if (len < -1) {
            // Token buffer size is -len;
            cap += -len;
            *token = malloc(cap * sizeof(**token));
            if (!*token) {
                *err = "Could not allocate a new configuration key/val pair";
                return -1;
            }
        }
        else if (len == -1) {
            // Empty token.
            *token = NULL;
            break;
        }
        else break; // Loop exit.
    }
    return src - buf;
}
