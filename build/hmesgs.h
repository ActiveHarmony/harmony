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

#ifndef __HMESGS_H__
#define __HMESGS_H__

#include <stddef.h>  /* For size_t definition. */

#define MAX_VAL_STRLEN   64 /* Maximum length (+1) for value string   */
#define MAX_MSG_STRLEN 4000 /* Maximum length (+1) for message data   */
#define HMESG_VERSION  0x03 /* Protocol Version                       */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Data structures, definitions, and functions for Harmony variables.
 */
typedef enum {
    HVAL_UNKNOWN = 0x00,
    HVAL_INT     = 0x01, /* Integer domain value */
    HVAL_REAL    = 0x02, /* Real domain value    */
    HVAL_STR     = 0x03, /* String value         */

    HVAL_MAX
} hval_type;

typedef struct {
    hval_type type;
    union {
        long i;
        double r;
        char s[MAX_VAL_STRLEN];
    } value;
} hval_t;

int hvar_serialize(char *, size_t, const hval_t *);
int hvar_deserialize(hval_t *, const char *);

/*
 * Data structures, definitions, and functions for Harmony messages.
 */
typedef enum {
    HMESG_UNKNOWN      = 0x00,
    HMESG_CONFIRM      = 0x01, /* Message ACK                      */
    HMESG_FAIL         = 0x02, /* Message NACK                     */
    HMESG_CLIENT_REG   = 0x03, /* Client registration info         */
    HMESG_CLIENT_UNREG = 0x04, /* Client disconnect                */
    HMESG_APP_DESCR    = 0x05, /* Initial application metadata     */
    HMESG_VAR_DESCR    = 0x06, /* Initial bundle metadata          */
    HMESG_QUERY        = 0x07, /* Query server for non-bundle info */
    HMESG_FETCH        = 0x08, /* Retrieve configuration to test   */
    HMESG_REPORT       = 0x09, /* Report configuration performance */

    HMESG_TYPE_MAX
} hmesg_type;

typedef struct {
    hmesg_type type;
    unsigned short id;
    int timestamp;
    unsigned short count;
    char data[MAX_MSG_STRLEN];
} hmesg_t;

int hmesg_serialize(char *, size_t, const hmesg_t *);
int hmesg_deserialize(hmesg_t *, const char *);

#ifdef __cplusplus
}
#endif

#endif  /* ifndef __HMESGS_H__ */
