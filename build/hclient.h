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

/******************************
 * This is the header file for the client side of harmony.
 * 
 * Author: Cristian Tapus
 * History:
 *   July 9, 2000 - first version
 *   July 15, 2000 - comments and update added by Cristian Tapus
 *   Nov  20, 2000 - some changes made by Dejan Perkovic
 *   Dec  20, 2000 - comments and updates added by Cristian Tapus
 *   2010 : fix for multiple server connection by George Teodoro
 *   2004-2010 : various fixes and additions by Ananta Tiwari
 *******************************/

#ifndef __HCLIENT_H__
#define __HCLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

struct hdesc_t;
typedef struct hdesc_t hdesc_t;

typedef enum harmony_iomethod_t {
    HARMONY_IO_POLL,
    HARMONY_IO_ASYNC,

    HARMONY_DATA_MAX
} harmony_iomethod_t;

typedef enum bundle_scope_t {
    BUNDLE_SCOPE_GLOBAL,
    BUNDLE_SCOPE_LOCAL,

    BUNDLE_SCOPE_MAX
} bundle_scope_t;

/* ----------------------------------------------------------------------------
 * Initialize a Harmony descriptor.
 *
 * Each Harmony session requires a unique harmony descriptor.  An application
 * name is required to distinguish client groups on the server.  The iomethod
 * parameter determines if the client will receive new configurations from
 * the server asynchronously (via signals), or by polling (via the
 * harmony_fetch function).
 *
 * Params:
 *   name     - Client application name
 *   iomethod - Client/Server communication method
 *
 * Returns a harmony handle on success, and -1 otherwise.
 */
hdesc_t *harmony_init(const char *name, harmony_iomethod_t iomethod);

/* ----------------------------------------------------------------------------
 * Connect to the Harmony server.
 *
 * Establishes a connection with the harmony server.  If host is NULL or port
 * is 0, the environment variable HARMONY_S_HOST or HARMONY_S_PORT will be
 * used, respectively.
 *
 * All variables must be registered prior to calling this function.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *   host  - Host of the server
 *   port  - Port of the server
 *
 * Returns a harmony handle on success, and -1 otherwise.
 */
int harmony_connect(hdesc_t *hdesc, const char *host, int port);

/* ----------------------------------------------------------------------------
 * Re-connect to the Harmony server.
 *
 * Use this function to migrate a harmony client, and continue its processing
 * elsewhere.  An additional client id is necessary to inform the server which
 * client is being migrated.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *   host  - Host of the server
 *   port  - Port of the server
 *   cid   - Original ID of client
 *
 * Returns a harmony handle on success, and -1 otherwise.
 */
int harmony_reconnect(hdesc_t *hdesc, const char *host, int port, int cid);

/* ----------------------------------------------------------------------------
 * Close the connection to the Harmony server.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *
 * Returns 0 on success, and -1 otherwise.
 */
int harmony_disconnect(hdesc_t *hdesc);

/* ----------------------------------------------------------------------------
 * Query the server's key/value configuration system.
 *
 * Requests the value associated with the parameter key on the Harmony server.
 * Heap memory is allocated for the result string.
 *
 * It is the caller's responsibility to free the result string.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *   key   - Config key to search for on the server
 *
 * Returns a c-style string on success, and NULL otherwise.
 */
char *harmony_query(hdesc_t *hdesc, const char *key);

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
 * Retrieve the best known configuration thus far from the Harmony server.
 *
 * The result is a heap memory allocated string consisting of the values of
 * the registered variables (in the order they were registered), joined by
 * underscores.
 *
 * It is the caller's responsibility to free the result string.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *
 * Returns a c-style string on success, and NULL otherwise.
 */
char *harmony_best_config(hdesc_t *hdesc);

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
 * Returns 1 if the search has converged, 0 if it has not, and -1 otherwise.
 */
int harmony_converged(hdesc_t *hdesc);

/* ----------------------------------------------------------------------------
 * Register a variable with the Harmony system.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *   name  - Name to associate with this variable
 *   ptr   - Pointer to the pre-existing client variable
 *   min   - Minimum range value (inclusive)
 *   max   - Maximum range value (inclusive)
 *   step  - Minimum search increment/decrement
 *
 * Returns 0 on success and -1 otherwise.
 */
int harmony_register_int(hdesc_t *hdesc, const char *name, void *ptr,
                         int min, int max, int step);

/* ----------------------------------------------------------------------------
 * Register a variable with the Harmony system.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *   name  - Name to associate with this variable
 *   ptr   - Pointer to the pre-existing client variable
 *   min   - Minimum range value (inclusive)
 *   max   - Maximum range value (inclusive)
 *   step  - Minimum search increment/decrement
 *
 * Returns 0 on success and -1 otherwise.
 */
int harmony_register_real(hdesc_t *hdesc, const char *name, void *ptr,
                          double min, double max, double step);

/* ----------------------------------------------------------------------------
 * Register a variable with the Harmony system.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *   name  - Name to associate with this variable
 *   ptr   - Pointer to the pre-existing client variable
 *
 * Returns 0 on success and -1 otherwise.
 */
int harmony_register_enum(hdesc_t *hdesc, const char *name, void *ptr);

/* ----------------------------------------------------------------------------
 * Add a member to the Harmony variable's enumeration set.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *   name  - Name of the Harmony variable to constrain
 *   value - Name of the enumeration member to add
 *
 * Returns 0 on success, and -1 otherwise.
 */
int harmony_range_enum(hdesc_t *hdesc, const char *name, const char *value);

/* ----------------------------------------------------------------------------
 * Unregister a variable from the Harmony system.
 *
 * Params:
 *   hdesc - Harmony handle returned from harmony_init()
 *   name  - Name of the Harmony variable to unregister
 *
 * Returns 0 on success, and -1 otherwise.
 */
int harmony_unregister(hdesc_t *hdesc, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* __HCLIENT_H__ */
