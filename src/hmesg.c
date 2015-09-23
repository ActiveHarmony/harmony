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

/* Internal helper function prototypes. */
static int pack_state(char** buf, int* buflen, const hmesg_t* mesg);
static int unpack_state(hmesg_t* mesg, char* buf);
static int pack_data(char** buf, int* buflen, const hmesg_t* mesg);
static int unpack_data(hmesg_t* mesg, char* buf);

void hmesg_scrub(hmesg_t* mesg)
{
    hsig_scrub(&mesg->state.sig);
    hpoint_scrub(&mesg->state.best);
    hcfg_scrub(&mesg->data.cfg);
    hpoint_scrub(&mesg->data.point);
}

void hmesg_fini(hmesg_t* mesg)
{
    hsig_fini(&mesg->state.sig);
    hpoint_fini(&mesg->state.best);
    hcfg_fini(&mesg->data.cfg);
    hpoint_fini(&mesg->data.point);
    hperf_fini(&mesg->data.perf);
    free(mesg->recv_buf);
    free(mesg->send_buf);
}

int hmesg_owner(hmesg_t* mesg, const void* ptr)
{
    return mesg && (ptr >= (void*)(mesg->recv_buf) &&
                    ptr <  (void*)(mesg->recv_buf + mesg->recv_len));
}

