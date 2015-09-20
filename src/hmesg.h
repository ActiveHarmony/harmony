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

#ifndef __HMESG_H__
#define __HMESG_H__

#include "hsig.h"
#include "hcfg.h"
#include "hpoint.h"
#include "hperf.h"

/* Message header layout:
 *
 *  0             15 16            31
 * |----------------|----------------|
 * |          HARMONY_MAGIC          |
 * |---------------------------------|
 * |          Message Length         |
 * |---------------------------------|
 * |  HMESG_VERSION |   Origin ID    |
 * |---------------------------------|
 * |          Message Data           |
 * |               ...               |
 */

/* Offset and size of static header members in a byte-stream. */
#define HMESG_MAGIC_OFFSET    0
#define HMESG_MAGIC_SIZE      4
#define HMESG_LENGTH_OFFSET   4
#define HMESG_LENGTH_SIZE     4
#define HMESG_VERSION_OFFSET  8
#define HMESG_VERSION_SIZE    2
#define HMESG_ORIGIN_OFFSET  10
#define HMESG_ORIGIN_SIZE     2
#define HMESG_HDR_SIZE       12

/* Magic number for messages between the harmony server and its clients.    */
#define HMESG_OLD_MAGIC 0x5261793a /* Magic number for packets (pre v4.5).  */
#define HMESG_MAGIC     0x5261797c /* Magic number for packet.              */
#define HMESG_VERSION   0x05       /* Protocol version.                     */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Data structures, definitions, and functions for Harmony messages.
 */
typedef enum {
    HMESG_UNKNOWN = 0x00,
    HMESG_SESSION, /* Tuning session description            */
    HMESG_JOIN,    /* Client registration info              */
    HMESG_GETCFG,  /* Get session cfg key/value pair        */
    HMESG_SETCFG,  /* Set new session cfg key/value pair    */
    HMESG_BEST,    /* Retrieve best known point             */
    HMESG_FETCH,   /* Retrieve search space point to test   */
    HMESG_REPORT,  /* Report search space point performance */
    HMESG_RESTART, /* Restart the search                    */

    HMESG_TYPE_MAX
} hmesg_type;

typedef enum {
    HMESG_STATUS_UNKNOWN = 0x00,
    HMESG_STATUS_REQ,  /* Initial request message          */
    HMESG_STATUS_OK,   /* Request acknowledged             */
    HMESG_STATUS_FAIL, /* Request failed                   */
    HMESG_STATUS_BUSY, /* Server could not respond in time */

    HMESG_STATUS_MAX
} hmesg_status;

/** \brief The hmesg_t structure.
 */
typedef struct {
    int origin;
    hmesg_type type;
    hmesg_status status;
    const char* src_id;

    union {
        struct mesg_session {
            hsig_t sig;
            hcfg_t cfg;
        } session;
        hsig_t join;
        hpoint_t point;
        struct mesg_report {
            int cand_id;
            hperf_t* perf;
        } report;
        const char* string;
    } data;

    char* recv_buf;
    int   recv_len;
    char* send_buf;
    int   send_len;
} hmesg_t;
#define HMESG_INITIALIZER {0}
extern const hmesg_t hmesg_zero;

void hmesg_scrub(hmesg_t* mesg);
void hmesg_fini(hmesg_t* mesg);
int  hmesg_serialize(hmesg_t* mesg);
int  hmesg_deserialize(hmesg_t* mesg);

#ifdef __cplusplus
}
#endif

#endif  /* ifndef __HMESG_H__ */
