/*
 * Copyright 2003-2012 Jeffrey K. Hollingsworth
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

#ifndef __HSESSION_H__
#define __HSESSION_H__

#include "hsignature.h"
#include "hcfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Defined configuration key convention
 */
#define SESSION_CORE_EXECFILE "session-core"
#define SESSION_PLUGIN_SEP    ':'

/*
 * Harmony session structure: Holds bookkeeping info to uniquely
 * identify individual tuning sessions.
 */
typedef struct hsession {
    hsignature_t sig;
    hcfg_t *cfg;
} hsession_t;

int hsession_init(hsession_t *sess);
int hsession_copy(hsession_t *copy, const hsession_t *orig);
void hsession_fini(hsession_t *sess);

/* ----------------------------------------------------------------------------
 * Add an integer-domain variable to the Harmony session description.
 *
 * Params:
 *   sess - Harmony session handle
 *   name - Name to associate with this variable
 *   min  - Minimum range value (inclusive)
 *   max  - Maximum range value (inclusive)
 *   step - Minimum search increment/decrement
 *
 * Returns 0 on success and -1 otherwise.
 */
int hsession_int(hsession_t *sess, const char *name,
                 long min, long max, long step);

/* ----------------------------------------------------------------------------
 * Add a real-domain variable to the Harmony session description.
 *
 * Params:
 *   sess - Harmony session handle
 *   name - Name to associate with this variable
 *   min  - Minimum range value (inclusive)
 *   max  - Maximum range value (inclusive)
 *   step - Minimum search increment/decrement
 *
 * Returns 0 on success and -1 otherwise.
 */
int hsession_real(hsession_t *sess, const char *name,
                  double min, double max, double step);

/* ----------------------------------------------------------------------------
 * Add an enumeration variable and append to the list of valid values
 * in this set.
 *
 * Params:
 *   sess  - Harmony handle returned from harmony_init()
 *   name  - Name to associate with this variable
 *   value - Value that belongs in this enumeration.
 *
 * Returns 0 on success and -1 otherwise.
 */
int hsession_enum(hsession_t *sess, const char *name, const char *value);

int hsession_name(hsession_t *sess, const char *name);
int hsession_strategy(hsession_t *sess, const char *strategy);
int hsession_plugins(hsession_t *sess, const char *plugins);
int hsession_cfg(hsession_t *sess, const char *key, const char *var);

/* ----------------------------------------------------------------------------
 * Connect to the Harmony server at host:port, and begin a new tuning
 * session.
 *
 * Returns NULL on success.  Otherwise, a string describing the error
 * condition will be returned to the user.
 */
const char *hsession_launch(hsession_t *sess, const char *host, int port);

int hsession_serialize(char **buf, int *buflen, const hsession_t *sess);
int hsession_deserialize(hsession_t *sess, char *buf);

#ifdef __cplusplus
}
#endif

#endif /* __HSESSION_H__ */
