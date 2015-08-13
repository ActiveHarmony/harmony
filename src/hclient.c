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
#define _XOPEN_SOURCE 600 // Needed for gethostname()

#include "hclient.h"
#include "hsession.h"
#include "hutil.h"
#include "hmesg.h"
#include "hpoint.h"
#include "hperf.h"
#include "hsockutil.h"
#include "hcfg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <limits.h>

typedef enum harmony_state_t {
    HARMONY_STATE_UNKNOWN,
    HARMONY_STATE_INIT,
    HARMONY_STATE_CONNECTED,
    HARMONY_STATE_READY,
    HARMONY_STATE_TESTING,
    HARMONY_STATE_CONVERGED,

    HARMONY_STATE_MAX
} harmony_state_t;

#define MAX_HOSTNAME_STRLEN 64
char default_id_buf[MAX_HOSTNAME_STRLEN + 32] = {0};

typedef struct hvarloc_t {
    void* ptr;
    hval_t val;
} hvarloc_t;

struct hdesc_t {
    harmony_state_t state;
    int socket;
    char* id;

    hsession_t sess;
    hmesg_t mesg;

    hvarloc_t* varloc;
    int varloc_cap;

    hperf_t* perf;
    hpoint_t curr, best;

    char* buf;
    int buflen;
    const char* errstr;
};

/* ---------------------------------------------------
 * Forward declarations for internal helper functions.
 */
char* default_id(int n);
int   send_request(hdesc_t* hdesc, hmesg_type msg_type);
int   set_values(hdesc_t* hdesc, const hpoint_t* pt);
int   var_index(hdesc_t* hdesc, const char* name);
int   varloc_extend(hdesc_t* hdesc);
int   varloc_setptr(hdesc_t* hdesc, const char* name, void* ptr);
int   update_best(hdesc_t* hdesc, const hpoint_t* pt);

static int debug_mode = 0;

/* ----------------------------------
 * Public Client API Implementations.
 */

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
 * Heap memory is allocated for the descriptor, so be sure to call
 * harmony_fini() when it is no longer needed.
 *
 * \return Returns Harmony descriptor upon success, and `NULL` otherwise.
 */
hdesc_t* harmony_init()
{
    hdesc_t* hdesc = malloc( sizeof(*hdesc) );
    if (!hdesc)
        return NULL;

    hdesc->sess = HSESSION_INITIALIZER;
    if (hcfg_init(&hdesc->sess.cfg) != 0)
        return NULL;

    hdesc->mesg = HMESG_INITIALIZER;
    hdesc->state = HARMONY_STATE_INIT;

    hdesc->varloc = NULL;
    hdesc->varloc_cap = 0;

    hdesc->curr = HPOINT_INITIALIZER;
    hdesc->best = HPOINT_INITIALIZER;

    hdesc->buf = NULL;
    hdesc->buflen = 0;
    hdesc->errstr = NULL;

    hdesc->id = NULL;

    return hdesc;
}

/**
 * \brief Find configuration directives in the command-line arguments.
 *
 * This function iterates through the command-line arguments (as
 * passed to the main() function) to search for Harmony configuration
 * directives in the form NAME=VAL.  Any arguments matching this
 * pattern are added to the configuration and removed from `argv`.
 *
 * Iteration through the command-line arguments stops prematurely if
 * a double-dash ("--") token is found.  All non-directive arguments
 * are shifted to the front of `argv`.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param argc Number of arguments contained in `argv`.
 * \param argv Array of command-line argument strings to search.
 *
 * \return Returns the number of directives successfully taken from
 *         `argv`, or -1 on error.
 */
int harmony_parse_args(hdesc_t* hdesc, int argc, char** argv)
{
    int skip = 0;
    int tail = 0;

    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "--") == 0)
            skip = 1;

        char* sep = strchr(argv[i], '=');
        if (!skip && sep) {
            *sep = '\0';
            if (hcfg_set(&hdesc->sess.cfg, argv[i], sep + 1) != 0) {
                *sep = '=';
                sep = NULL;
            }
        }

        if (!sep) {
            argv[tail++] = argv[i];
        }
    }
    return argc - tail;
}

/**
 * \brief Release all resources associated with a Harmony client descriptor.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 */
