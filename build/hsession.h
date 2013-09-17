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

/**
 * \file
 * \brief Harmony session creation routines.
 *
 * Provides functions to create new Harmony sessions by specifying
 * properties such as tuning variables and the session's unique
 * identifier.
 *
 * Once created (aka launched), Harmony sessions cannot be modified.
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
#ifndef DOXYGEN_SKIP

#define SESSION_CORE_EXECFILE "session-core"
#define SESSION_LAYER_SEP     ':'

typedef struct hsession {
    hsignature_t sig;
    hcfg_t *cfg;
} hsession_t;

#endif

/**
 * \brief Initialize a new Harmony session descriptor.
 *
 * \param sess Session structure to initialize.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int hsession_init(hsession_t *sess);

/**
 * \brief Fully copy a Harmony session descriptor.
 *
 * Performs a deep copy of a given Harmony session descriptor.
 *
 * \param[in,out] copy Destination of copy operation.
 * \param[in]     orig Source of copy operation.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int hsession_copy(hsession_t *copy, const hsession_t *orig);

/**
 * \brief Release all resources associated with a Harmony session descriptor.
 *
 * \param sess Harmony descriptor initialized by
 *             [hsession_init()](\ref hsession_init).
 */
void hsession_fini(hsession_t *sess);

/**
 * \brief Add an integer-domain variable to the Harmony session.
 *
 * \param sess Harmony descriptor initialized by
 *             [hsession_init()](\ref hsession_init).
 * \param name Name to associate with this variable.
 * \param min  Minimum range value (inclusive).
 * \param max  Maximum range value (inclusive).
 * \param step Minimum search increment.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int hsession_int(hsession_t *sess, const char *name,
                 long min, long max, long step);

/**
 * \brief Add a real-domain variable to the Harmony session.
 *
 * \param sess Harmony descriptor initialized by
 *             [hsession_init()](\ref hsession_init).
 * \param name Name to associate with this variable.
 * \param min  Minimum range value (inclusive).
 * \param max  Maximum range value (inclusive).
 * \param step Minimum search increment.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int hsession_real(hsession_t *sess, const char *name,
                  double min, double max, double step);

/**
 * \brief Add an enumeration variable and append to the list of valid values
 *        in this set.
 *
 * \param sess  Harmony descriptor initialized by
 *              [hsession_init()](\ref hsession_init).
 * \param name  Name to associate with this variable.
 * \param value String that belongs in this enumeration.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int hsession_enum(hsession_t *sess, const char *name, const char *value);

/**
 * \brief Specify a unique name for the Harmony session.
 *
 * \param sess Harmony descriptor initialized by
 *             [hsession_init()](\ref hsession_init).
 * \param name Name to associate with this session.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int hsession_name(hsession_t *sess, const char *name);

/**
 * \brief Specify the search strategy to use in the new Harmony session.
 *
 * \param sess     Harmony descriptor initialized by
 *                 [hsession_init()](\ref hsession_init).
 * \param strategy Filename of the strategy plug-in to use in this session.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int hsession_strategy(hsession_t *sess, const char *strategy);

/**
 * \brief Specify the list of plug-ins to use in the new Harmony session.
 *
 * Plug-in layers are specified via a single string of filenames,
 * separated by the colon character (`:`).  The layers are loaded in
 * list order, with each successive layer placed further from the
 * search strategy in the center.
 *
 * \param sess    Harmony descriptor initialized by
 *                [hsession_init()](\ref hsession_init).
 * \param plugins List of plug-ins to load with this session.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int hsession_plugins(hsession_t *sess, const char *plugins);

/**
 * /brief Specify a key/value pair within the new Harmony session.
 *
 * \param sess Harmony descriptor initialized by
 *             [hsession_init()](\ref hsession_init).
 * \param key  Config key to introduce in the session.
 * \param val  Config value to associate with the key.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int hsession_setcfg(hsession_t *sess, const char *key, const char *val);

/**
 * \brief Instantiate a new Harmony tuning session.
 *
 * The new session will be launched on the Harmony server located at
 * <var>host</var>:<var>port</var>.
 *
 * If <var>host</var> is `NULL` or <var>port</var> is `0`, values from
 * the environment variable `HARMONY_S_HOST` or `HARMONY_S_PORT` will
 * be used, respectively.  If either environment variable is not
 * defined, values from defaults.h will be used as a last resort.
 *
 * \param sess Harmony descriptor initialized by
 *             [hsession_init()](\ref hsession_init).
 * \param host Host of the Harmony server.
 * \param port Port of the Harmony server.
 *
 * \return Returns `NULL` on success.  Otherwise, a string describing
 *         the error condition will be returned to the user.
 */
const char *hsession_launch(hsession_t *sess, const char *host, int port);

#ifndef DOXYGEN_SKIP

int hsession_serialize(char **buf, int *buflen, const hsession_t *sess);
int hsession_deserialize(hsession_t *sess, char *buf);

#endif

#ifdef __cplusplus
}
#endif

#endif /* __HSESSION_H__ */
