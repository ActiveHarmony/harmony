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
#include "hsig.h"
#include "hcfg.h"
#include "hutil.h"
#include "hmesg.h"
#include "hpoint.h"
#include "hperf.h"
#include "hsockutil.h"

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

    HARMONY_STATE_MAX
} harmony_state_t;

typedef struct hvarloc_t {
    void* ptr;
    hval_t val;
} hvarloc_t;

struct hdesc_t {
    harmony_state_t state;
    int socket;
    char* id;
    char default_id[HOST_NAME_MAX + 32];

    hsig_t sig;
    hcfg_t cfg;
    hmesg_t mesg;

    hvarloc_t* varloc;
    int varloc_cap;

    hperf_t perf;
    hpoint_t curr, best;

    char* buf;
    int buflen;
    const char* errstr;
};

/* ---------------------------------------------------
 * Forward declarations for internal helper functions.
 */
void generate_id(hdesc_t* hd);
int  extend_perf(hdesc_t* hd);
int  extend_varloc(hdesc_t* hd);
int  find_var(hdesc_t* hd, const char* name);
int  send_request(hdesc_t* hd, hmesg_type msg_type);
int  set_varloc(hdesc_t* hd, const char* name, void* ptr);
int  write_values(hdesc_t* hd, const hpoint_t* pt);

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
 * ah_fini() when it is no longer needed.
 *
 * \return Returns Harmony descriptor upon success, and `NULL` otherwise.
 */
hdesc_t* ah_init()
{
    hdesc_t* hd = calloc(1, sizeof(*hd));
    if (!hd)
        return NULL;

    if (hcfg_init(&hd->cfg) != 0)
        return NULL;

    hd->state = HARMONY_STATE_INIT;
    return hd;
}

/**
 * \brief Find configuration directives in the command-line arguments.
 *
 * This function iterates through the command-line arguments (as
 * passed to the main() function) to search for Harmony configuration
 * directives in the form NAME=VAL.  Any arguments matching this
 * pattern are added to the configuration and removed from `argv`.
 *
 * Iteration through the command-line arguments stops prematurely if a
 * double-dash ("--") token is found.  All non-directive arguments are
 * shifted to the front of `argv`.  Finally, `argc` is updated to
 * reflect the remaining number of program arguments.
 *
 * \param hd   Harmony descriptor returned from ah_init().
 * \param argc Address of the `argc` variable.
 * \param argv Address of the command-line argument array
 *             (i.e., the value of `argv`).
 *
 * \return Returns the number of directives successfully taken from
 *         `argv`, or -1 on error.
 */
int ah_args(hdesc_t* hd, int* argc, char** argv)
{
    int skip = 0;
    int tail = 0;

    for (int i = 0; i < *argc; ++i) {
        if (strcmp(argv[i], "--") == 0)
            skip = 1;

        if (skip || hcfg_parse(&hd->cfg, argv[i], &hd->errstr) != 1)
            argv[tail++] = argv[i];
    }

    int removed = *argc - tail;
    *argc = tail;
    return removed;
}

/**
 * \brief Release all resources associated with a Harmony client descriptor.
 *
 * \param hd Harmony descriptor returned from ah_init().
 */
