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
 *  \file
 *  \brief Harmony client application routines.
 *
 *  All clients must include this file to participate in a Harmony
 *  tuning session.
 */

#ifndef __HCLIENT_H__
#define __HCLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DOXYGEN_SKIP

struct hdesc_t;
typedef struct hdesc_t hdesc_t;

#endif

/**
 * \brief Allocate and initialize a new Harmony client descriptor.
 *
 * \return Returns Harmony descriptor upon success, and NULL otherwise.
 */
hdesc_t *harmony_init(void);

/**
 * \brief Release all resources associated with a Harmony client descriptor.
 *
 * \param hdesc Harmony descriptor returned from
 *              [harmony_init()](\ref harmony_init).
 */
void harmony_fini(hdesc_t *hdesc);

/**
 * \brief Bind a local variable of type `long` to an integer-domain
 *        session variable.
 *
 * This function associates a local variable with a session variable
 * declared using [hsession_int()](\ref hsession_int).  Upon
 * [harmony_fetch()](\ref harmony_fetch), the value chosen by the
 * session will be stored at the address <var>ptr</var>.
 *
 * This function must be called for each integer-domain variable
 * defined in the joining session.  Otherwise
 * [harmony_join()](\ref harmony_join) will fail when called.
 *
 * \param hdesc Harmony descriptor returned from
 *              [harmony_init()](\ref harmony_init).
 * \param name  Session variable defined using
 *              [hsession_int()](\ref hsession_int).
 * \param ptr   Pointer to a local `long` variable that will
 *                  hold the current testing value.
 *
 * \return Returns a harmony descriptor on success, and -1 otherwise.
 */
int harmony_bind_int(hdesc_t *hdesc, const char *name, long *ptr);

/**
 * \brief Bind a local variable of type `double` to a real-domain
 *        session variable.
 *
 * This function associates a local variable with a session variable
 * declared using [hsession_real()](\ref hsession_real).  Upon
 * [harmony_fetch()](\ref harmony_fetch), the value chosen by the
 * session will be stored at the address <var>ptr</var>.
 *
 * This function must be called for each real-domain variable
 * defined in the joining session.  Otherwise
 * [harmony_join()](\ref harmony_join) will fail when called.
 *
 * \param hdesc Harmony descriptor returned from
 *              [harmony_init()](\ref harmony_init).
 * \param name  Session variable defined using
 *              [hsession_real()](\ref hsession_real).
 * \param ptr   Pointer to a local `double` variable that will
 *                  hold the current testing value.
 *
 * \return Returns a harmony descriptor on success, and -1 otherwise.
 */
int harmony_bind_real(hdesc_t *hdesc, const char *name, double *ptr);

/**
 * \brief Bind a local variable of type `char *` to a enumerated
 *        string-based session variable.
 *
 * This function associates a local variable with a session variable
 * declared using [hsession_enum()](\ref hsession_enum).  Upon
 * [harmony_fetch()](\ref harmony_fetch), the value chosen by the
 * session will be stored at the address <var>ptr</var>.
 *
 * This function must be called for each string-based variable
 * defined in the joining session.  Otherwise
 * [harmony_join()](\ref harmony_join) will fail when called.
 *
 * \param hdesc Harmony descriptor returned from
 *              [harmony_init()](\ref harmony_init).
 * \param name  Session variable defined using
 *              [hsession_enum()](\ref hsession_enum).
 * \param ptr   Pointer to a local `char *` variable that will
 *                  hold the current testing value.
 *
 * \return Returns a harmony descriptor on success, and -1 otherwise.
 */
int harmony_bind_enum(hdesc_t *hdesc, const char *name, const char **ptr);