void harmony_fini(hdesc_t* hdesc)
{
    if (hdesc) {
        hmesg_fini(&hdesc->mesg);
        hsession_fini(&hdesc->sess);
        hpoint_fini(&hdesc->curr);
        hpoint_fini(&hdesc->best);
        hperf_fini(hdesc->perf);

        if (hdesc->id && hdesc->id != default_id_buf)
            free(hdesc->id);

        free(hdesc->buf);
        free(hdesc->varloc);
        free(hdesc);
    }
}

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
int harmony_session_name(hdesc_t* hdesc, const char* name)
{
    return hsignature_name(&hdesc->sess.sig, name);
}

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
int harmony_int(hdesc_t* hdesc, const char* name,
                long min, long max, long step)
{
    return hsignature_int(&hdesc->sess.sig, name, min, max, step);
}

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
int harmony_real(hdesc_t* hdesc, const char* name,
                 double min, double max, double step)
{
    return hsignature_real(&hdesc->sess.sig, name, min, max, step);
}

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
int harmony_enum(hdesc_t* hdesc, const char* name, const char* value)
{
    return hsignature_enum(&hdesc->sess.sig, name, value);
}

/**
 * \brief Specify the search strategy to use in the new Harmony session.
 *
 * \param hdesc    Harmony descriptor returned from harmony_init().
 * \param strategy Filename of the strategy plug-in to use in this session.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_strategy(hdesc_t* hdesc, const char* strategy)
{
    return hcfg_set(&hdesc->sess.cfg, CFGKEY_STRATEGY, strategy);
}

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
int harmony_layers(hdesc_t* hdesc, const char* list)
{
    return hcfg_set(&hdesc->sess.cfg, CFGKEY_LAYERS, list);
}

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
int harmony_launch(hdesc_t* hdesc, const char* host, int port)
{
    /* Sanity check input */
    if (hdesc->sess.sig.range_len < 1) {
        hdesc->errstr = "No tuning variables defined";
        errno = EINVAL;
        return -1;
    }

    if (varloc_extend(hdesc) != 0) {
        return -1;
    }

    if (!host) {
        host = hcfg_get(&hdesc->sess.cfg, CFGKEY_HARMONY_HOST);
    }

    if (port == 0) {
        port = hcfg_int(&hdesc->sess.cfg, CFGKEY_HARMONY_PORT);
    }

    if (!host) {
        char* path;
        const char* home;

        /* Provide a default name, if one isn't defined. */
        if (!hdesc->sess.sig.name) {
            if (hsignature_name(&hdesc->sess.sig, "NONAME"))
                return -1;
        }

        /* Find the Active Harmony installation. */
        home = hcfg_get(&hdesc->sess.cfg, CFGKEY_HARMONY_HOME);
        if (!home) {
            hdesc->errstr = "No host or " CFGKEY_HARMONY_HOME " specified";
            return -1;
        }

        /* Fork a local tuning session. */
        path = sprintf_alloc("%s/libexec/" SESSION_CORE_EXECFILE, home);
        hdesc->socket = socket_launch(path, NULL, NULL);
        free(path);
    }
    else {
        hdesc->socket = tcp_connect(host, port);
    }

    if (hdesc->socket < 0) {
        hdesc->errstr = strerror(errno);
        return -1;
    }

    /* Prepare a default client id, if necessary. */
    if (hdesc->id == NULL)
        hdesc->id = default_id(hdesc->socket);
    hdesc->state = HARMONY_STATE_CONNECTED;

    /* Prepare a Harmony message. */
    hmesg_scrub(&hdesc->mesg);
    hdesc->mesg.data.session = HSESSION_INITIALIZER;
    hsession_copy(&hdesc->mesg.data.session, &hdesc->sess);

    return send_request(hdesc, HMESG_SESSION);
}

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
int harmony_id(hdesc_t* hdesc, const char* id)
{
    if (hdesc->id && hdesc->id != default_id_buf)
        free(hdesc->id);

    hdesc->id = stralloc(id);
    if (!hdesc->id) {
        hdesc->errstr = strerror(errno);
        return -1;
    }
    return 0;
}

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
int harmony_bind_int(hdesc_t* hdesc, const char* name, long* ptr)
{
    return varloc_setptr(hdesc, name, ptr);
}

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
int harmony_bind_real(hdesc_t* hdesc, const char* name, double* ptr)
{
    return varloc_setptr(hdesc, name, ptr);
}

