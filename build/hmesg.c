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

#include "hmesg.h"
#include "hsession.h"
#include "hval.h"
#include "hutil.h"
#include "hsockutil.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

const hmesg_t HMESG_INITIALIZER = {0};

void hmesg_scrub(hmesg_t *mesg)
{
    switch (mesg->type) {
    case HMESG_SESSION:
        if (mesg->status == HMESG_STATUS_REQ)
            hsession_fini(&mesg->data.session);
        break;

    case HMESG_JOIN:
        if (mesg->status == HMESG_STATUS_REQ ||
            mesg->status == HMESG_STATUS_OK)
        {
            hsignature_fini(&mesg->data.join);
        }
        break;

    case HMESG_FETCH:
        if (mesg->status == HMESG_STATUS_OK) {
            hpoint_fini(&mesg->data.fetch.cand);
            hpoint_fini(&mesg->data.fetch.best);
        }
        break;

    case HMESG_REPORT:
        if (mesg->status == HMESG_STATUS_REQ)
            hpoint_fini(&mesg->data.fetch.cand);
        break;

    default:
        break;
        /* All other cases have no heap memory to release. */
    }
}

void hmesg_fini(hmesg_t *mesg)
{
    hmesg_scrub(mesg);
    free(mesg->buf);
}

int hmesg_serialize(hmesg_t *mesg)
{
    const char *type_str, *status_str;
    char hdr[HMESG_HDRLEN + 1];
    char *buf;
    int buflen, count, total;

  top:
    buf = mesg->buf;
    buflen = mesg->buflen;

    /* Leave room for a header. */
    buf += HMESG_HDRLEN;
    buflen -= HMESG_HDRLEN;
    if (buflen < 0)
        buflen = 0;
    total = HMESG_HDRLEN;

    switch (mesg->type) {
    case HMESG_UNKNOWN: type_str = "UNK"; break;
    case HMESG_SESSION: type_str = "SES"; break;
    case HMESG_JOIN:    type_str = "JOI"; break;
    case HMESG_GETCFG:  type_str = "QRY"; break;
    case HMESG_SETCFG:  type_str = "INF"; break;
    case HMESG_FETCH:   type_str = "FET"; break;
    case HMESG_REPORT:  type_str = "REP"; break;
    default: goto invalid;
    }

    switch (mesg->status) {
    case HMESG_STATUS_REQ:  status_str = "REQ"; break;
    case HMESG_STATUS_OK:   status_str = "ACK"; break;
    case HMESG_STATUS_FAIL: status_str = "ERR"; break;
    case HMESG_STATUS_BUSY: status_str = "BSY"; break;
    default: goto invalid;
    }

    count = snprintf_serial(&buf, &buflen, ":%d:%s:%s:", mesg->dest,
                            type_str, status_str);
    if (count < 0) goto error;
    total += count;

    count = printstr_serial(&buf, &buflen, mesg->src_id);
    if (count < 0) goto error;
    total += count;

    if (mesg->status == HMESG_STATUS_BUSY) {
        /* Busy messages contain no data. */
    }
    else if (mesg->status == HMESG_STATUS_FAIL) {
        count = printstr_serial(&buf, &buflen, mesg->data.string);
        if (count < 0) goto error;
        total += count;
    }
    else {
        switch (mesg->type) {
        case HMESG_SESSION:
            if (mesg->status == HMESG_STATUS_REQ) {
                count = hsession_serialize(&buf, &buflen, &mesg->data.session);
                if (count < 0) goto error;
                total += count;
            }
            break;

        case HMESG_JOIN:
            count = hsignature_serialize(&buf, &buflen, &mesg->data.join);
            if (count < 0) goto error;
            total += count;
            break;

        case HMESG_GETCFG:
        case HMESG_SETCFG:
            count = printstr_serial(&buf, &buflen, mesg->data.string);
            if (count < 0) goto error;
            total += count;
            break;

        case HMESG_FETCH:
            if (mesg->status == HMESG_STATUS_REQ) {
                count = snprintf_serial(&buf, &buflen, "%d ",
                                        mesg->data.fetch.best.id);
                if (count < 0) goto error;
                total += count;
            }
            else if (mesg->status == HMESG_STATUS_OK) {
                count = hpoint_serialize(&buf, &buflen,
                                         &mesg->data.fetch.cand);
                if (count < 0) goto error;
                total += count;

                count = hpoint_serialize(&buf, &buflen,
                                         &mesg->data.fetch.best);
                if (count < 0) goto error;
                total += count;
            }
            break;

        case HMESG_REPORT:
            if (mesg->status == HMESG_STATUS_REQ) {
                count = hpoint_serialize(&buf, &buflen,
                                         &mesg->data.report.cand);
                if (count < 0) goto error;
                total += count;

                count = snprintf_serial(&buf, &buflen, "%la ",
                                        mesg->data.report.perf);
                if (count < 0) goto error;
                total += count;
            }
            break;

        default:
            goto invalid;
        }
    }

    if (total >= mesg->buflen) {
        buf = (char *) realloc(mesg->buf, total + 1);
        if (!buf)
            goto error;

        mesg->buf = buf;
        mesg->buflen = total + 1;
        goto top;
    }

    snprintf(hdr, sizeof(hdr), "XXXX%04d%02x", total, HMESG_VERSION);
    *(unsigned int *)(hdr) = htonl(HMESG_MAGIC);
    memcpy(mesg->buf, hdr, HMESG_HDRLEN);

    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}

