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

#ifndef __HCLIENT_H__
#define __HCLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

struct hdesc_t;
typedef struct hdesc_t hdesc_t;

/* ----------------------------------------------------------------------------
 * Allocate and initialize a Harmony descriptor.
 *
 * Returns a harmony handle on success, and NULL otherwise.
 */
hdesc_t *harmony_init(void);

/* ----------------------------------------------------------------------------
 * Frees all resources associated with a Harmony descriptor.
 */
void harmony_fini(hdesc_t *hdesc);

/* ----------------------------------------------------------------------------
 * Binds local memory to tunable variables from the Harmony Server.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *   name  - Tunable variable name defined using hsession_xxx() routines
 *   ptr   - Pointer to local memory that will hold configuration values
 *
 * Returns a harmony handle on success, and -1 otherwise.
 */
int harmony_bind_int(hdesc_t *hdesc, const char *name, long *ptr);
int harmony_bind_real(hdesc_t *hdesc, const char *name, double *ptr);
int harmony_bind_enum(hdesc_t *hdesc, const char *name, const char **ptr);

/* ----------------------------------------------------------------------------
 * Join a Harmony tuning session.
 *
 * Establishes a connection with the Harmony Server on a specific host
 * and port, and joins the named session.  All variables must be bound
 * to local memory via harmony_bind() for this call to succeed.
 *
 * If host is NULL or port is 0, values from the environment variable
 * HARMONY_S_HOST or HARMONY_S_PORT will be used, respectively.  If
 * either environment variable is not defined, values from defaults.h
 * will be used as a last resort.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *   host  - Host of the server
 *   port  - Port of the server
 *   sess  - Name of an existing tuning session
 *
 * Returns a harmony handle on success, and -1 otherwise.
 */
int harmony_join(hdesc_t *hdesc, const char *host, int port, const char *sess);

/* ----------------------------------------------------------------------------
 * Leave a Harmony tuning session.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *
 * Returns 0 on success, and -1 otherwise.
 */
int harmony_leave(hdesc_t *hdesc);

/* ----------------------------------------------------------------------------
 * Query the server's key/value configuration system.
 *
 * Searches the server's configuration system for key, and returns the
 * string value associated with it if found. Heap memory is allocated
 * for the result string.
 *
 * This function allocates memory for the return value.  It is the
 * user's responsibility to free this memory.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *   key   - Config key to search for on the server
 *
 * Returns a c-style string on success, and NULL otherwise.
 */
char *harmony_query(hdesc_t *hdesc, const char *key);

/* ----------------------------------------------------------------------------
 * Inform the server of a new key/value pair.
 *
 * Writes the new key/value pair into the server's run-time configuration
 * database.  If the key exists in the database, its value is overwritten.
 * If val is NULL, the key will be erased from the database.  These key/value
 * pairs exist only in memory, and will not be written back to the server's
 * configuration file.
 *
 * This function allocates memory for the return value.  It is the
 * user's responsibility to free this memory.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *   key   - Config key to modify on the server
 *   val   - Config value to associate with the key
 *
 * Returns the original key value string on success and NULL
 * otherwise, setting errno if appropriate.  Since this function may
 * legitimately return NULL, errno must be cleared pre-call, and
 * checked post-call.
 */
char *harmony_inform(hdesc_t *hdesc, const char *key, const char *val);

/* ----------------------------------------------------------------------------
 * Fetch a new configuration from the Harmony server.
 *
 * If a new configuration is available, this function will update the values of
 * all registered variables.  Otherwise, it will configure the system to run
 * with the best known configuration thus far.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *
 * Returns 0 if no registered variables were modified, 1 if any registered
 * variables were modified, and -1 otherwise.
 */
int harmony_fetch(hdesc_t *hdesc);

/* ----------------------------------------------------------------------------
 * Report the performance of a configuration to the Harmony server.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *   value - Performance measured for the current configuration
 *
 * Returns 0 on success, and -1 otherwise.
 */
int harmony_report(hdesc_t *hdesc, double value);

/* ----------------------------------------------------------------------------
 * Retrieve the convergence state of the current search.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *
 * Returns 1 if the search has converged, 0 if it has not, and -1 on error.
 */
int harmony_converged(hdesc_t *hdesc);

/* ----------------------------------------------------------------------------
 * Returns a pointer to a string that describes the latest Harmony
 * error, or NULL if no error has occurred since the last call to
 * harmony_error_clear().
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 */
const char *harmony_error_string(hdesc_t *hdesc);

/* ----------------------------------------------------------------------------
 * Clears the error status of the given Harmony descriptor.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 */
void harmony_error_clear(hdesc_t *hdesc);

#ifdef __cplusplus
}
#endif

#endif /* __HCLIENT_H__ */
