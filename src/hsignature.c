/*
 * Copyright 2003-2013 Jeffrey K. Hollingsworth
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

#include "hsignature.h"
#include "hutil.h"
#include "defaults.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

const hrange_t HRANGE_INITIALIZER = {0};
const hsignature_t HSIGNATURE_INITIALIZER = {0};

/*
 * Harmony range related function definitions.
 */
hrange_t *hrange_find(hsignature_t *sig, const char *name)
{
    int i;
    for (i = 0; i < sig->range_len; ++i)
        if (strcmp(name, sig->range[i].name) == 0)
            return &sig->range[i];

    return NULL;
}

hrange_t *hrange_add(hsignature_t *sig, const char *name)
{
    hrange_t *range;

    if (hrange_find(sig, name) != NULL) {
        errno = EINVAL;
        return NULL;
    }

    /* Grow range array, if needed. */
    if (sig->range_len == sig->range_cap) {
        if (array_grow(&sig->range, &sig->range_cap, sizeof(hrange_t)) < 0)
            return NULL;
    }
    range = &sig->range[sig->range_len];

    range->name = stralloc(name);
    if (!range->name)
        return NULL;
    ++sig->range_len;

    return range;
}

void hrange_fini(hrange_t *range)
{
    int i;

    if (range->name)
        free(range->name);

    if (range->type == HVAL_STR) {
        for (i = 0; i < range->bounds.s.set_len; ++i)
            free(range->bounds.s.set[i]);
    }
    *range = HRANGE_INITIALIZER;
}