void ah_fini(hdesc_t* hd)
{
    if (hd) {
        if (hd->state >= HARMONY_STATE_CONNECTED) {
            if (close(hd->socket) != 0 && debug_mode)
                perror("Error closing socket during ah_leave()");
        }

        hmesg_fini(&hd->mesg);
        hcfg_fini(&hd->cfg);
        hsig_fini(&hd->sig);
        hpoint_fini(&hd->curr);
        hpoint_fini(&hd->best);
        hperf_fini(&hd->perf);

        if (hd->id && hd->id != hd->default_id)
            free(hd->id);

        free(hd->buf);
        free(hd->varloc);
        free(hd);
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
 * \brief Load a tuning session description from a file.
 *
 * Opens and parses a file for configuration and tuning variable
 * declarations.  The session name is copied from the filename.  If
 * `filename` is the string "-", `stdin` will be used instead.
 * To load a file named "-", include the path (e.g., "./-").
 *
 * The file is parsed line by line.  Each line may contain either a
 * tuning variable or configuration variable definitions.  The line is
 * considered a tuning variable declaration if it begins with a valid
 * tuning variable type.
 *
 * Here are some examples of valid integer-domain variables:
 *
 *     int first_ivar = min:-10 max:10 step:2
 *     int second_ivar = min:1 max:100 # Integers have a default step of 1.
 *
 * Here are some examples of valid real-domain variables:
 *
 *     real first_rvar = min:0 max:1 step:1e-4
 *     real second_rvar = min:-0.5 max:0.5 step:0.001
 *
 * Here are some examples of valid enumerated-domain variables:
 *
 *     enum sort_algo   = bubble, insertion, quick, merge
 *     enum search_algo = forward backward binary # Commas are optional.
 *     enum quotes      = "Cogito ergo sum" \
 *                        "To be, or not to be." \
 *                        "What's up, doc?"
 *
 * Otherwise, the line is considered to be a configuration variable
 * definition, like the following examples:
 *
 *     STRATEGY=pro.so
 *     INIT_POINT = 1.41421, \
 *                  2.71828, \
 *                  3.14159
 *     LAYERS=agg.so:log.so
 *
 *     AGG_TIMES=5
 *     AGG_FUNC=median
 *
 *     LOG_FILE=/tmp/search.log
 *     LOG_MODE=w
 *
 * Variable names must conform to C identifier rules.  Comments are
 * preceded by the hash (#) character, and long lines may be joined
 * with a backslash (\) character.
 *
 * Parsing stops immediately upon error and ah_error_string() may be
 * used to determine the nature of the error.
 *
 * \param hd       Harmony descriptor returned from ah_init().
 * \param filename Name to associate with this variable.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int ah_load(hdesc_t* hd, const char* filename)
{
    FILE* fp;
    int retval = 0;

    if (hsig_name(&hd->sig, filename) != 0) {
        hd->errstr = "Could not copy session name from filename";
        return -1;
    }

    if (strcmp(filename, "-") == 0) {
        fp = stdin;
    }
    else {
        fp = fopen(filename, "r");
        if (!fp) {
            snprintf_grow(&hd->buf, &hd->buflen,
                          "Could not open '%s' for reading", filename);
            hd->errstr = hd->buf;
            return -1;
        }
    }

    char* buf = NULL;
    int   cap = 0;
    char* end = NULL;
    int   linenum = 1;
    while (1) {
        char* line;
        int   count = file_read_line(fp, &buf, &cap, &line, &end, &hd->errstr);
        if (count <  0) goto error;
        if (count == 0) break; // Loop exit.

        if ((strncmp(line, "int",  3) == 0 && isspace(line[3])) ||
            (strncmp(line, "real", 4) == 0 && isspace(line[4])) ||
            (strncmp(line, "enum", 4) == 0 && isspace(line[4])))
        {
            if (hsig_parse(&hd->sig, line, &hd->errstr) == -1)
                goto error;
        }
        else {
            if (hcfg_parse(&hd->cfg, line, &hd->errstr) == -1)
                goto error;
        }
        linenum += count;
    }
    goto cleanup;

  error:
    snprintf_grow(&hd->buf, &hd->buflen, "Parse error at %s:%d: %s",
                  filename, linenum, hd->errstr);
    hd->errstr = hd->buf;
    retval = -1;

  cleanup:
    fclose(fp);
    free(buf);
    return retval;
}

/**
 * \brief Add an integer-domain variable to the Harmony session.
 *
 * \param hd   Harmony descriptor returned from ah_init().
 * \param name Name to associate with this variable.
 * \param min  Minimum range value (inclusive).
 * \param max  Maximum range value (inclusive).
 * \param step Minimum search increment.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int ah_int(hdesc_t* hd, const char* name, long min, long max, long step)
{
    return hsig_int(&hd->sig, name, min, max, step);
}

/**
 * \brief Add a real-domain variable to the Harmony session.
 *
 * \param hd   Harmony descriptor returned from ah_init().
 * \param name Name to associate with this variable.
 * \param min  Minimum range value (inclusive).
 * \param max  Maximum range value (inclusive).
 * \param step Minimum search increment.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int ah_real(hdesc_t* hd, const char* name, double min, double max, double step)
{
    return hsig_real(&hd->sig, name, min, max, step);
}

/**
 * \brief Add an enumeration variable and append to the list of valid values
 *        in this set.
 *
 * \param hd    Harmony descriptor returned from ah_init().
 * \param name  Name to associate with this variable.
 * \param value String that belongs in this enumeration.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int ah_enum(hdesc_t* hd, const char* name, const char* value)
{
    return hsig_enum(&hd->sig, name, value);
}

/**
 * \brief Specify the search strategy to use in the new Harmony session.
 *
 * \param hd       Harmony descriptor returned from ah_init().
 * \param strategy Filename of the strategy plug-in to use in this session.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int ah_strategy(hdesc_t* hd, const char* strategy)
{
    return hcfg_set(&hd->cfg, CFGKEY_STRATEGY, strategy);
}

/**
 * \brief Specify the list of plug-ins to use in the new Harmony session.
 *
 * Plug-in layers are specified via a single string of filenames,
 * separated by the colon character (`:`).  The layers are loaded in
 * list order, with each successive layer placed further from the
 * search strategy in the center.
 *
 * \param hd   Harmony descriptor returned from ah_init().
 * \param list List of plug-ins to load with this session.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int ah_layers(hdesc_t* hd, const char* list)
{
    return hcfg_set(&hd->cfg, CFGKEY_LAYERS, list);
}

/**
 * \brief Instantiate a new Harmony tuning session.
 *
 * After using the session establishment routines (ah_int(),
 * ah_strategy(), etc.) to specify a tuning session, this function
 * launches a new tuning session.  It may either be started locally or
 * on the Harmony Server located at *host*:*port*.
 *
 * If *host* is `NULL`, its value will be taken from the environment
 * variable `HARMONY_HOST`.  If `HARMONY_HOST` is not defined, the
 * environment variable `HARMONY_HOME` will be used to initiate a
 * private tuning session, which will be available only to the local
 * process.
 *
 * If *port* is 0, its value will be taken from the environment
 * variable `HARMONY_PORT`, if defined.  Otherwise, its value will be
 * taken from the src/defaults.h file.
 *
 * \param hd   Harmony descriptor returned from ah_init().
 * \param host Host of the Harmony server (or `NULL`).
 * \param port Port of the Harmony server.
 * \param name Name to associate with this session instance.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int ah_launch(hdesc_t* hd, const char* host, int port, const char* name)
{
    /* Sanity check input */
    if (hd->state > HARMONY_STATE_INIT) {
        hd->errstr = "Descriptor already connected to another session";
        errno = EINVAL;
        return -1;
    }

    if (hd->sig.range_len < 1) {
        hd->errstr = "No tuning variables defined";
        errno = EINVAL;
        return -1;
    }

    if (extend_varloc(hd) != 0)
        return -1;

    if (extend_perf(hd) != 0)
        return -1;

    if (!host)
        host = hcfg_get(&hd->cfg, CFGKEY_HARMONY_HOST);

    if (port == 0)
        port = hcfg_int(&hd->cfg, CFGKEY_HARMONY_PORT);

    if (!host) {
        char* path;
        const char* home;

        /* Find the Active Harmony installation. */
        home = hcfg_get(&hd->cfg, CFGKEY_HARMONY_HOME);
        if (!home) {
            hd->errstr = "No host or " CFGKEY_HARMONY_HOME " specified";
            errno = EINVAL;
            return -1;
        }

        /* Fork a local tuning session. */
        path = sprintf_alloc("%s/libexec/" SESSION_CORE_EXECFILE, home);
        hd->socket = socket_launch(path, NULL, NULL);
        free(path);
    }
    else {
        hd->socket = tcp_connect(host, port);
    }

    if (hd->socket < 0) {
        hd->errstr = strerror(errno);
        return -1;
    }
    generate_id(hd);

    hd->state = HARMONY_STATE_CONNECTED;

    /* Provide a default name, if necessary. */
    if (!name && !hd->sig.name)
        name = hd->default_id;

    if (name && hsig_name(&hd->sig, name) != 0) {
        hd->errstr = "Error setting session name";
        return -1;
    }

    /* Prepare a default client id, if necessary. */
    if (hd->id == NULL)
        hd->id = hd->default_id;

    /* Prepare a Harmony message. */
    hsig_copy(&hd->mesg.state.sig, &hd->sig);
    hcfg_copy(&hd->mesg.data.cfg, &hd->cfg);

    if (send_request(hd, HMESG_SESSION) != 0)
        return -1;

    hd->state = HARMONY_STATE_READY;

    return 0;
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
 * \brief Join an established Harmony tuning session.
 *
 * Establishes a connection with the Harmony Server on a specific host
 * and port, and joins the named session.
 *
 * If *host* is `NULL`, its value will be taken from the environment
 * variable `HARMONY_HOST`.  Either *host* or `HARMONY_HOST` must be
 * defined for this function to succeed.
 *
 * If *port* is 0, its value will be taken from the environment
 * variable `HARMONY_PORT`, if defined.  Otherwise, its value will be
 * taken from the src/defaults.h file.
 *
 * Upon success, tuning variable information is retrieved from the
 * server and may subsequently be bound to local memory via
 * ah_bind_XXX().
 *
 * \param hd   Harmony descriptor returned from ah_init().
 * \param host Host of the Harmony server.
 * \param port Port of the Harmony server.
 * \param name Name of an existing tuning session on the server.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int ah_join(hdesc_t* hd, const char* host, int port, const char* name)
{
    if (hd->state < HARMONY_STATE_CONNECTED) {
        if (!host)
            host = hcfg_get(&hd->cfg, CFGKEY_HARMONY_HOST);

        if (port == 0)
            port = hcfg_int(&hd->cfg, CFGKEY_HARMONY_PORT);

        if (!host) {
            hd->errstr = "No host specified";
            errno = EINVAL;
            return -1;
        }

        hd->socket = tcp_connect(host, port);
        if (hd->socket < 0) {
            hd->errstr = "Error establishing TCP connection with server";
            return -1;
        }
        generate_id(hd);

        hd->state = HARMONY_STATE_CONNECTED;
    }

    if (hd->state < HARMONY_STATE_READY) {
        if (!name) {
            hd->errstr = "Invalid session name";
            errno = EINVAL;
            return -1;
        }

        if (hsig_name(&hd->sig, name) != 0) {
            hd->errstr = "Error setting session name";
            return -1;
        }

        if (hd->id == NULL)
            hd->id = hd->default_id;

        /* Prepare a Harmony message. */
        hd->mesg.data.string = name;

        /* send the client registration message */
        if (send_request(hd, HMESG_JOIN) != 0)
            return -1;

        if (hd->mesg.status != HMESG_STATUS_OK) {
            hd->errstr = "Invalid message received";
            errno = EINVAL;
            return -1;
        }

        if (hsig_copy(&hd->sig, &hd->mesg.state.sig) != 0) {
            hd->errstr = "Error copying received signature structure";
            return -1;
        }

        if (extend_varloc(hd) != 0)
            return -1;

        if (extend_perf(hd) != 0)
            return -1;

        hd->state = HARMONY_STATE_READY;
    }
    else {
        // Descriptor was already joined.  Make sure the session names match.
        if (name && strcmp(hd->sig.name, name) != 0) {
            hd->errstr = "Descriptor already joined with an existing session";
            errno = EINVAL;
            return -1;
        }
    }

    return 0;
}

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
 * \param hd Harmony descriptor returned from ah_init().
 * \param id Unique identification string.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int ah_id(hdesc_t* hd, const char* id)
{
    if (hd->id && hd->id != hd->default_id)
        free(hd->id);

    hd->id = stralloc(id);
    if (!hd->id) {
        hd->errstr = strerror(errno);
        return -1;
    }
    return 0;
}

/**
 * \brief Bind a local variable of type `long` to an integer-domain
 *        session variable.
 *
 * This function associates a local variable with a session variable
 * declared using ah_int().  Upon ah_fetch(), the value
 * chosen by the session will be stored at the address `ptr`.
 *
 * This function must be called for each integer-domain variable
 * defined in the joining session.  Otherwise ah_join() will fail
 * when called.
 *
 * \param hd   Harmony descriptor returned from ah_init().
 * \param name Session variable defined using ah_int().
 * \param ptr   Pointer to a local `long` variable that will
 *              hold the current testing value.
 *
 * \return Returns a harmony descriptor on success, and -1 otherwise.
 */
int ah_bind_int(hdesc_t* hd, const char* name, long* ptr)
{
    return set_varloc(hd, name, ptr);
}

/**
 * \brief Bind a local variable of type `double` to a real-domain
 *        session variable.
 *
 * This function associates a local variable with a session variable
 * declared using ah_real().  Upon ah_fetch(), the value
 * chosen by the session will be stored at the address `ptr`.
 *
 * This function must be called for each real-domain variable defined
 * in the joining session.  Otherwise ah_join() will fail when
 * called.
 *
 * \param hd   Harmony descriptor returned from ah_init().
 * \param name Session variable defined using ah_real().
 * \param ptr  Pointer to a local `double` variable that will
 *             hold the current testing value.
 *
 * \return Returns a harmony descriptor on success, and -1 otherwise.
 */
int ah_bind_real(hdesc_t* hd, const char* name, double* ptr)
{
    return set_varloc(hd, name, ptr);
}

/**
 * \brief Bind a local variable of type `char*` to an enumerated
 *        string-based session variable.
 *
 * This function associates a local variable with a session variable
 * declared using ah_enum().  Upon ah_fetch(), the value
 * chosen by the session will be stored at the address `ptr`.
 *
 * This function must be called for each string-based variable defined
 * in the joining session.  Otherwise ah_join() will fail when
 * called.
 *
 * \param hd   Harmony descriptor returned from ah_init().
 * \param name Session variable defined using ah_enum().
 * \param ptr  Pointer to a local `char*` variable that will
 *             hold the current testing value.
 *
 * \return Returns a harmony descriptor on success, and -1 otherwise.
 */
int ah_bind_enum(hdesc_t* hd, const char* name, const char** ptr)
{
    return set_varloc(hd, name, ptr);
}

/**
 * \brief Leave a Harmony tuning session.
 *
 * End participation in a Harmony tuning session.
 *
 * \param hd Harmony descriptor returned from ah_init().
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int ah_leave(hdesc_t* hd)
{
    if (hd->state <= HARMONY_STATE_INIT) {
        hd->errstr = "Descriptor not currently joined to any session";
        errno = EINVAL;
        return -1;
    }

    hd->state = HARMONY_STATE_INIT;
    if (close(hd->socket) != 0 && debug_mode)
        perror("Error closing socket during ah_leave()");

    /* Reset the hsession_t to prepare for harmony descriptor reuse. */
    hsig_scrub(&hd->sig);
    hd->best.id = 0;

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
 * \param hd   Harmony descriptor returned from ah_init().
 * \param name Name of a tuning variable declared with ah_int().
 *
 * \return Returns the current value of a tuning variable, if the
 *         given name matches an integer-domain session variable.
 *         Otherwise, LONG_MIN is returned and errno is set
 *         appropriately.
 */
long ah_get_int(hdesc_t* hd, const char* name)
{
    int idx = find_var(hd, name);

    if (idx >= 0 && hd->sig.range[idx].type == HVAL_INT) {
        if (hd->varloc[idx].ptr)
            return *(long*)hd->varloc[idx].ptr;
        else
            return hd->varloc[idx].val.value.i;
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
 * \param hd   Harmony descriptor returned from ah_init().
 * \param name Name of a tuning variable declared with ah_real().
 *
 * \return Returns the current value of a tuning variable, if the
 *         given name matches a real-domain session variable.
 *         Otherwise, NAN is returned and errno is set appropriately.
 */
double ah_get_real(hdesc_t* hd, const char* name)
{
    int idx = find_var(hd, name);

    if (idx >= 0 && hd->sig.range[idx].type == HVAL_REAL) {
        if (hd->varloc[idx].ptr)
            return *(double*)hd->varloc[idx].ptr;
        else
            return hd->varloc[idx].val.value.r;
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
 * \param hd   Harmony descriptor returned from ah_init().
 * \param name Name of a tuning variable declared with ah_enum().
 *
 * \return Returns the current value of a tuning variable, if the
 *         given name matches an enumerated-domain session variable.
 *         Otherwise, NULL is returned and errno is set appropriately.
 */
const char* ah_get_enum(hdesc_t* hd, const char* name)
{
    int idx = find_var(hd, name);

    if (idx >= 0 && hd->sig.range[idx].type == HVAL_STR) {
        if (hd->varloc[idx].ptr)
            return *(char**)hd->varloc[idx].ptr;
        else
            return hd->varloc[idx].val.value.s;
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
 * \param hd  Harmony descriptor returned from ah_init().
 * \param key Config key to search for on the session.
 *
 * \return Returns a c-style string on success, and `NULL` otherwise.
 */
char* ah_get_cfg(hdesc_t* hd, const char* key)
{
    if (!key) {
        hd->errstr = "Invalid key string";
        errno = EINVAL;
        return NULL;
    }

    if (hd->state < HARMONY_STATE_CONNECTED) {
        // Use the local CFG environment.
        return hcfg_get(&hd->cfg, key);
    }

    /* Prepare a Harmony message. */
    hd->mesg.data.string = key;

    if (send_request(hd, HMESG_GETCFG) != 0)
        return NULL;

    if (hd->mesg.status != HMESG_STATUS_OK) {
        hd->errstr = "Invalid message received from server";
        errno = EINVAL;
        return NULL;
    }

    snprintf_grow(&hd->buf, &hd->buflen, "%s", hd->mesg.data.string);
    if (hcfg_set(&hd->cfg, key, hd->buf) != 0) {
        hd->errstr = "Error setting local CFG cache";
        return NULL;
    }
    return hd->buf;
}

/**
 * \brief Set a new key/value pair in the session's configuration.
 *
 * Writes the new key/value pair into the session's run-time
 * configuration database.  If the key exists in the database, its
 * value is overwritten.  If val is `NULL`, the key will be erased
 * from the database.
 *
 * \param hd  Harmony descriptor returned from ah_init().
 * \param key Config key to modify in the session.
 * \param val Config value to associate with the key.
 *
 * \return Returns the original key value on success.  If the key did not
 *         exist prior to this call, an empty string ("") is returned.
 *         Otherwise, `NULL` is returned on error.
 */
char* ah_set_cfg(hdesc_t* hd, const char* key, const char* val)
{
    if (!key) {
        hd->errstr = "Invalid key string";
        errno = EINVAL;
        return NULL;
    }

    if (hd->state >= HARMONY_STATE_READY) {
        // Query the remote CFG environment.
        char* buf = sprintf_alloc("%s=%s", key, val ? val : "");
        int retval;

        buf = sprintf_alloc("%s=%s", key, val ? val : "");
        if (!buf) {
            hd->errstr = "Internal memory allocation error";
            return NULL;
        }

        /* Prepare a Harmony message. */
        hd->mesg.data.string = buf;

        retval = send_request(hd, HMESG_SETCFG);
        free(buf);

        if (retval != 0)
            return NULL;

        if (hd->mesg.status != HMESG_STATUS_OK) {
            hd->errstr = "Invalid message received from server";
            return NULL;
        }
        snprintf_grow(&hd->buf, &hd->buflen, "%s", hd->mesg.data.string);
    }
    else {
        // Use the local CFG environment.
        const char* cfgval = hcfg_get(&hd->cfg, key);
        if (!cfgval) cfgval = "";
        snprintf_grow(&hd->buf, &hd->buflen, "%s", cfgval);
    }

    if (hcfg_set(&hd->cfg, key, val) != 0) {
        hd->errstr = "Error setting local CFG cache";
        return NULL;
    }

    return hd->buf;
}

/**
 * \brief Fetch a new configuration from the Harmony session.
 *
 * If a new configuration is available, this function will update the
 * values of all registered variables.  Otherwise, it will configure
 * the system to run with the best known configuration thus far.
 *
 * \param hd Harmony descriptor returned from ah_init().
 *
 * \return Returns 0 if no registered variables were modified, 1 if
 *         any registered variables were modified, and -1 otherwise.
 */
int ah_fetch(hdesc_t* hd)
{
    int i;

    if (hd->state < HARMONY_STATE_CONNECTED) {
        hd->errstr = "Descriptor not currently joined to any session";
        errno = EINVAL;
        return -1;
    }

    if (hd->state == HARMONY_STATE_READY) {
        /* Prepare a Harmony message. */
        if (send_request(hd, HMESG_FETCH) != 0)
            return -1;

        if (hd->mesg.status == HMESG_STATUS_BUSY) {
            if (!hd->best.id) {
                /* No best point is available yet.  Do not set values. */
                return 0;
            }

            /* Set current point to best point. */
            if (hpoint_copy(&hd->curr, &hd->best) != 0) {
                hd->errstr = "Error copying best point data";
                return -1;
            }
        }
        else if (hd->mesg.status == HMESG_STATUS_OK) {
            if (hpoint_copy(&hd->curr, &hd->mesg.data.point) != 0) {
                hd->errstr = "Error copying current point data";
                return -1;
            }
        }
        else {
            hd->errstr = "Invalid message received from server";
            errno = EINVAL;
            return -1;
        }
    }

    /* Update the variables from the content of the message. */
    if (write_values(hd, &hd->curr) != 0)
        return -1;

    /* Initialize our internal performance array. */
    for (i = 0; i < hd->perf.len; ++i)
        hd->perf.obj[i] = NAN;

    /* Client variables were changed.  Inform the user by returning 1. */
    hd->state = HARMONY_STATE_TESTING;
    return 1;
}

/**
 * \brief Report the performance of a configuration to the Harmony session.
 *
 * Sends a report of the tested performance to the Harmony session.
 * The report will be analyzed by the session to produce values for
 * the next set of calls to ah_fetch().
 *
 * The `value` parameter should point to a floating-point double.  For
 * multi-objective tuning, where multiple performance values are
 * required for a single test, `value` should point to an array of
 * floating-point doubles.  Finally, `value` may be NULL, provided
 * that all performance values have already been set via
 * ah_report_one().
 *
 * Sends a complete performance report regarding the current
 * configuration to the Harmony session.  For multi-objective search
 * problems, the performance parameter should point to an array
 * containing all performance values.
 *
 * If all performance values have been reported via
 * ah_report_one(), then `NULL` may be passed as the performance
 * pointer.  Unreported performance values will result in error.
 *
 * \param hd   Harmony descriptor returned from ah_init().
 * \param perf Performance vector for the current configuration.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int ah_report(hdesc_t* hd, double* perf)
{
    int i;

    if (hd->state < HARMONY_STATE_CONNECTED) {
        hd->errstr = "Descriptor not currently joined to any session";
        errno = EINVAL;
        return -1;
    }

    if (hd->state < HARMONY_STATE_TESTING)
        return 0;

    if (perf)
        memcpy(hd->perf.obj, perf, sizeof(*hd->perf.obj) * hd->perf.len);

    for (i = 0; i < hd->perf.len; ++i) {
        if (isnan(hd->perf.obj[i])) {
            hd->errstr = "Insufficient performance values to report";
            errno = EINVAL;
            return -1;
        }
    }

    /* Prepare a Harmony message. */
    hd->mesg.data.point.id = hd->curr.id;
    if (hperf_copy(&hd->mesg.data.perf, &hd->perf) != 0) {
        hd->errstr = "Error allocating performance array for message";
        return -1;
    }

    if (send_request(hd, HMESG_REPORT) != 0)
        return -1;

    if (hd->mesg.status != HMESG_STATUS_OK) {
        hd->errstr = "Invalid message received from server";
        return -1;
    }

    hd->state = HARMONY_STATE_READY;
    return 0;
}

/**
 * \brief Report a single performance value for the current configuration.
 *
 * Allows performance values to be reported one at a time for
 * multi-objective search problems.
 *
 * Note that this function only caches values to send to the Harmony
 * session.  Once all values have been reported, ah_report() must
 * be called (passing `NULL` as the performance argument).
 *
 * \param hd    Harmony descriptor returned from ah_init().
 * \param index Objective index of the value to report.
 * \param value Performance measured for the current configuration.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int ah_report_one(hdesc_t* hd, int index, double value)
{
    if (hd->state < HARMONY_STATE_CONNECTED) {
        hd->errstr = "Descriptor not currently joined to any session";
        errno = EINVAL;
        return -1;
    }

    if (index != 0 || index >= hd->perf.len) {
        hd->errstr = "Invalid performance index";
        errno = EINVAL;
        return -1;
    }

    if (hd->state == HARMONY_STATE_TESTING)
        hd->perf.obj[index] = value;

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
 * \param hd Harmony descriptor returned from ah_init().
 *
 * \return Returns 1 if a new best point was retrieved from the
 *         session, 0 if a local copy was used, and -1 otherwise.
 */
int ah_best(hdesc_t* hd)
{
    int retval = 0;

    if (hd->state >= HARMONY_STATE_CONNECTED) {
        /* Prepare a Harmony message. */
        if (send_request(hd, HMESG_BEST) != 0)
            return -1;

        if (hd->mesg.status != HMESG_STATUS_OK) {
            hd->errstr = "Invalid message received from server";
            return -1;
        }
        retval = 1;
    }

    /* Make sure our best known point is valid. */
    if (!hd->best.id) {
        errno = EINVAL;
        return -1;
    }

    if (write_values(hd, &hd->best) != 0)
        return -1;

    return retval;
}

/**
 * \brief Retrieve the convergence state of the current search.
 *
 * \param hd Harmony descriptor returned from ah_init().
 *
 * \return Returns 1 if the search has converged, 0 if it has not,
 *         and -1 on error.
 */
int ah_converged(hdesc_t* hd)
{
    char* retval = ah_get_cfg(hd, CFGKEY_CONVERGED);
    return (retval && retval[0] == '1');
}

/**
 * \brief Access the current Harmony error string.
 *
 * \param hd Harmony descriptor returned from ah_init().
 *
 * \return Returns a pointer to a string that describes the latest
 *         Harmony error, or `NULL` if no error has occurred since
 *         the last call to ah_error_clear().
 */
const char* ah_error_string(hdesc_t* hd)
{
    return hd->errstr;
}

/**
 * \brief Clears the error status of the given Harmony descriptor.
 *
 * \param hd Harmony descriptor returned from ah_init().
 */
void ah_error_clear(hdesc_t* hd)
{
    hd->errstr = NULL;
}

/**
 * @}
 */

/* ----------------------------------------
 * Deprecated function implementations.
 */

hdesc_t* harmony_init(void)
{
    return ah_init();
}

int harmony_parse_args(hdesc_t* hd, int argc, char** argv)
{
    return ah_args(hd, &argc, argv);
}

void harmony_fini(hdesc_t* hd)
{
    ah_fini(hd);
}

int harmony_int(hdesc_t* hd, const char* name,
                long min, long max, long step)
{
    return ah_int(hd, name, min, max, step);
}

int harmony_real(hdesc_t* hd, const char* name,
                 double min, double max, double step)
{
    return ah_real(hd, name, min, max, step);
}

int harmony_enum(hdesc_t* hd, const char* name, const char* value)
{
    return ah_enum(hd, name, value);
}

int harmony_session_name(hdesc_t* hd, const char* name)
{
    return hsig_name(&hd->sig, name);
}

int harmony_strategy(hdesc_t* hd, const char* strategy)
{
    return ah_strategy(hd, strategy);
}

int harmony_layers(hdesc_t* hd, const char* list)
{
    return ah_layers(hd, list);
}

int harmony_launch(hdesc_t* hd, const char* host, int port)
{
    return ah_launch(hd, host, port, NULL);
}

int harmony_id(hdesc_t* hd, const char* id)
{
    return ah_id(hd, id);
}

int harmony_bind_int(hdesc_t* hd, const char* name, long* ptr)
{
    return ah_bind_int(hd, name, ptr);
}

int harmony_bind_real(hdesc_t* hd, const char* name, double* ptr)
{
    return ah_bind_real(hd, name, ptr);
}

int harmony_bind_enum(hdesc_t* hd, const char* name, const char** ptr)
{
    return ah_bind_enum(hd, name, ptr);
}

int harmony_join(hdesc_t* hd, const char* host, int port, const char* name)
{
    return ah_join(hd, host, port, name);
}

int harmony_leave(hdesc_t* hd)
{
    return ah_leave(hd);
}

long harmony_get_int(hdesc_t* hd, const char* name)
{
    return ah_get_int(hd, name);
}

double harmony_get_real(hdesc_t* hd, const char* name)
{
    return ah_get_real(hd, name);
}

const char* harmony_get_enum(hdesc_t* hd, const char* name)
{
    return ah_get_enum(hd, name);
}

char* harmony_getcfg(hdesc_t* hd, const char* key)
{
    return ah_get_cfg(hd, key);
}

char* harmony_setcfg(hdesc_t* hd, const char* key, const char* val)
{
    return ah_set_cfg(hd, key, val);
}

int harmony_fetch(hdesc_t* hd)
{
    return ah_fetch(hd);
}

int harmony_report(hdesc_t* hd, double* perf)
{
    return ah_report(hd, perf);
}

int harmony_report_one(hdesc_t* hd, int index, double value)
{
    return ah_report_one(hd, index, value);
}

int harmony_best(hdesc_t* hd)
{
    return ah_best(hd);
}

int harmony_converged(hdesc_t* hd)
{
    return ah_converged(hd);
}

const char* harmony_error_string(hdesc_t* hd)
{
    return ah_error_string(hd);
}

void harmony_error_clear(hdesc_t* hd)
{
    ah_error_clear(hd);
}

/* ----------------------------------------
 * Private helper function implementations.
 */
int find_var(hdesc_t* hd, const char* name)
{
    int idx;

    for (idx = 0; idx < hd->sig.range_len; ++idx) {
        if (strcmp(hd->sig.range[idx].name, name) == 0)
            return idx;
    }
    return -1;
}

int extend_perf(hdesc_t* hd)
{
    int perf_len;
    char* cfgval;

    cfgval = ah_get_cfg(hd, CFGKEY_PERF_COUNT);
    if (!cfgval) {
        hd->errstr = "Error retrieving performance count";
        return -1;
    }

    perf_len = atoi(cfgval);
    if (perf_len < 1) {
        hd->errstr = "Invalid value for " CFGKEY_PERF_COUNT;
        return -1;
    }

    if (hperf_init(&hd->perf, perf_len) != 0) {
        hd->errstr = "Error allocating performance array";
        return -1;
    }

    return 0;
}

int extend_varloc(hdesc_t* hd)
{
    if (hd->varloc_cap < hd->sig.range_cap) {
        int i;
        hvarloc_t* tmp = realloc(hd->varloc,
                                 hd->sig.range_cap * sizeof(*tmp));
        if (!tmp)
            return -1;

        for (i = hd->varloc_cap; i < hd->sig.range_cap; ++i)
            tmp[i].ptr = NULL;

        hd->varloc = tmp;
        hd->varloc_cap = i;
    }
    return 0;
}

void generate_id(hdesc_t* hd)
{
    char* buf = hd->default_id;

    gethostname(buf, HOST_NAME_MAX);
    buf[HOST_NAME_MAX + 1] = '\0';
    buf += strlen(buf);
    sprintf(buf, "_%d_%d", getpid(), hd->socket);
}

int send_request(hdesc_t* hd, hmesg_type msg_type)
{
    hd->mesg.type = msg_type;
    hd->mesg.status = HMESG_STATUS_REQ;

    hd->mesg.state.sig.id = hd->sig.id;
    hd->mesg.state.best.id = hd->best.id;
    hd->mesg.state.client = hd->id;

    if (mesg_send(hd->socket, &hd->mesg) < 1) {
        hd->errstr = "Error sending Harmony message to server";
        errno = EIO;
        return -1;
    }

    do {
        if (mesg_recv(hd->socket, &hd->mesg) < 1) {
            hd->errstr = "Error retrieving Harmony message from server";
            errno = EIO;
            return -1;
        }

        /* If the httpinfo layer is enabled during a stand-alone session,
         * it will generate extraneous messages where origin == -1.
         * Make sure to ignore these messages.
         */
    } while (hd->mesg.origin == -1);

    if (hd->mesg.type != msg_type) {
        hd->mesg.status = HMESG_STATUS_FAIL;
        hd->errstr = "Server response message mismatch";
        return -1;
    }

    if (hd->mesg.status == HMESG_STATUS_FAIL) {
        hd->errstr = hd->mesg.data.string;
        return -1;
    }
    else {
        // Update local state from server.
        if (hd->sig.id < hd->mesg.state.sig.id) {
            if (hsig_copy(&hd->sig, &hd->mesg.state.sig) != 0) {
                hd->errstr = "Could not update session signature";
                return -1;
            }
        }
        if (hd->best.id < hd->mesg.state.best.id) {
            if (hpoint_copy(&hd->best, &hd->mesg.state.best) != 0) {
                hd->errstr = "Could not update best known point";
                return -1;
            }
        }
    }
    return 0;
}

int set_varloc(hdesc_t* hd, const char* name, void* ptr)
{
    int idx = find_var(hd, name);
    if (idx < 0) {
        hd->errstr = "Tuning variable not found";
        errno = EINVAL;
        return -1;
    }

    extend_varloc(hd);
    hd->varloc[idx].ptr = ptr;
    return 0;
}

int write_values(hdesc_t* hd, const hpoint_t* pt)
{
    int i;

    if (hd->sig.range_len != pt->len) {
        hd->errstr = "Invalid internal point structure";
        return -1;
    }

    for (i = 0; i < pt->len; ++i) {
        if (hd->varloc[i].ptr == NULL) {
            hd->varloc[i].val = pt->val[i];
        }
        else {
            switch (pt->val[i].type) {
            case HVAL_INT:
                *(long*)hd->varloc[i].ptr = pt->val[i].value.i;
                break;
            case HVAL_REAL:
                *(double*)hd->varloc[i].ptr = pt->val[i].value.r;
                break;
            case HVAL_STR:
                *(const char**)hd->varloc[i].ptr = pt->val[i].value.s;
                break;
            default:
                hd->errstr = "Invalid signature value type";
                return -1;
            }
        }
    }
    return 0;
}
