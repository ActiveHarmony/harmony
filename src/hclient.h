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
 * \file hclient.h
 * \brief Harmony client application function header.
 *
 * All clients must include this file to participate in a Harmony
 * tuning session.
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
 * \defgroup api_desc Harmony Descriptor Management Functions
 *
 * A Harmony descriptor is an opaque structure that stores state
 * associated with a particular tuning session.  The functions within
 * this section allow the user to create and manage the resources
 * controlled by the descriptor.
 *
 * @{
 */

/**
 * \brief Allocate and initialize a new Harmony descriptor.
 *
 * All client API functions require a valid Harmony descriptor to
 * function correctly.  The descriptor is designed to be used as an
 * opaque type and no guarantees are made about the contents of its
 * structure.
 *
 * Similar to MPI's initialization function mpi_init(), harmony_init()
 * optionally accepts the address of the two parameters to main
 * (`argc` and `argv`).  If provided, any string that resembles a
 * Harmony configuration directive will removed from argv and saved
 * elsewhere.  Scanning ends early if a `--` parameter is found.
 * These directives are then applied immediately prior to session
 * launch or client join, whichever comes first.
 *
 * Heap memory is allocated for the descriptor, so be sure to call
 * harmony_fini() when it is no longer needed.
 *
 * \param argc Address of argc parameter from main().
 * \param argv Address of argv parameter from main().
 *
 * \return Returns Harmony descriptor upon success, and `NULL` otherwise.
 */
hdesc_t *harmony_init(int *argc, char ***argv);

/**
 * \brief Release all resources associated with a Harmony client descriptor.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 */
void harmony_fini(hdesc_t *hdesc);

/**
 * @}
 *
 * \defgroup api_sess Session Setup Functions
 *
 * These functions are used to describe and establish new tuning
 * sessions.  Valid sessions must define at least one tuning variable
 * before they are launched.
 *
 * @{
 */

/**
 * \brief Specify a unique name for the Harmony session.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param name  Name to associate with this session.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_session_name(hdesc_t *hdesc, const char *name);

/**
 * \brief Add an integer-domain variable to the Harmony session.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param name  Name to associate with this variable.
 * \param min   Minimum range value (inclusive).
 * \param max   Maximum range value (inclusive).
 * \param step  Minimum search increment.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_int(hdesc_t *hdesc, const char *name,
                 long min, long max, long step);

/**
 * \brief Add a real-domain variable to the Harmony session.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param name  Name to associate with this variable.
 * \param min   Minimum range value (inclusive).
 * \param max   Maximum range value (inclusive).
 * \param step  Minimum search increment.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_real(hdesc_t *hdesc, const char *name,
                  double min, double max, double step);

/**
 * \brief Add an enumeration variable and append to the list of valid values
 *        in this set.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param name  Name to associate with this variable.
 * \param value String that belongs in this enumeration.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_enum(hdesc_t *hdesc, const char *name, const char *value);

/**
 * \brief Specify the search strategy to use in the new Harmony session.
 *
 * \param hdesc    Harmony descriptor returned from harmony_init().
 * \param strategy Filename of the strategy plug-in to use in this session.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_strategy(hdesc_t *hdesc, const char *strategy);

/**
 * \brief Specify the list of plug-ins to use in the new Harmony session.
 *
 * Plug-in layers are specified via a single string of filenames,
 * separated by the colon character (`:`).  The layers are loaded in
 * list order, with each successive layer placed further from the
 * search strategy in the center.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param list  List of plug-ins to load with this session.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_layer_list(hdesc_t *hdesc, const char *list);

/**
 * \brief Instantiate a new Harmony tuning session.
 *
 * After using the session establishment routines (harmony_int,
 * harmony_name, etc.) to specify a tuning session, this function
 * launches a new tuning session.  It may either be started locally or
 * on the Harmony Server located at *host*:*port*.
 *
 * If *host* is `NULL`, its value will be taken from the environment
 * variable `HARMONY_S_HOST`.  If `HARMONY_S_HOST` is not defined, the
 * environment variable `HARMONY_HOME` will be used to initiate a
 * private tuning session, which will be available only to the local
 * process.
 *
 * If *port* is 0, its value will be taken from the environment
 * variable `HARMONY_S_PORT`, if defined.  Otherwise, its value will
 * be taken from the src/defaults.h file, if needed.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param host  Host of the Harmony server (or `NULL`).
 * \param port  Port of the Harmony server.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_launch(hdesc_t *hdesc, const char *host, int port);

/**
 * @}
 *
 * \defgroup api_client Tuning Client Setup Functions
 *
 * These functions prepare the client to join an established tuning
 * session.  Specifically, variables within the client application
 * must be bound to session variables.  This allows the client to
 * change appropriately upon fetching new points from the session.
 *
 * @{
 */

/**
 * \brief Assign an identifying string to this client.
 *
 * Set the client identification string.  All messages sent to the
 * tuning session will be tagged with this string, allowing the
 * framework to distinguish clients from one another.  As such, care
 * should be taken to ensure this string is unique among all clients
 * participating in a tuning session.
 *
 * By default, a string is generated from the hostname, process id,
 * and socket descriptor.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param id    Unique identification string.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_id(hdesc_t *hdesc, const char *id);

/**
 * \brief Bind a local variable of type `long` to an integer-domain
 *        session variable.
 *
 * This function associates a local variable with a session variable
 * declared using harmony_int().  Upon harmony_fetch(), the value
 * chosen by the session will be stored at the address `ptr`.
 *
 * This function must be called for each integer-domain variable
 * defined in the joining session.  Otherwise harmony_join() will fail
 * when called.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param name  Session variable defined using harmony_int().
 * \param ptr   Pointer to a local `long` variable that will
 *              hold the current testing value.
 *
 * \return Returns a harmony descriptor on success, and -1 otherwise.
 */
int harmony_bind_int(hdesc_t *hdesc, const char *name, long *ptr);

/**
 * \brief Bind a local variable of type `double` to a real-domain
 *        session variable.
 *
 * This function associates a local variable with a session variable
 * declared using harmony_real().  Upon harmony_fetch(), the value
 * chosen by the session will be stored at the address `ptr`.
 *
 * This function must be called for each real-domain variable defined
 * in the joining session.  Otherwise harmony_join() will fail when
 * called.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param name  Session variable defined using harmony_real().
 * \param ptr   Pointer to a local `double` variable that will
 *              hold the current testing value.
 *
 * \return Returns a harmony descriptor on success, and -1 otherwise.
 */
int harmony_bind_real(hdesc_t *hdesc, const char *name, double *ptr);

/**
 * \brief Bind a local variable of type `char *` to an enumerated
 *        string-based session variable.
 *
 * This function associates a local variable with a session variable
 * declared using harmony_enum().  Upon harmony_fetch(), the value
 * chosen by the session will be stored at the address `ptr`.
 *
 * This function must be called for each string-based variable defined
 * in the joining session.  Otherwise harmony_join() will fail when
 * called.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param name  Session variable defined using harmony_enum().
 * \param ptr   Pointer to a local `char *` variable that will
 *              hold the current testing value.
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
 * If `host` is `NULL` or `port` is 0, values from the environment
 * variable `HARMONY_S_HOST` or `HARMONY_S_PORT` will be used,
 * respectively.  If either environment variable is not defined,
 * values from defaults.h will be used as a last resort.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
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
 * \param hdesc Harmony descriptor returned from harmony_init().
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_leave(hdesc_t *hdesc);

/**
 * @}
 *
 * \defgroup api_runtime Client/Session Interaction Functions
 *
 * These functions are used by the client after it has joined an
 * established session.
 *
 * @{
 */

/**
 * \brief Get a key value from the session's configuration.
 *
 * Searches the server's configuration system for key, and returns
 * the string value associated with it if found. Heap memory is
 * allocated for the result string.
 *
 * \warning This function allocates memory for the return value.  It
 *          is the user's responsibility to free this memory.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param key   Config key to search for on the server.
 *
 * \return Returns a c-style string on success, and `NULL` otherwise.
 */
char *harmony_getcfg(hdesc_t *hdesc, const char *key);

/**
 * \brief Set a new key/value pair in the session's configuration.
 *
 * Writes the new key/value pair into the server's run-time
 * configuration database.  If the key exists in the database, its
 * value is overwritten.  If val is `NULL`, the key will be erased
 * from the database.  These key/value pairs exist only in memory, and
 * will not be written back to the server's configuration file.
 *
 * \warning This function allocates memory for the return value.  It
 *          is the user's responsibility to free this memory.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param key   Config key to modify on the server.
 * \param val   Config value to associate with the key.
 *
 * \return Returns the original key value string on success and `NULL`
 *         otherwise, setting errno if appropriate.  Since this
 *         function may legitimately return `NULL`, errno must be
 *         cleared pre-call, and checked post-call.
 */
char *harmony_setcfg(hdesc_t *hdesc, const char *key, const char *val);

/**
 * \brief Fetch a new configuration from the Harmony server.
 *
 * If a new configuration is available, this function will update the
 * values of all registered variables.  Otherwise, it will configure
 * the system to run with the best known configuration thus far.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 *
 * \return Returns 0 if no registered variables were modified, 1 if
 *         any registered variables were modified, and -1 otherwise.
 */
int harmony_fetch(hdesc_t *hdesc);

/**
 * \brief Report the performance of a configuration to the Harmony server.
 *
 * Sends a complete performance report regarding the current
 * configuration to the Harmony session.  For multi-objective search
 * problems, the performance parameter should point to an array
 * containing all performance values.
 *
 * If all performance values have been reported via
 * harmony_report_one(), then `NULL` may be passed as the performance
 * pointer.  Unreported performance values will result in error.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param value Performance vector for the current configuration.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_report(hdesc_t *hdesc, double *perf);

/**
 * \brief Report a single performance value for the current configuration.
 *
 * Allows performance values to be reported one at a time for
 * multi-objective search problems.
 *
 * Note that this function only caches values to send to the Harmony
 * session.  Once all values have been reported, harmony_report() must
 * be called (passing `NULL` as the performance argument).
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param index Objective index of the value to report.
 * \param value Performance measured for the current configuration.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_report_one(hdesc_t *hdesc, int index, double value);

/**
 * \brief Sets variables under Harmony's control to the best known
 *        configuration.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_best(hdesc_t *hdesc);

/**
 * \brief Retrieve the convergence state of the current search.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 *
 * \return Returns 1 if the search has converged, 0 if it has not,
 *         and -1 on error.
 */
int harmony_converged(hdesc_t *hdesc);

/**
 * \brief Access the current Harmony error string.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 *
 * \return Returns a pointer to a string that describes the latest
 *         Harmony error, or `NULL` if no error has occurred since
 *         the last call to harmony_error_clear().
 */
const char *harmony_error_string(hdesc_t *hdesc);

/**
 * \brief Clears the error status of the given Harmony descriptor.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 */
void harmony_error_clear(hdesc_t *hdesc);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __HCLIENT_H__ */
