/*
 * Copyright 2003-2011 Jeffrey K. Hollingsworth
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

#include "hmesgs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int hval_serialize(char *buf, size_t buflen, const hval_t *val)
{
    int total;
    size_t len;

    switch (val->type) {
    case HVAL_INT:
        total = snprintf(buf, buflen, "|%d %ld", HVAL_INT, val->value.i);
        break;

    case HVAL_REAL:
        total = snprintf(buf, buflen, "|%d %la", HVAL_REAL, val->value.r);
        break;

    case HVAL_STR:
        len = strlen(val->value.s);
        if (len >= MAX_VAL_STRLEN) {
            fprintf(stderr, "*** Error: Harmony string variable overflow.\n");
            return -1;
        }
        total = snprintf(buf, buflen, "|%d %zu %.*s", HVAL_STR,
                         len, (unsigned int)len, val->value.s);
        break;

    default:
        return -1;
    }

    if (total < 0 || ((size_t)total) >= buflen) {
        fprintf(stderr, "*** Error: Harmony variable overflow.\n");
        return -1;
    }

    return total;
}

int hval_deserialize(hval_t *val, const char *buf)
{
    unsigned int len, type;
    int count, total;

    if (sscanf(buf, " |%d%n", &type, &count) < 1)
        return -1;
    val->type = type;
    total = count;

    switch (type) {
    case HVAL_INT:
        if (sscanf(buf + total, " %ld%n", &val->value.i, &count) < 1)
            return -1;
        total += count;
        break;

    case HVAL_REAL:
        if (sscanf(buf + total, " %la%n", &val->value.r, &count) < 1)
            return -1;
        total += count;
        break;

    case HVAL_STR:
        if (sscanf(buf + total, " %d %n", &len, &count) < 1)
            return -1;
        total += count;

        if (len >= MAX_VAL_STRLEN) {
            fprintf(stderr, "*** Warning: Harmony string variable overflow."
                    "  Truncating.\n");

            strncpy(val->value.s, buf + total, MAX_VAL_STRLEN - 1);
            val->value.s[MAX_VAL_STRLEN - 1] = '\0';
        }
        else {
            strncpy(val->value.s, buf + total, len);
            val->value.s[len] = '\0';
        }

        total += len;
        break;

    default:
        return -1;
    }

    return total;
}

int hmesg_serialize(char *buf, size_t buflen, const hmesg_t *mesg)
{
    unsigned int i;
    const char *type_str;
    int count, total;

    switch (mesg->type) {
    case HMESG_CONFIRM:      type_str = "ACK"; break;
    case HMESG_FAIL:         type_str = "NAK"; break;
    case HMESG_CLIENT_REG:   type_str = "REG"; break;
    case HMESG_CLIENT_UNREG: type_str = "UNR"; break;
    case HMESG_APP_DESCR:    type_str = "APP"; break;
    case HMESG_VAR_DESCR:    type_str = "VAR"; break;
    case HMESG_QUERY:        type_str = "QRY"; break;
    case HMESG_FETCH:        type_str = "FET"; break;
    case HMESG_REPORT:       type_str = "REP"; break;
    default:
        fprintf(stderr, "*** Error: Unknown Harmony message type.\n");
        return -1;
    }

    count = snprintf(buf, buflen, "%u:%s", HMESG_VERSION, type_str);
    total = count;
    if (count < 0 || ((size_t)total) >= buflen) {
        fprintf(stderr, "*** Error: Harmony message overflow.\n");
        return -1;
    }

    count = snprintf(buf + total, buflen - total, ":%hu", mesg->id);
    if (count < 0 || ((size_t)total) >= buflen) {
        fprintf(stderr, "*** Error: Harmony message overflow.\n");
        return -1;
    }
    total += count;

    count = snprintf(buf + total, buflen - total, ":%d", mesg->timestamp);
    if (count < 0 || ((size_t)total) >= buflen) {
        fprintf(stderr, "*** Error: Harmony message overflow.\n");
        return -1;
    }
    total += count;

    switch (mesg->type) {
    case HMESG_CLIENT_UNREG:
        break;

    case HMESG_CONFIRM:
    case HMESG_FAIL:
    case HMESG_CLIENT_REG:
    case HMESG_APP_DESCR:
    case HMESG_QUERY:
        if (mesg->count >= MAX_MSG_STRLEN) {
            fprintf(stderr, "*** Error: Harmony string variable overflow.\n");
            return -1;
        }
        count = snprintf(buf + total, buflen - total, ":%hu", mesg->count);
        total += count;
        if (count < 0 || ((size_t)total) >= buflen) {
            fprintf(stderr, "*** Error: Harmony message overflow.\n");
            return -1;
        }

        if (mesg->count > 0) {
            count = snprintf(buf + total, buflen - total, " %s",
                             (char *)mesg->data);
            total += count;
            if (count < 0 || ((size_t)total) >= buflen) {
                fprintf(stderr, "*** Error: Harmony message overflow.\n");
                return -1;
            }
        }
        break;

    case HMESG_VAR_DESCR:
    case HMESG_FETCH:
    case HMESG_REPORT:
        count = snprintf(buf + total, buflen - total, ":%hu", mesg->count);
        if (count < 0 || ((size_t)total) >= buflen) {
            fprintf(stderr, "*** Error: Harmony message overflow.\n");
            return -1;
        }
        total += count;

        for (i = 0; i < mesg->count; ++i) {
            count = hval_serialize(buf + total, buflen - total,
                                    ((hval_t *)mesg->data) + i);
            if (count < 0 || ((size_t)total) >= buflen) {
                fprintf(stderr, "*** Error: Harmony message overflow.\n");
                return -1;
            }
            total += count;
        }
        break;

    default:
        fprintf(stderr, "*** Error: Unknown Harmony message type.\n");
        return -1;
    }
    return total;
}

int hmesg_deserialize(hmesg_t *mesg, const char *buf)
{
    unsigned int i, version;
    char type_str[4];
    int count, total;

    if (sscanf(buf, " %u:%3s%n", &version, type_str, &count) < 2 ||
        version != HMESG_VERSION)
    {
        return -1;
    }
    total = count;

    if      (strcmp(type_str, "ACK") == 0) mesg->type = HMESG_CONFIRM;
    else if (strcmp(type_str, "NAK") == 0) mesg->type = HMESG_FAIL;
    else if (strcmp(type_str, "REG") == 0) mesg->type = HMESG_CLIENT_REG;
    else if (strcmp(type_str, "UNR") == 0) mesg->type = HMESG_CLIENT_UNREG;
    else if (strcmp(type_str, "APP") == 0) mesg->type = HMESG_APP_DESCR;
    else if (strcmp(type_str, "VAR") == 0) mesg->type = HMESG_VAR_DESCR;
    else if (strcmp(type_str, "QRY") == 0) mesg->type = HMESG_QUERY;
    else if (strcmp(type_str, "FET") == 0) mesg->type = HMESG_FETCH;
    else if (strcmp(type_str, "REP") == 0) mesg->type = HMESG_REPORT;
    else {
        fprintf(stderr, "*** Error: Unknown Harmony message type.\n");
        return -1;
    }

    if (sscanf(buf + total, " :%hu%n", &mesg->id, &count) < 1)
        return -1;
    total += count;

    if (sscanf(buf + total, " :%d%n", &mesg->timestamp, &count) < 1)
        return -1;
    total += count;

    switch (mesg->type) {
    case HMESG_CLIENT_UNREG:
        break;

    case HMESG_CONFIRM:
    case HMESG_FAIL:
    case HMESG_CLIENT_REG:
    case HMESG_APP_DESCR:
    case HMESG_QUERY:
        if (sscanf(buf + total, " :%hu %n", &mesg->count, &count) < 1)
            return -1;
        total += count;

        if (mesg->count >= MAX_MSG_STRLEN) {
            fprintf(stderr, "*** Warning: Harmony message overflow."
                    "  Truncating.\n");
            mesg->count = MAX_MSG_STRLEN - 1;
        }

        strncpy(mesg->data, buf + total, mesg->count);
        mesg->data[mesg->count] = '\0';

        total += mesg->count;
        break;

    case HMESG_VAR_DESCR:
    case HMESG_FETCH:
    case HMESG_REPORT:
        if (sscanf(buf + total, " :%hu %n", &mesg->count, &count) < 1)
            return -1;
        total += count;
        
        for (i = 0; i < mesg->count; ++i) {
            count = hval_deserialize(((hval_t *)mesg->data) + i, buf + total);
            if (count < 0)
                return -1;
            total += count;
        }
        break;

    default:
        return -1;
    }
    return total;
}