/**
 * \brief Bind a local variable of type `char*` to an enumerated
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
 * \param ptr   Pointer to a local `char*` variable that will
 *              hold the current testing value.
 *
 * \return Returns a harmony descriptor on success, and -1 otherwise.
 */
int harmony_bind_enum(hdesc_t* hdesc, const char* name, const char** ptr)
{
    return varloc_setptr(hdesc, name, ptr);
}

/**
 * \brief Join an established Harmony tuning session.
 *
 * Establishes a connection with the Harmony Server on a specific
 * host and port, and joins the named session.  All variables must be
 * bound to local memory via harmony_bind_XXX() for this call to succeed.
 *
 * If `host` is `NULL` or `port` is 0, values from the environment
 * variable `HARMONY_S_HOST` or `HARMONY_S_PORT` will be used,
 * respectively.  If either environment variable is not defined,
 * values from defaults.h will be used as a last resort.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param host  Host of the Harmony server.
 * \param port  Port of the Harmony server.
 * \param name  Name of an existing tuning session on the server.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_join(hdesc_t* hdesc, const char* host, int port, const char* name)
{
    int perf_len;
    char* cfgval;

    /* Verify that we have *at least* one variable bound, and that
     * this descriptor isn't already associated with a tuning
     * session.
     */
    if (hdesc->state >= HARMONY_STATE_READY) {
        hdesc->errstr = "Descriptor already joined with an existing session";
        errno = EINVAL;
        return -1;
    }

    if (hdesc->state == HARMONY_STATE_INIT) {
        if (!host) {
            host = hcfg_get(&hdesc->sess.cfg, CFGKEY_HARMONY_HOST);
            hdesc->errstr = "Invalid value for " CFGKEY_HARMONY_HOST
                            " configuration variable.";
        }

        if (port == 0) {
            port = hcfg_int(&hdesc->sess.cfg, CFGKEY_HARMONY_PORT);
        }

        hdesc->socket = tcp_connect(host, port);
        if (hdesc->socket < 0) {
            hdesc->errstr = "Error establishing TCP connection with server";
            return -1;
        }

        if (hdesc->id == NULL)
            hdesc->id = default_id(hdesc->socket);
        hdesc->state = HARMONY_STATE_CONNECTED;

        hdesc->sess.sig.name = stralloc(name);
        if (!hdesc->sess.sig.name) {
            hdesc->errstr = "Error allocating memory for signature name";
            return -1;
        }
    }

    /* Prepare a Harmony message. */
    hmesg_scrub(&hdesc->mesg);
    hdesc->mesg.data.join = HSIGNATURE_INITIALIZER;
    if (hsignature_copy(&hdesc->mesg.data.join, &hdesc->sess.sig) != 0) {
        hdesc->errstr = "Internal error copying signature data";
        return -1;
    }

    /* send the client registration message */
    if (send_request(hdesc, HMESG_JOIN) != 0)
        return -1;

    if (hdesc->mesg.status != HMESG_STATUS_OK) {
        hdesc->errstr = "Invalid message received";
        errno = EINVAL;
        return -1;
    }

    hsignature_fini(&hdesc->sess.sig);
    if (hsignature_copy(&hdesc->sess.sig, &hdesc->mesg.data.join) != 0) {
        hdesc->errstr = "Error copying received signature structure";
        return -1;
    }

    cfgval = harmony_getcfg(hdesc, CFGKEY_PERF_COUNT);
    if (!cfgval) {
        hdesc->errstr = "Error retrieving performance count.";
        return -1;
    }

    perf_len = atoi(cfgval);
    if (perf_len < 1) {
        hdesc->errstr = "Invalid value for " CFGKEY_PERF_COUNT;
        return -1;
    }

    hdesc->perf = hperf_alloc(perf_len);
    if (!hdesc->perf) {
        hdesc->errstr = "Error allocating performance array.";
        return -1;
    }

    hdesc->state = HARMONY_STATE_READY;
    return 0;
}