/**
 * \brief Join an established Harmony tuning session.
 *
 * Establishes a connection with the Harmony Server on a specific
 * host and port, and joins the named session.  All variables must be
 * bound to local memory via harmony_bind() for this call to succeed.
 *
 * If <var>host</var> is `NULL` or <var>port</var> is 0, values from
 * the environment variable `HARMONY_S_HOST` or `HARMONY_S_PORT` will
 * be used, respectively.  If either environment variable is not
 * defined, values from defaults.h will be used as a last resort.
 *
 * \param hdesc Harmony descriptor returned from
 *              [harmony_init()](\ref harmony_init).
 * \param host  Host of the Harmony server.
 * \param port  Port of the Harmony server.
 * \param sess  Name of an existing tuning session on the server.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_join(hdesc_t *hdesc, const char *host, int port, const char *sess);

/**
 * \brief Leave a Harmony tuning session.
 *
 * End participation in a Harmony tuning session by closing the
 * connection to the Harmony server.
 *
 * \param hdesc Harmony descriptor returned from
 *              [harmony_init()](\ref harmony_init).
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_leave(hdesc_t *hdesc);

/**
 * \brief Query the server's key/value configuration system.
 *
 * Searches the server's configuration system for key, and returns
 * the string value associated with it if found. Heap memory is
 * allocated for the result string.
 *
 * \warning This function allocates memory for the return value.  It
 *          is the user's responsibility to free this memory.
 *
 * \param hdesc Harmony descriptor returned from
 *              [harmony_init()](\ref harmony_init).
 * \param key   Config key to search for on the server.
 *
 * \return Returns a c-style string on success, and NULL otherwise.
 */
char *harmony_query(hdesc_t *hdesc, const char *key);

/**
 * \brief Inform the server of a new key/value pair.
 *
 * Writes the new key/value pair into the server's run-time
 * configuration database.  If the key exists in the database, its
 * value is overwritten.  If val is NULL, the key will be erased from
 * the database.  These key/value pairs exist only in memory, and
 * will not be written back to the server's configuration file.
 *
 * \warning This function allocates memory for the return value.  It
 *          is the user's responsibility to free this memory.
 *
 * \param hdesc Harmony descriptor returned from
 *              [harmony_init()](\ref harmony_init).
 * \param key   Config key to modify on the server.
 * \param val   Config value to associate with the key.
 *
 * \return Returns the original key value string on success and NULL
 *         otherwise, setting errno if appropriate.  Since this
 *         function may legitimately return NULL, errno must be
 *         cleared pre-call, and checked post-call.
 */
char *harmony_inform(hdesc_t *hdesc, const char *key, const char *val);

/**
 * \brief Fetch a new configuration from the Harmony server.
 *
 * If a new configuration is available, this function will update the
 * values of all registered variables.  Otherwise, it will configure
 * the system to run with the best known configuration thus far.
 *
 * \param hdesc Harmony descriptor returned from
 *              [harmony_init()](\ref harmony_init).
 *
 * \return Returns 0 if no registered variables were modified, 1 if
 *         any registered variables were modified, and -1 otherwise.
 */
int harmony_fetch(hdesc_t *hdesc);

/**
 * \brief Report the performance of a configuration to the Harmony server.
 *
 * \param hdesc Harmony descriptor returned from
 *              [harmony_init()](\ref harmony_init).
 * \param value Performance measured for the current configuration.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_report(hdesc_t *hdesc, double value);

/**
 * \brief Sets variables under Harmony's control to the best known
 *        configuration.
 *
 * \param hdesc Harmony descriptor returned from
 *              [harmony_init()](\ref harmony_init).
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_best(hdesc_t *hdesc);

/**
 * \brief Retrieve the convergence state of the current search.
 *
 * \param hdesc Harmony descriptor returned from
 *              [harmony_init()](\ref harmony_init).
 *
 * \return Returns 1 if the search has converged, 0 if it has not,
 *         and -1 on error.
 */
int harmony_converged(hdesc_t *hdesc);

/**
 * \brief Access the current Harmony error string.
 *
 * \param hdesc Harmony descriptor returned from
 *              [harmony_init()](\ref harmony_init).
 *
 * \return Returns a pointer to a string that describes the latest
 *         Harmony error, or NULL if no error has occurred since the
 *         last call to harmony_error_clear().
 */
const char *harmony_error_string(hdesc_t *hdesc);

/**
 * \brief Clears the error status of the given Harmony descriptor.
 *
 * \param hdesc Harmony descriptor returned from
 *              [harmony_init()](\ref harmony_init).
 */
void harmony_error_clear(hdesc_t *hdesc);

#ifdef __cplusplus
}
#endif

#endif /* __HCLIENT_H__ */