int hmesg_deserialize(hmesg_t *mesg)
{
    char type_str[4], status_str[4];
    int count, total;
    unsigned int msgver;
    char *buf = mesg->buf;

    /* Verify HMESG_MAGIC and HMESG_VERSION */
    if (ntohl(*(unsigned int *)buf) != HMESG_MAGIC)
        goto invalid;

    if (sscanf(buf + sizeof(unsigned int), "%*4d%2x", &msgver) < 1)
        goto invalid;

    if (msgver != HMESG_VERSION)
        goto invalid;
    total = HMESG_HDRLEN;

    if (sscanf(buf + total, " :%d:%3s:%3s:%n", &mesg->dest,
               type_str, status_str, &count) < 3)
        goto invalid;
    total += count;

    count = scanstr_serial(&mesg->src_id, buf + total);
    if (count < 0) goto invalid;
    total += count;

    /* Before we overwrite this message's type, make sure memory allocated
     * from previous usage has been released.
     */
    if      (strcmp(type_str, "UNK") == 0) mesg->type = HMESG_UNKNOWN;
    else if (strcmp(type_str, "SES") == 0) mesg->type = HMESG_SESSION;
    else if (strcmp(type_str, "JOI") == 0) mesg->type = HMESG_JOIN;
    else if (strcmp(type_str, "QRY") == 0) mesg->type = HMESG_GETCFG;
    else if (strcmp(type_str, "INF") == 0) mesg->type = HMESG_SETCFG;
    else if (strcmp(type_str, "FET") == 0) mesg->type = HMESG_FETCH;
    else if (strcmp(type_str, "REP") == 0) mesg->type = HMESG_REPORT;
    else goto invalid;

    if      (strcmp(status_str, "REQ") == 0) mesg->status = HMESG_STATUS_REQ;
    else if (strcmp(status_str, "ACK") == 0) mesg->status = HMESG_STATUS_OK;
    else if (strcmp(status_str, "ERR") == 0) mesg->status = HMESG_STATUS_FAIL;
    else if (strcmp(status_str, "BSY") == 0) mesg->status = HMESG_STATUS_BUSY;
    else goto invalid;

    if (mesg->status == HMESG_STATUS_BUSY) {
        /* Busy messages contain no data. */
    }
    else if (mesg->status == HMESG_STATUS_FAIL) {
        count = scanstr_serial(&mesg->data.string, buf + total);
        if (count < 0) goto error;
        total += count;
    }
    else {
        switch (mesg->type) {
        case HMESG_SESSION:
            if (mesg->status == HMESG_STATUS_REQ) {
                hsession_init(&mesg->data.session);
                count = hsession_deserialize(&mesg->data.session, buf + total);
                if (count < 0) goto error;
                total += count;
            }
            break;

        case HMESG_JOIN:
            mesg->data.join = HSIGNATURE_INITIALIZER;
            count = hsignature_deserialize(&mesg->data.join, buf + total);
            if (count < 0) goto error;
            total += count;
            break;

        case HMESG_GETCFG:
        case HMESG_SETCFG:
            count = scanstr_serial(&mesg->data.string, buf + total);
            if (count < 0) goto error;
            total += count;
            break;

        case HMESG_FETCH:
            if (mesg->status == HMESG_STATUS_REQ) {
                if (sscanf(buf + total, " %d%n",
                           &mesg->data.fetch.best.id, &count) < 1)
                    goto invalid;
                total += count;
            }
            else if (mesg->status == HMESG_STATUS_OK) {
                mesg->data.fetch.cand = HPOINT_INITIALIZER;
                count = hpoint_deserialize(&mesg->data.fetch.cand,
                                           buf + total);
                if (count < 0) goto error;
                total += count;

                mesg->data.fetch.best = HPOINT_INITIALIZER;
                count = hpoint_deserialize(&mesg->data.fetch.best,
                                           buf + total);
                if (count < 0) goto error;
                total += count;
            }
            break;

        case HMESG_REPORT:
            if (mesg->status == HMESG_STATUS_REQ) {
                mesg->data.fetch.cand = HPOINT_INITIALIZER;
                count = hpoint_deserialize(&mesg->data.report.cand,
                                           buf + total);
                if (count < 0) goto error;
                total += count;

                if (sscanf(buf + total, " %la%n",
                           &mesg->data.report.perf, &count) < 1)
                    goto invalid;
                total += count;
            }
            break;

        default:
            goto invalid;
        }
    }
    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}