/**
 * \brief Leave a Harmony tuning session.
 *
 * End participation in a Harmony tuning session.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_leave(hdesc_t* hdesc)
{
    if (hdesc->state <= HARMONY_STATE_INIT) {
        hdesc->errstr = "Descriptor not currently joined to any session.";
        errno = EINVAL;
        return -1;
    }

    hdesc->state = HARMONY_STATE_INIT;
    if (close(hdesc->socket) != 0 && debug_mode)
        perror("Error closing socket during harmony_leave()");

    /* Reset the hsession_t to prepare for hdesc reuse. */
    hsignature_fini(&hdesc->sess.sig);
    hdesc->best.id = -1;

    return 0;
}

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
 * \brief Return the current value of an integer-domain session variable.
 *
 * Finds an integer-domain tuning variable given its name and returns
 * its current value.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param name  Name of a tuning variable declared with harmony_int().
 *
 * \return Returns the current value of a tuning variable, if the
 *         given name matches an integer-domain session variable.
 *         Otherwise, LONG_MIN is returned and errno is set
 *         appropriately.
 */
long harmony_get_int(hdesc_t* hdesc, const char* name)
{
    int idx = var_index(hdesc, name);

    if (idx >= 0 && hdesc->sess.sig.range[idx].type == HVAL_INT) {
        if (hdesc->varloc[idx].ptr)
            return *(long*)hdesc->varloc[idx].ptr;
        else
            return hdesc->varloc[idx].val.value.i;
    }

    errno = EINVAL;
    return LONG_MIN;
}

/**
 * \brief Return the current value of a real-domain session variable.
 *
 * Finds a real-domain tuning variable given its name and returns its
 * current value.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param name  Name of a tuning variable declared with harmony_real().
 *
 * \return Returns the current value of a tuning variable, if the
 *         given name matches a real-domain session variable.
 *         Otherwise, NAN is returned and errno is set appropriately.
 */
double harmony_get_real(hdesc_t* hdesc, const char* name)
{
    int idx = var_index(hdesc, name);

    if (idx >= 0 && hdesc->sess.sig.range[idx].type == HVAL_REAL) {
        if (hdesc->varloc[idx].ptr)
            return *(double*)hdesc->varloc[idx].ptr;
        else
            return hdesc->varloc[idx].val.value.r;
    }

    errno = EINVAL;
    return NAN;
}

/**
 * \brief Return the current value of an enumerated-domain session variable.
 *
 * Finds an enumerated-domain tuning variable given its name and
 * returns its current value.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param name  Name of a tuning variable declared with harmony_enum().
 *
 * \return Returns the current value of a tuning variable, if the
 *         given name matches an enumerated-domain session variable.
 *         Otherwise, NULL is returned and errno is set appropriately.
 */
const char* harmony_get_enum(hdesc_t* hdesc, const char* name)
{
    int idx = var_index(hdesc, name);

    if (idx >= 0 && hdesc->sess.sig.range[idx].type == HVAL_STR) {
        if (hdesc->varloc[idx].ptr)
            return *(char**)hdesc->varloc[idx].ptr;
        else
            return hdesc->varloc[idx].val.value.s;
    }

    errno = EINVAL;
    return NULL;
}

/**
 * \brief Get a key value from the session's configuration.
 *
 * Searches the session's configuration system for key, and returns
 * the string value associated with it if found.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param key   Config key to search for on the session.
 *
 * \return Returns a c-style string on success, and `NULL` otherwise.
 */
char* harmony_getcfg(hdesc_t* hdesc, const char* key)
{
    if (hdesc->state < HARMONY_STATE_CONNECTED) {
        hdesc->errstr = "Descriptor not currently joined to any session.";
        errno = EINVAL;
        return NULL;
    }

    /* Prepare a Harmony message. */
    hmesg_scrub(&hdesc->mesg);
    hdesc->mesg.data.string = key;

    if (send_request(hdesc, HMESG_GETCFG) != 0)
        return NULL;

    if (hdesc->mesg.status != HMESG_STATUS_OK) {
        hdesc->errstr = "Invalid message received from server.";
        errno = EINVAL;
        return NULL;
    }

    snprintf_grow(&hdesc->buf, &hdesc->buflen, "%s", hdesc->mesg.data.string);
    return hdesc->buf;
}

/**
 * \brief Set a new key/value pair in the session's configuration.
 *
 * Writes the new key/value pair into the session's run-time
 * configuration database.  If the key exists in the database, its
 * value is overwritten.  If val is `NULL`, the key will be erased
 * from the database.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 * \param key   Config key to modify in the session.
 * \param val   Config value to associate with the key.
 *
 * \return Returns the original key value on success.  If the key did not
 *         exist prior to this call, an empty string ("") is returned.
 *         Otherwise, `NULL` is returned on error.
 */
