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

#include "hmesg.h"
#include "hval.h"
#include "hutil.h"
#include "hsockutil.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

const hmesg_t hmesg_zero = HMESG_INITIALIZER;

void hmesg_scrub(hmesg_t* mesg)
{
    switch (mesg->type) {
    case HMESG_SESSION:
        if (mesg->status == HMESG_STATUS_REQ) {
            hsig_scrub(&mesg->data.session.sig);
            hcfg_scrub(&mesg->data.session.cfg);
        }
        break;

    case HMESG_JOIN:
        if (mesg->status == HMESG_STATUS_REQ ||
            mesg->status == HMESG_STATUS_OK)
        {
            hsig_scrub(&mesg->data.join);
        }
        break;

    case HMESG_BEST:
    case HMESG_FETCH:
        if (mesg->status == HMESG_STATUS_OK ||
            mesg->status == HMESG_STATUS_BUSY)
        {
            hpoint_scrub(&mesg->data.point);
        }
        break;

    case HMESG_REPORT:
        if (mesg->status == HMESG_STATUS_REQ)
            hperf_fini(mesg->data.report.perf);
        break;

    default:
        break;
        /* All other cases have no heap memory to release. */
    }
}

void hmesg_fini(hmesg_t* mesg)
{
    hmesg_scrub(mesg);
    free(mesg->recv_buf);
    free(mesg->send_buf);
}

int hmesg_owner(hmesg_t* mesg, const void* ptr)
{
    return mesg && (ptr >= (void*)(mesg->recv_buf) &&
                    ptr <  (void*)(mesg->recv_buf + mesg->recv_len));
}

int hmesg_serialize(hmesg_t* mesg)
{
    const char* type_str;
    const char* status_str;
    char* buf;
    int buflen;
    int count, total;

    if (mesg->origin < -1 || mesg->origin >= 255) {
        fprintf(stderr, "Error: hmesg_serialize():"
                "Origin (%d) is out of range [-1, 254]\n", mesg->origin);
        return -1;
    }

  top:
    buf = mesg->send_buf;
    buflen = mesg->send_len;

    /* Leave room for a header. */
    buf += HMESG_HDR_SIZE;
    buflen -= HMESG_HDR_SIZE;
    if (buflen < 0)
        buflen = 0;
    total = HMESG_HDR_SIZE;

    switch (mesg->type) {
    case HMESG_UNKNOWN: type_str = "UNK"; break;
    case HMESG_SESSION: type_str = "SES"; break;
    case HMESG_JOIN:    type_str = "JOI"; break;
    case HMESG_GETCFG:  type_str = "QRY"; break;
    case HMESG_SETCFG:  type_str = "INF"; break;
    case HMESG_BEST:    type_str = "BST"; break;
    case HMESG_FETCH:   type_str = "FET"; break;
    case HMESG_REPORT:  type_str = "REP"; break;
    case HMESG_RESTART: type_str = "RES"; break;
    default: goto invalid;
    }

    switch (mesg->status) {
    case HMESG_STATUS_REQ:  status_str = "REQ"; break;
    case HMESG_STATUS_OK:   status_str = "ACK"; break;
    case HMESG_STATUS_FAIL: status_str = "ERR"; break;
    case HMESG_STATUS_BUSY: status_str = "BSY"; break;
    default: goto invalid;
    }

    count = snprintf_serial(&buf, &buflen, ":%s:%s:", type_str, status_str);
    if (count < 0) goto error;
    total += count;

    count = printstr_serial(&buf, &buflen, mesg->src_id);
    if (count < 0) goto error;
    total += count;

    if (mesg->status == HMESG_STATUS_FAIL) {
        count = printstr_serial(&buf, &buflen, mesg->data.string);
        if (count < 0) goto error;
        total += count;
    }
    else {
        switch (mesg->type) {
        case HMESG_SESSION:
            if (mesg->status == HMESG_STATUS_REQ) {
                count = hsig_serialize(&buf, &buflen, &mesg->data.session.sig);
                if (count < 0) goto error;
                total += count;

                count = hcfg_serialize(&buf, &buflen, &mesg->data.session.cfg);
                if (count < 0) goto error;
                total += count;
            }
            break;

        case HMESG_JOIN:
            count = hsig_serialize(&buf, &buflen, &mesg->data.join);
            if (count < 0) goto error;
            total += count;
            break;

        case HMESG_GETCFG:
        case HMESG_SETCFG:
            count = printstr_serial(&buf, &buflen, mesg->data.string);
            if (count < 0) goto error;
            total += count;
            break;

        case HMESG_BEST:
        case HMESG_FETCH:
            if (mesg->status == HMESG_STATUS_OK ||
                mesg->status == HMESG_STATUS_BUSY)
            {
                count = hpoint_serialize(&buf, &buflen, &mesg->data.point);
                if (count < 0) goto error;
                total += count;
            }
            break;

        case HMESG_REPORT:
            if (mesg->status == HMESG_STATUS_REQ) {
                count = snprintf_serial(&buf, &buflen, "%d ",
                                        mesg->data.report.cand_id);
                if (count < 0) goto error;
                total += count;

                count = hperf_serialize(&buf, &buflen, mesg->data.report.perf);
                if (count < 0) goto error;
                total += count;
            }
            break;

        case HMESG_RESTART:
            break;

        default:
            goto invalid;
        }
    }

    if (total >= mesg->send_len) {
        buf = realloc(mesg->send_buf, total + 1);
        if (!buf)
            goto error;

        mesg->send_buf = buf;
        mesg->send_len = total + 1;
        goto top;
    }

    unsigned int magic = htonl(HMESG_MAGIC);
    memcpy(mesg->send_buf, &magic, HMESG_MAGIC_SIZE);

    char header_buf[HMESG_HDR_SIZE - HMESG_LENGTH_OFFSET + 1];
    snprintf(header_buf, sizeof(header_buf), "%04d%02x%02x",
             total, HMESG_VERSION, mesg->origin);
    memcpy(mesg->send_buf + HMESG_LENGTH_OFFSET, header_buf,
           HMESG_HDR_SIZE - HMESG_LENGTH_OFFSET);

    return total;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}

