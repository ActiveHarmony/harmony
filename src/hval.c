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

#include "hval.h"
#include "hutil.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

const hval_t HVAL_INITIALIZER = {0};

int hval_serialize(char **buf, int *buflen, const hval_t *val)
{
    int count, total;
    const char *type_str;

    switch (val->type) {
    case HVAL_INT:  type_str = "INT"; break;
    case HVAL_REAL: type_str = "REL"; break;
    case HVAL_STR:  type_str = "STR"; break;
    default: goto invalid;
    }

    count = snprintf_serial(buf, buflen, "hval:%s ", type_str);
    if (count < 0) goto error;
    total = count;

    switch (val->type) {
    case HVAL_INT:
        count = snprintf_serial(buf, buflen, "%ld ", val->value.i);
        if (count < 0) goto error;
        total += count;
        break;

    case HVAL_REAL:
        count = snprintf_serial(buf, buflen, "%la ", val->value.r);
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

int hval_deserialize(hval_t *val, char *buf)
{
    int count, total;
    char type_str[4];

    if (sscanf(buf, " hval:%3s%n", type_str, &count) < 1)
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