char* harmony_setcfg(hdesc_t* hdesc, const char* key, const char* val)
{
    char* buf;
    int retval;

    if (hdesc->state < HARMONY_STATE_CONNECTED) {
        /* User must be preparing for a new session since we're not
         * connected yet.  Store the key/value pair in a local cache.
         */
        const char* cfgval = hcfg_get(&hdesc->sess.cfg, key);
        if (!cfgval) cfgval = "";
        snprintf_grow(&hdesc->buf, &hdesc->buflen, "%s", cfgval);

        if (hcfg_set(&hdesc->sess.cfg, key, val) != 0) {
            hdesc->errstr = "Error setting local environment variable.";
            return NULL;
        }
        return hdesc->buf;
    }

    if (!key) {
        hdesc->errstr = "Invalid key string.";
        errno = EINVAL;
        return NULL;
    }

    buf = sprintf_alloc("%s=%s", key, val ? val : "");
    if (!buf) {
        hdesc->errstr = "Internal memory allocation error.";
        return NULL;
    }

    /* Prepare a Harmony message. */
    hmesg_scrub(&hdesc->mesg);
    hdesc->mesg.data.string = buf;

    retval = send_request(hdesc, HMESG_SETCFG);
    free(buf);

    if (retval != 0)
        return NULL;

    if (hdesc->mesg.status != HMESG_STATUS_OK) {
        hdesc->errstr = "Invalid message received from server.";
        errno = EINVAL;
        return NULL;
    }

    snprintf_grow(&hdesc->buf, &hdesc->buflen, "%s", hdesc->mesg.data.string);
    return hdesc->buf;
}

/**
 * \brief Fetch a new configuration from the Harmony session.
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
int harmony_fetch(hdesc_t* hdesc)
{
    int i;

    if (hdesc->state < HARMONY_STATE_CONNECTED) {
        hdesc->errstr = "Descriptor not currently joined to any session.";
        errno = EINVAL;
        return -1;
    }

    /* Prepare a Harmony message. */
    hmesg_scrub(&hdesc->mesg);

    if (send_request(hdesc, HMESG_FETCH) != 0)
        return -1;

    if (hdesc->mesg.status == HMESG_STATUS_BUSY) {
        if (update_best(hdesc, &hdesc->mesg.data.point) != 0)
            return -1;

        if (hdesc->best.id == -1) {
            /* No best point is available. Inform the user by returning 0. */
            return 0;
        }

        /* Set current point to best point. */
        if (hpoint_copy(&hdesc->curr, &hdesc->best) != 0) {
            hdesc->errstr = "Internal error copying point data.";
            errno = EINVAL;
            return -1;
        }
    }
    else if (hdesc->mesg.status == HMESG_STATUS_OK) {
        if (hpoint_copy(&hdesc->curr, &hdesc->mesg.data.point) != 0) {
            hdesc->errstr = "Internal error copying point data.";
            errno = EINVAL;
            return -1;
        }
    }
    else {
        hdesc->errstr = "Invalid message received from server.";
        errno = EINVAL;
        return -1;
    }

    /* Update the variables from the content of the message. */
    if (set_values(hdesc, &hdesc->curr) != 0)
        return -1;

    /* Initialize our internal performance array. */
    for (i = 0; i < hdesc->perf->n; ++i)
        hdesc->perf->p[i] = NAN;

    /* Client variables were changed.  Inform the user by returning 1. */
    hdesc->state = HARMONY_STATE_TESTING;
    return 1;
}