int hmesg_deserialize(hmesg_t* mesg)
{
    int count, total;
    char* buf = mesg->recv_buf;

    /* Verify HMESG_MAGIC and HMESG_VERSION */
    unsigned int magic;
    memcpy(&magic, buf, sizeof(magic));
    if (ntohl(magic) != HMESG_MAGIC)
        goto invalid;

    unsigned int msgver;
    if (sscanf(buf + HMESG_VERSION_OFFSET, "%2x%2x",
               &msgver, &mesg->origin) < 2)
        goto invalid;
    if (mesg->origin == 255)
        mesg->origin = -1;

    if (msgver != HMESG_VERSION)
        goto invalid;
    total = HMESG_HDR_SIZE;

    char type_str[4];
    char status_str[4];
    if (sscanf(buf + total, " :%3s:%3s:%n", type_str, status_str, &count) < 2)
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
    else if (strcmp(type_str, "BST") == 0) mesg->type = HMESG_BEST;
    else if (strcmp(type_str, "FET") == 0) mesg->type = HMESG_FETCH;
    else if (strcmp(type_str, "REP") == 0) mesg->type = HMESG_REPORT;
    else if (strcmp(type_str, "RES") == 0) mesg->type = HMESG_RESTART;
    else goto invalid;

    if      (strcmp(status_str, "REQ") == 0) mesg->status = HMESG_STATUS_REQ;
    else if (strcmp(status_str, "ACK") == 0) mesg->status = HMESG_STATUS_OK;
    else if (strcmp(status_str, "ERR") == 0) mesg->status = HMESG_STATUS_FAIL;
    else if (strcmp(status_str, "BSY") == 0) mesg->status = HMESG_STATUS_BUSY;
    else goto invalid;

    if (mesg->status == HMESG_STATUS_FAIL) {
        count = scanstr_serial(&mesg->data.string, buf + total);
        if (count < 0) goto error;
        total += count;
    }
    else {
        switch (mesg->type) {
        case HMESG_SESSION:
            if (mesg->status == HMESG_STATUS_REQ) {
                mesg->data.session.sig.owner = mesg;
                count = hsig_deserialize(&mesg->data.session.sig, buf + total);
                if (count < 0) goto error;
                total += count;

                mesg->data.session.cfg.owner = mesg;
                count = hcfg_deserialize(&mesg->data.session.cfg, buf + total);
                if (count < 0) goto error;
                total += count;
            }
            break;

        case HMESG_JOIN:
            mesg->data.join = hsig_zero;
            mesg->data.join.owner = mesg;
            count = hsig_deserialize(&mesg->data.join, buf + total);
            if (count < 0) goto error;
            total += count;
            break;

        case HMESG_GETCFG:
        case HMESG_SETCFG:
            count = scanstr_serial(&mesg->data.string, buf + total);
            if (count < 0) goto error;
            total += count;
            break;

        case HMESG_BEST:
        case HMESG_FETCH:
            if (mesg->status == HMESG_STATUS_OK ||
                mesg->status == HMESG_STATUS_BUSY)
            {
                mesg->data.point = hpoint_zero;
                mesg->data.point.owner = mesg;
                count = hpoint_deserialize(&mesg->data.point, buf + total);
                if (count < 0) goto error;
                total += count;
            }
            break;

        case HMESG_REPORT:
            if (mesg->status == HMESG_STATUS_REQ) {
                if (sscanf(buf + total, " %d%n",
                           &mesg->data.report.cand_id, &count) < 1)
                    goto invalid;
                total += count;

                count = hperf_deserialize(&mesg->data.report.perf,
                                          buf + total);
                if (count < 0) goto error;
                total += count;
            }
            break;

        case HMESG_RESTART:
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