int hmesg_pack(hmesg_t* mesg)
{
    const char* type_str;
    const char* status_str;
    char* buf;
    int buflen;
    int count, total;

    if (mesg->origin < -1 || mesg->origin >= 255) {
        fprintf(stderr, "Error: hmesg_pack():"
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

    if (mesg->status == HMESG_STATUS_FAIL) {
        count = printstr_serial(&buf, &buflen, mesg->data.string);
        if (count < 0) goto error;
        total += count;
    }
    else {
        count = pack_state(&buf, &buflen, mesg);
        if (count < 0) goto error;
        total += count;

        count = pack_data(&buf, &buflen, mesg);
        if (count < 0) goto error;
        total += count;
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

int hmesg_unpack(hmesg_t* mesg)
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
        count = unpack_state(mesg, buf + total);
        if (count < 0) goto error;
        total += count;

        count = unpack_data(mesg, buf + total);
        if (count < 0) goto error;
        total += count;
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
int pack_state(char** buf, int* buflen, const hmesg_t* mesg)
{
    int count, total = 0;

    if (mesg->type == HMESG_SESSION ||
        mesg->type == HMESG_JOIN)
    {
        // Session state doesn't exist just yet.
        return 0;
    }

    switch (mesg->status) {
    case HMESG_STATUS_REQ:
        count = snprintf_serial(buf, buflen, " state:%u %u",
                                mesg->state.sig.id, mesg->state.best.id);
        if (count < 0) return -1;
        total += count;

        count = printstr_serial(buf, buflen, mesg->state.client);
        if (count < 0) return -1;
        total += count;
        break;

    case HMESG_STATUS_OK:
    case HMESG_STATUS_BUSY:
        count = snprintf_serial(buf, buflen, " state:");
        if (count < 0) return -1;
        total += count;

        count = hsig_pack(buf, buflen, &mesg->state.sig);
        if (count < 0) return -1;
        total += count;

        count = hpoint_pack(buf, buflen, &mesg->state.best);
        if (count < 0) return -1;
        total += count;
        break;

    case HMESG_STATUS_FAIL:
        break; // No need to send state.

    default:
        return -1;
    }
    return total;
}

int unpack_state(hmesg_t* mesg, char* buf)
{
    int count = -1, total = 0;

    if (mesg->type == HMESG_SESSION ||
        mesg->type == HMESG_JOIN)
    {
        // Session state doesn't exist just yet.
        return 0;
    }

    switch (mesg->status) {
    case HMESG_STATUS_REQ:
        if (sscanf(buf, " state:%u %u%n", &mesg->state.sig.id,
                   &mesg->state.best.id, &count) < 2)
            return -1;
        total += count;

        count = scanstr_serial(&mesg->state.client, buf + total);
        if (count < 0) return -1;
        total += count;
        break;

    case HMESG_STATUS_OK:
    case HMESG_STATUS_BUSY:
        sscanf(buf, " state:%n", &count);
        if (count < 0) return -1;
        total += count;

        mesg->state.sig.owner = mesg;
        count = hsig_unpack(&mesg->state.sig, buf + total);
        if (count < 0) return -1;
        total += count;

        mesg->state.best.owner = mesg;
        count = hpoint_unpack(&mesg->state.best, buf + total);
        if (count < 0) return -1;
        total += count;
        break;

    case HMESG_STATUS_FAIL:
        break; // No need to send state.

    default:
        return -1;
    }
    return total;
}

int pack_data(char** buf, int* buflen, const hmesg_t* mesg)
{
    int count, total = 0;

    // Pack message data based on message type and status.
    switch (mesg->type) {
    case HMESG_SESSION:
        if (mesg->status == HMESG_STATUS_REQ) {
            count = hsig_pack(buf, buflen, &mesg->state.sig);
            if (count < 0) return -1;
            total += count;

            count = hcfg_pack(buf, buflen, &mesg->data.cfg);
            if (count < 0) return -1;
            total += count;
        }
        break;

    case HMESG_JOIN:
        if (mesg->status == HMESG_STATUS_REQ) {
            count = printstr_serial(buf, buflen, mesg->data.string);
            if (count < 0) return -1;
            total += count;
            break;
        }
        else if (mesg->status == HMESG_STATUS_OK) {
            count = hsig_pack(buf, buflen, &mesg->state.sig);
            if (count < 0) return -1;
            total += count;
        }

    case HMESG_GETCFG:
    case HMESG_SETCFG:
        count = printstr_serial(buf, buflen, mesg->data.string);
        if (count < 0) return -1;
        total += count;
        break;

    case HMESG_FETCH:
        if (mesg->status == HMESG_STATUS_OK) {
            count = hpoint_pack(buf, buflen, &mesg->data.point);
            if (count < 0) return -1;
            total += count;
        }
        break;

    case HMESG_REPORT:
        if (mesg->status == HMESG_STATUS_REQ) {
            count = snprintf_serial(buf, buflen, " %d", mesg->data.point.id);
            if (count < 0) return -1;
            total += count;

            count = hperf_pack(buf, buflen, &mesg->data.perf);
            if (count < 0) return -1;
            total += count;
        }
        break;

    case HMESG_BEST:
    case HMESG_RESTART:
        break;

    default:
        return -1;
    }
    return total;
}

int unpack_data(hmesg_t* mesg, char* buf)
{
    int count, total = 0;

    switch (mesg->type) {
    case HMESG_SESSION:
        if (mesg->status == HMESG_STATUS_REQ) {
            mesg->state.sig.owner = mesg;
            count = hsig_unpack(&mesg->state.sig, buf + total);
            if (count < 0) return -1;
            total += count;

            mesg->data.cfg.owner = mesg;
            count = hcfg_unpack(&mesg->data.cfg, buf + total);
            if (count < 0) return -1;
            total += count;
        }
        break;

    case HMESG_JOIN:
        if (mesg->status == HMESG_STATUS_REQ) {
            count = scanstr_serial(&mesg->data.string, buf + total);
            if (count < 0) return -1;
            total += count;
        }
        else if (mesg->status == HMESG_STATUS_OK) {
            mesg->state.sig.owner = mesg;
            count = hsig_unpack(&mesg->state.sig, buf + total);
            if (count < 0) return -1;
            total += count;
        }
        break;

    case HMESG_GETCFG:
    case HMESG_SETCFG:
        count = scanstr_serial(&mesg->data.string, buf + total);
        if (count < 0) return -1;
        total += count;
        break;

    case HMESG_FETCH:
        if (mesg->status == HMESG_STATUS_OK) {
            mesg->data.point.owner = mesg;
            count = hpoint_unpack(&mesg->data.point, buf + total);
            if (count < 0) return -1;
            total += count;
        }
        break;

    case HMESG_REPORT:
        if (mesg->status == HMESG_STATUS_REQ) {
            if (sscanf(buf + total, " %d%n",
                       &mesg->data.point.id, &count) < 1)
                return -1;
            total += count;

            count = hperf_unpack(&mesg->data.perf, buf + total);
            if (count < 0) return -1;
            total += count;
        }
        break;

    case HMESG_BEST:
    case HMESG_RESTART:
        break;

    default:
        return -1;
    }
    return total;
}