/**
 * \brief Report the performance of a configuration to the Harmony session.
 *
 * Sends a report of the tested performance to the Harmony session.
 * The report will be analyzed by the session to produce values for
 * the next set of calls to harmony_fetch().
 *
 * The `value` parameter should point to a floating-point double.  For
 * multi-objective tuning, where multiple performance values are
 * required for a single test, `value` should point to an array of
 * floating-point doubles.  Finally, `value` may be NULL, provided
 * that all performance values have already been set via
 * harmony_single_perf().
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
 * \param perf  Performance vector for the current configuration.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int harmony_report(hdesc_t* hdesc, double* perf)
{
    int i;

    if (hdesc->state < HARMONY_STATE_CONNECTED) {
        hdesc->errstr = "Descriptor not currently joined to any session.";
        errno = EINVAL;
        return -1;
    }

    if (hdesc->state < HARMONY_STATE_TESTING)
        return 0;

    if (perf)
        memcpy(hdesc->perf->p, perf, sizeof(double) * hdesc->perf->n);

    for (i = 0; i < hdesc->perf->n; ++i) {
        if (isnan(hdesc->perf->p[i])) {
            hdesc->errstr = "Error: Performance report incomplete.";
            errno = EINVAL;
            return -1;
        }
    }

    /* Prepare a Harmony message. */
    hmesg_scrub(&hdesc->mesg);
    hdesc->mesg.data.report.cand_id = hdesc->curr.id;
    hdesc->mesg.data.report.perf = hperf_clone(hdesc->perf);
    if (!hdesc->mesg.data.report.perf) {
        hdesc->errstr = "Error allocating performance array for message.";
        return -1;
    }

    if (send_request(hdesc, HMESG_REPORT) != 0)
        return -1;

    if (hdesc->mesg.status != HMESG_STATUS_OK) {
        hdesc->errstr = "Invalid message received from server.";
        errno = EINVAL;
        return -1;
    }

    hdesc->state = HARMONY_STATE_READY;
    return 0;
}

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
int harmony_report_one(hdesc_t* hdesc, int index, double value)
{
    if (hdesc->state < HARMONY_STATE_CONNECTED) {
        hdesc->errstr = "Descriptor not currently joined to any session.";
        errno = EINVAL;
        return -1;
    }

    if (index != 0 || index >= hdesc->perf->n) {
        hdesc->errstr = "Invalid performance index.";
        errno = EINVAL;
        return -1;
    }

    if (hdesc->state == HARMONY_STATE_TESTING)
        hdesc->perf->p[index] = value;

    return 0;
}

/**
 * \brief Sets variables under Harmony's control to the best known
 *        configuration.
 *
 * If the client is currently connected, the best known configuration
 * is retrieved from the session.  Otherwise, the a local copy of the
 * best known point is used.
 *
 * If no best configuration exists (e.g. before any configurations
 * have been evaluated), this function will return an error.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 *
 * \return Returns 1 if a new best point was retrieved from the
 *         session, 0 if a local copy was used, and -1 otherwise.
 */
int harmony_best(hdesc_t* hdesc)
{
    int retval = 0;

    if (hdesc->state >= HARMONY_STATE_CONNECTED) {
        /* Prepare a Harmony message. */
        hmesg_scrub(&hdesc->mesg);

        if (send_request(hdesc, HMESG_BEST) != 0)
            return -1;

        if (hdesc->mesg.status != HMESG_STATUS_OK) {
            hdesc->errstr = "Invalid message received from server.";
            errno = EINVAL;
            return -1;
        }

        if (hdesc->best.id < hdesc->mesg.data.point.id) {
            if (update_best(hdesc, &hdesc->mesg.data.point) != 0)
                return -1;
            retval = 1;
        }
    }

    /* Make sure our best known point is valid. */
    if (hdesc->best.id < 0) {
        errno = EINVAL;
        return -1;
    }

    if (set_values(hdesc, &hdesc->best) != 0)
        return -1;

    return retval;
}

/**
 * \brief Retrieve the convergence state of the current search.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 *
 * \return Returns 1 if the search has converged, 0 if it has not,
 *         and -1 on error.
 */
int harmony_converged(hdesc_t* hdesc)
{
    int retval;
    char* str;

    retval = 0;
    str = harmony_getcfg(hdesc, CFGKEY_CONVERGED);
    if (str && str[0] == '1') {
        hdesc->state = HARMONY_STATE_CONVERGED;
        retval = 1;
    }
    return retval;
}

/**
 * \brief Access the current Harmony error string.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 *
 * \return Returns a pointer to a string that describes the latest
 *         Harmony error, or `NULL` if no error has occurred since
 *         the last call to harmony_error_clear().
 */
const char* harmony_error_string(hdesc_t* hdesc)
{
    return hdesc->errstr;
}

/**
 * \brief Clears the error status of the given Harmony descriptor.
 *
 * \param hdesc Harmony descriptor returned from harmony_init().
 */
