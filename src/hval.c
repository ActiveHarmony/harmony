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

#include "hval.h"
#include "hutil.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

const hval_t hval_zero = HVAL_INITIALIZER;

// Internal helper function prototypes.
static int parse_int(hval_t* val, const char* buf);
static int parse_real(hval_t* val, const char* buf);
static int parse_str(hval_t* val, const char* buf);

//
// Data transmission function implementation.
//
int hval_pack(char** buf, int* buflen, const hval_t* val)
{
    int count, total;
    const char* type_str;

    switch (val->type) {
    case HVAL_INT:  type_str = "INT"; break;
    case HVAL_REAL: type_str = "REL"; break;
    case HVAL_STR:  type_str = "STR"; break;
    default: goto invalid;
    }

    count = snprintf_serial(buf, buflen, " val:%s", type_str);
    if (count < 0) goto error;
    total = count;

    switch (val->type) {
    case HVAL_INT:
        count = snprintf_serial(buf, buflen, " %ld", val->value.i);
        if (count < 0) goto error;
        total += count;
        break;

    case HVAL_REAL:
        count = snprintf_serial(buf, buflen, " %la", val->value.r);
        if (count < 0) goto error;
        total += count;
        break;

    case HVAL_STR:
        count = printstr_serial(buf, buflen, val->value.s);
        if (count < 0) goto error;
        total += count;
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

int hval_unpack(hval_t* val, char* buf)
{
    int count, total;
    char type_str[4];

    if (sscanf(buf, " val:%3s%n", type_str, &count) < 1)
        goto invalid;
    total = count;

    if      (strcmp(type_str, "INT") == 0) val->type = HVAL_INT;
    else if (strcmp(type_str, "REL") == 0) val->type = HVAL_REAL;
    else if (strcmp(type_str, "STR") == 0) val->type = HVAL_STR;
    else goto invalid;

    switch (val->type) {
    case HVAL_INT:
        if (sscanf(buf + total, " %ld%n", &val->value.i, &count) < 1)
            goto invalid;
        total += count;
        break;

    case HVAL_REAL:
        if (sscanf(buf + total, " %la%n", &val->value.r, &count) < 1)
            goto invalid;
        total += count;
        break;

    case HVAL_STR:
        count = scanstr_serial(&val->value.s, buf + total);
        if (count < 0) goto invalid;
        total += count;
        break;

    default:
        goto invalid;
    }
    return total;

  invalid:
    errno = EINVAL;
    return -1;
}

int hval_parse(hval_t* val, const char* buf)
{
    int span;

    switch (val->type) {
    case HVAL_INT:  span = parse_int(val, buf);  break;
    case HVAL_REAL: span = parse_real(val, buf); break;
    case HVAL_STR:  span = parse_str(val, buf);  break;
    default:        return -1;
    }

    return span;
}

//
// Internal helper function implementations.
//
int parse_int(hval_t* val, const char* buf)
{
    int span = -1;
    sscanf(buf, " %ld%n", &val->value.i, &span);
    return span;
}

int parse_real(hval_t* val, const char* buf)
{
    int span = -1;
    sscanf(buf, " %lf%n", &val->value.r, &span);
    return span;
}

int parse_str(hval_t* val, const char* buf)
{
    char* token;
    int span = unquote_string(buf, &token, NULL);
    val->value.s = token;
    return span;
}