int hrange_copy(hrange_t *dst, const hrange_t *src)
{
    int i;

    hrange_fini(dst);

    dst->name = stralloc(src->name);
    if (!dst->name)
        return -1;

    dst->type = src->type;
    switch (dst->type) {
    case HVAL_INT:  memcpy(&dst->bounds.i, &src->bounds.i,
                           sizeof(int_bounds_t)); break;
    case HVAL_REAL: memcpy(&dst->bounds.r, &src->bounds.r,
                           sizeof(real_bounds_t)); break;
    case HVAL_STR:
        dst->bounds.s.set = (char **) malloc(src->bounds.s.set_len *
                                             sizeof(char *));
        if (!dst->bounds.s.set)
            return -1;

        dst->bounds.s.set_len = src->bounds.s.set_len;
        dst->bounds.s.set_cap = src->bounds.s.set_len;
        for (i = 0; i < dst->bounds.s.set_len; ++i) {
            dst->bounds.s.set[i] = stralloc(src->bounds.s.set[i]);
            if (!dst->bounds.s.set[i])
                return -1;
        }
        break;

    default:
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int hrange_serialize(char **buf, int *buflen, const hrange_t *range)
{
    int i, count, total;
    const char *type_str;

    count = snprintf_serial(buf, buflen, "range: ");
    if (count < 0) goto error;
    total = count;

    count = printstr_serial(buf, buflen, range->name);
    if (count < 0) goto error;
    total += count;

    switch (range->type) {
    case HVAL_INT:  type_str = "INT"; break;
    case HVAL_REAL: type_str = "REL"; break;
    case HVAL_STR:  type_str = "STR"; break;
    default: goto invalid;
    }

    count = snprintf_serial(buf, buflen, "%s ", type_str);
    if (count < 0) goto invalid;
    total += count;

    switch (range->type) {
    case HVAL_INT:
        count = snprintf_serial(buf, buflen, "%ld %ld %ld ",
                                range->bounds.i.min,
                                range->bounds.i.max,
                                range->bounds.i.step);
        if (count < 0) goto invalid;
        total += count;
        break;

    case HVAL_REAL:
        count = snprintf_serial(buf, buflen, "%la %la %la ",
                                range->bounds.r.min,
                                range->bounds.r.max,
                                range->bounds.r.step);
        if (count < 0) goto invalid;
        total += count;
        break;

    case HVAL_STR:
        count = snprintf_serial(buf, buflen, "%d ", range->bounds.s.set_len);
        if (count < 0) goto invalid;
        total += count;

        for (i = 0; i < range->bounds.s.set_len; ++i) {
            count = printstr_serial(buf, buflen, range->bounds.s.set[i]);
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

int hrange_deserialize(hrange_t *range, char *buf)
{
    int i, count, total;
    char **newbuf;
    char *strptr;
    char type_str[4];

    for (i = 0; isspace(buf[i]); ++i);
    if (strncmp("range:", buf + i, 6) != 0)
        goto invalid;
    total = i + 6;

    count = scanstr_serial((const char **)&strptr, buf + total);
    if (count < 0) goto invalid;
    total += count;

    range->name = stralloc(strptr);
    if (!range->name) goto error;

    if (sscanf(buf + total, " %3s%n", type_str, &count) < 1)
        goto invalid;
    total += count;

    if      (strcmp(type_str, "INT") == 0) range->type = HVAL_INT;
    else if (strcmp(type_str, "REL") == 0) range->type = HVAL_REAL;
    else if (strcmp(type_str, "STR") == 0) range->type = HVAL_STR;
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
                   &range->bounds.s.set_len, &count) < 1)
            goto invalid;
        total += count;

        if (range->bounds.s.set_cap < range->bounds.s.set_len) {
            newbuf = (char **) realloc(range->bounds.s.set,
                                       sizeof(char *) *
                                       range->bounds.s.set_len);
            if (!newbuf) goto error;
            range->bounds.s.set = newbuf;
            range->bounds.s.set_cap = range->bounds.s.set_len;
        }

        for (i = 0; i < range->bounds.s.set_len; ++i) {
            count = scanstr_serial((const char **)&strptr, buf + total);
            if (count < 0) goto invalid;
            total += count;

            range->bounds.s.set[i] = stralloc(strptr);
            if (!range->bounds.s.set[i]) goto error;
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
 * Signature-related function definitions.
 */
int hsignature_copy(hsignature_t *dst, const hsignature_t *src)
{
    int i;

    hsignature_fini(dst);

    dst->range = (hrange_t *) malloc(sizeof(hrange_t) * src->range_len);
    if (!dst->range)
        return -1;

    dst->range_len = src->range_len;
    dst->range_cap = src->range_len;

    dst->name = stralloc(src->name);
    if (!dst->name)
        return -1;

    for (i = 0; i < dst->range_len; ++i) {
        dst->range[i] = HRANGE_INITIALIZER;
        if (hrange_copy(&dst->range[i], &src->range[i]) != 0)
            return -1;
    }
    return 0;
}

void hsignature_fini(hsignature_t *sig)
{
    int i;

    for (i = 0; i < sig->range_len; ++i) {
        free(sig->range[i].name);
        if (sig->range[i].type == HVAL_STR) {
            sig->range[i].name = NULL;
            hrange_fini(&sig->range[i]);
        }
    }

    free(sig->range);
    free(sig->name);
    *sig = HSIGNATURE_INITIALIZER;
}

int hsignature_equal(const hsignature_t *sig_a, const hsignature_t *sig_b)
{
    int i, j;

    if (strcmp(sig_a->name, sig_b->name) != 0)
        return 0;

    if (sig_a->range_len != sig_b->range_len)
        return 0;

    for (i = 0; i < sig_a->range_len; ++i) {
        hrange_t *range_a = &sig_a->range[i];
        hrange_t *range_b = &sig_b->range[i];

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
            if (range_a->bounds.s.set_len != range_b->bounds.s.set_len)
                return 0;
            for (j = 0; j < range_a->bounds.s.set_len; ++j) {
                if (strcmp(range_a->bounds.s.set[j],
                           range_b->bounds.s.set[j]) != 0)
                    return 0;
            }
            break;
        default:
            return 0;
        }
    }
    return 1;
}

int hsignature_match(const hsignature_t *sig_a, const hsignature_t *sig_b)
{
    int i;

    if (strcmp(sig_a->name, sig_b->name) != 0)
        return 0;

    if (sig_a->range_len != sig_b->range_len)
        return 0;

    for (i = 0; i < sig_a->range_len; ++i) {
        if (strcmp(sig_a->range[i].name, sig_b->range[i].name) != 0)
            return 0;
        if (sig_a->range[i].type != sig_b->range[i].type)
            return 0;
    }
    return 1;
}

int hsignature_name(hsignature_t *sig, const char *name)
{
    if (sig->name)
        free(sig->name);

    sig->name = stralloc(name);
    if (!sig->name)
        return -1;

    return 0;
}

int hsignature_int(hsignature_t *sig, const char *name,
                   long min, long max, long step)
{
    hrange_t *range;

    if (max < min) {
        errno = EINVAL;
        return -1;
    }

    range = hrange_add(sig, name);
    if (!range)
        return -1;

    range->type = HVAL_INT;
    range->bounds.i.min = min;
    range->bounds.i.max = max;
    range->bounds.i.step = step;

    return 0;
}

int hsignature_real(hsignature_t *sig, const char *name,
                    double min, double max, double step)
{
    hrange_t *range;

    if (max < min) {
        errno = EINVAL;
        return -1;
    }

    range = hrange_add(sig, name);
    if (!range)
        return -1;

    range->type = HVAL_REAL;
    range->bounds.r.min = min;
    range->bounds.r.max = max;
    range->bounds.r.step = step;

    return 0;
}

int hsignature_enum(hsignature_t *sig, const char *name, const char *value)
{
    int i;
    hrange_t *range;

    range = hrange_find(sig, name);
    if (range && range->type != HVAL_STR) {
        errno = EINVAL;
        return -1;
    }

    if (!range) {
        range = hrange_add(sig, name);
        if (!range)
            return -1;

        range->type = HVAL_STR;
    }

    for (i = 0; i < range->bounds.s.set_len; ++i) {
        if (strcmp(value, range->bounds.s.set[i]) == 0) {
            errno = EINVAL;
            return -1;
        }
    }

    if (range->bounds.s.set_len == range->bounds.s.set_cap) {
        if (array_grow(&range->bounds.s.set,
                       &range->bounds.s.set_cap, sizeof(char *)) < 0)
            return -1;
    }

    range->bounds.s.set[i] = stralloc(value);
    if (!range->bounds.s.set[i])
        return -1;

    ++range->bounds.s.set_len;
    return 0;
}

int hsignature_serialize(char **buf, int *buflen, const hsignature_t *sig)
{
    int i, count, total;

    count = snprintf_serial(buf, buflen, "sig: ");
    if (count < 0) goto invalid;
    total = count;

    count = printstr_serial(buf, buflen, sig->name);
    if (count < 0) goto error;
    total += count;

    count = snprintf_serial(buf, buflen, "%d ", sig->range_len);
    if (count < 0) goto invalid;
    total += count;

    for (i = 0; i < sig->range_len; ++i) {
        count = hrange_serialize(buf, buflen, &sig->range[i]);
        if (count < 0) goto error;
        total += count;
    }
    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}

int hsignature_deserialize(hsignature_t *sig, char *buf)
{
    int i, count, total;
    char *strptr;
    hrange_t *newbuf;

    for (i = 0; isspace(buf[i]); ++i);
    if (strncmp("sig:", buf + i, 4) != 0)
        goto invalid;
    total = i + 4;

    count = scanstr_serial((const char **)&strptr, buf + total);
    if (count < 0) goto invalid;
    total += count;

    sig->name = stralloc(strptr);
    if (!sig->name) goto error;

    if (sscanf(buf + total, " %d%n", &sig->range_len, &count) < 1)
        goto invalid;
    total += count;

    if (sig->range_cap < sig->range_len) {
        newbuf = (hrange_t *) realloc(sig->range,
                                      sizeof(hrange_t) * sig->range_len);
        if (!newbuf) goto error;
        sig->range = newbuf;
        sig->range_cap = sig->range_len;
    }

    for (i = 0; i < sig->range_len; ++i) {
        count = hrange_deserialize(&sig->range[i], buf + total);
        if (count < 0) goto invalid;
        total += count;
    }

    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}