void harmony_error_clear(hdesc_t* hdesc)
{
    hdesc->errstr = NULL;
}

/**
 * @}
 */

/* ----------------------------------------
 * Private helper function implementations.
 */
char* default_id(int n)
{
    gethostname(default_id_buf, MAX_HOSTNAME_STRLEN);
    default_id_buf[MAX_HOSTNAME_STRLEN] = '\0';
    sprintf(default_id_buf + strlen(default_id_buf), "_%d_%d", getpid(), n);

    return default_id_buf;
}

int send_request(hdesc_t* hdesc, hmesg_type msg_type)
{
    hdesc->mesg.type = msg_type;
    hdesc->mesg.status = HMESG_STATUS_REQ;
    hdesc->mesg.src_id = hdesc->id;

    if (mesg_send(hdesc->socket, &hdesc->mesg) < 1) {
        hdesc->errstr = "Error sending Harmony message to server.";
        errno = EIO;
        return -1;
    }

    do {
        if (mesg_recv(hdesc->socket, &hdesc->mesg) < 1) {
            hdesc->errstr = "Error retrieving Harmony message from server.";
            errno = EIO;
            return -1;
        }

        /* If the httpinfo layer is enabled during a stand-alone session,
         * it will generate extraneous messages where dest == -1.
         * Make sure to ignore these messages.
         */
    } while (hdesc->mesg.dest == -1);

    if (hdesc->mesg.type != msg_type) {
        hdesc->mesg.status = HMESG_STATUS_FAIL;
        hdesc->errstr = "Internal error: Server response message mismatch.";
        return -1;
    }

    if (hdesc->mesg.status == HMESG_STATUS_FAIL) {
        hdesc->errstr = hdesc->mesg.data.string;
        return -1;
    }
    return 0;
}

int set_values(hdesc_t* hdesc, const hpoint_t* pt)
{
    int i;

    if (hdesc->sess.sig.range_len != pt->n) {
        hdesc->errstr = "Invalid internal point structure.";
        errno = EINVAL;
        return -1;
    }

    for (i = 0; i < pt->n; ++i) {
        if (hdesc->varloc[i].ptr == NULL) {
            hdesc->varloc[i].val = pt->val[i];
        }
        else {
            switch (pt->val[i].type) {
            case HVAL_INT:
                *(long*)hdesc->varloc[i].ptr = pt->val[i].value.i;
                break;
            case HVAL_REAL:
                *(double*)hdesc->varloc[i].ptr = pt->val[i].value.r;
                break;
            case HVAL_STR:
                *(const char**)hdesc->varloc[i].ptr = pt->val[i].value.s;
                break;
            default:
                hdesc->errstr = "Invalid signature value type.";
                errno = EINVAL;
                return -1;
            }
        }
    }
    return 0;
}

int varloc_setptr(hdesc_t* hdesc, const char* name, void* ptr)
{
    int idx = var_index(hdesc, name);
    if (idx < 0) {
        errno = EINVAL;
        return -1;
    }

    varloc_extend(hdesc);
    hdesc->varloc[idx].ptr = ptr;
    return 0;
}

int varloc_extend(hdesc_t* hdesc)
{
    if (hdesc->varloc_cap < hdesc->sess.sig.range_cap) {
        int i;
        hvarloc_t* tmp = realloc(hdesc->varloc,
                                 hdesc->sess.sig.range_cap * sizeof(*tmp));
        if (!tmp)
            return -1;

        for (i = hdesc->varloc_cap; i < hdesc->sess.sig.range_cap; ++i)
            tmp[i].ptr = NULL;

        hdesc->varloc = tmp;
        hdesc->varloc_cap = i;
    }
    return 0;
}

int var_index(hdesc_t* hdesc, const char* name)
{
    int idx;

    for (idx = 0; idx < hdesc->sess.sig.range_len; ++idx) {
        if (strcmp(hdesc->sess.sig.range[idx].name, name) == 0)
            return idx;
    }
    return -1;
}

int update_best(hdesc_t* hdesc, const hpoint_t* pt)
{
    if (hdesc->best.id >= pt->id)
        return 0;

    if (hpoint_copy(&hdesc->best, pt) != 0) {
        hdesc->errstr = "Internal error copying point data.";
        errno = EINVAL;
        return -1;
    }
    return 0;
}
