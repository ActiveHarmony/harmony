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

#ifndef __HMESG_H__
#define __HMESG_H__

#include "hpoint.h"
#include "hsession.h"

#define HMESG_MAGIC   0x5261793a /* Magic number for packet identification. */
#define HMESG_VERSION 0x04       /* Protocol version.                       */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Data structures, definitions, and functions for Harmony messages.
 */
typedef enum {
    HMESG_UNKNOWN = 0x00,
    HMESG_SESSION, /* Tuning session description           */
    HMESG_JOIN,    /* Client registration info             */
    HMESG_QUERY,   /* Query server for non-bundle info     */
    HMESG_INFORM,  /* Inform server of new non-bundle info */
    HMESG_FETCH,   /* Retrieve configuration to test       */
    HMESG_REPORT,  /* Report configuration performance     */

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

typedef struct {
    int dest;
    hmesg_type type;
    hmesg_status status;
    int index;
    union {
        hsession_t session;
        hsignature_t join;
        struct mesg_fetch {
            hpoint_t cand;
            hpoint_t best;
        } fetch;
        struct mesg_report {
            hpoint_t cand;
            double perf;
        } report;
        const char *string;
    } data;

    char *buf;
    int buflen;
} hmesg_t;
extern hmesg_t HMESG_INITIALIZER;

void hmesg_init(hmesg_t *mesg);
void hmesg_scrub(hmesg_t *mesg);
int  hmesg_copy(hmesg_t *copy, const hmesg_t *orig);
void hmesg_fini(hmesg_t *mesg);
int hmesg_serialize(hmesg_t *mesg);
int hmesg_deserialize(hmesg_t *mesg);

#ifdef __cplusplus
}
#endif

#endif  /* ifndef __HMESG_H__ */
