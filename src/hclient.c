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
#define _XOPEN_SOURCE 500 // Needed for gethostname()

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

struct hdesc_t {
    harmony_state_t state;
    int socket;
    char* id;

    hsession_t sess;
    hmesg_t mesg;

    void** ptr;
    int ptr_len;
    int ptr_cap;

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
int   ptr_bind(hdesc_t* hdesc, const char* name, hval_type type, void* ptr);
int   send_request(hdesc_t* hdesc, hmesg_type msg_type);
int   update_best(hdesc_t* hdesc, const hpoint_t* pt);
int   set_values(hdesc_t* hdesc, const hpoint_t* pt);

static int debug_mode = 0;

/* ----------------------------------
 * Public Client API Implementations.
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

    hdesc->ptr = NULL;
    hdesc->ptr_len = hdesc->ptr_cap = 0;

    hdesc->curr = HPOINT_INITIALIZER;
    hdesc->best = HPOINT_INITIALIZER;

    hdesc->buf = NULL;
    hdesc->buflen = 0;
    hdesc->errstr = NULL;

    hdesc->id = NULL;

    return hdesc;
}

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
        free(hdesc->ptr);
        free(hdesc);
    }
}

/* -------------------------------------------------------------------
 * Session Establishment API Implementations
 */
int harmony_session_name(hdesc_t* hdesc, const char* name)
{
    return hsignature_name(&hdesc->sess.sig, name);
}

int harmony_int(hdesc_t* hdesc, const char* name,
                long min, long max, long step)
{
    return hsignature_int(&hdesc->sess.sig, name, min, max, step);
}

int harmony_real(hdesc_t* hdesc, const char* name,
                 double min, double max, double step)
{
    return hsignature_real(&hdesc->sess.sig, name, min, max, step);
}

int harmony_enum(hdesc_t* hdesc, const char* name, const char* value)
{
    return hsignature_enum(&hdesc->sess.sig, name, value);
}

int harmony_strategy(hdesc_t* hdesc, const char* strategy)
{
    return hcfg_set(&hdesc->sess.cfg, CFGKEY_STRATEGY, strategy);
}

int harmony_layers(hdesc_t* hdesc, const char* list)
{
    return hcfg_set(&hdesc->sess.cfg, CFGKEY_LAYERS, list);
}

int harmony_launch(hdesc_t* hdesc, const char* host, int port)
{
    /* Sanity check input */
    if (hdesc->sess.sig.range_len < 1) {
        hdesc->errstr = "No tuning variables defined";
        errno = EINVAL;
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

/* -------------------------------------------------------------------
 * Tuning Client Establishment API Implementations
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

int harmony_bind_int(hdesc_t* hdesc, const char* name, long* ptr)
{
    return ptr_bind(hdesc, name, HVAL_INT, ptr);
}

int harmony_bind_real(hdesc_t* hdesc, const char* name, double* ptr)
{
    return ptr_bind(hdesc, name, HVAL_REAL, ptr);
}

int harmony_bind_enum(hdesc_t* hdesc, const char* name, const char** ptr)
{
    return ptr_bind(hdesc, name, HVAL_STR, (void*)ptr);
}

int ptr_bind(hdesc_t* hdesc, const char* name, hval_type type, void* ptr)
{
    int i;
    hsignature_t* sig = &hdesc->sess.sig;

    for (i = 0; i < sig->range_len; ++i) {
        if (strcmp(sig->range[i].name, name) == 0)
            break;
    }
    if (i == sig->range_cap) {
        if (array_grow(&sig->range, &sig->range_cap, sizeof(hrange_t)) != 0)
            return -1;
    }

    if (i == sig->range_len) {
        sig->range[i].name = stralloc(name);
        if (!sig->range[i].name)
            return -1;

        sig->range[i].type = type;
        ++sig->range_len;
    }

    if (hdesc->ptr_cap < sig->range_cap) {
        void** tmpbuf = realloc(hdesc->ptr, sig->range_cap * sizeof(void*));
        if (!tmpbuf)
            return -1;

        hdesc->ptr = tmpbuf;
        hdesc->ptr_cap = sig->range_cap;
    }

    hdesc->ptr[i] = ptr;
    ++hdesc->ptr_len;

    return 0;
}

int harmony_join(hdesc_t* hdesc, const char* host, int port, const char* name)
{
    int perf_len;
    char* cfgval;

    /* Verify that we have *at least* one variable bound, and that
     * this descriptor isn't already associated with a tuning
     * session.
     */
    if (hdesc->ptr_len == 0) {
        hdesc->errstr = "No variables bound to Harmony session";
        errno = EINVAL;
        return -1;
    }

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
    hdesc->ptr_len = 0;
    hdesc->best.id = -1;

    return 0;
}

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

const char* harmony_error_string(hdesc_t* hdesc)
{
    return hdesc->errstr;
}

void harmony_error_clear(hdesc_t* hdesc)
{
    hdesc->errstr = NULL;
}

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
        errno = ECOMM;
        return -1;
    }

    do {
        if (mesg_recv(hdesc->socket, &hdesc->mesg) < 1) {
            hdesc->errstr = "Error retrieving Harmony message from server.";
            errno = ECOMM;
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

int set_values(hdesc_t* hdesc, const hpoint_t* pt)
{
    int i;

    if (hdesc->ptr_len != pt->n) {
        hdesc->errstr = "Invalid internal point structure.";
        errno = EINVAL;
        return -1;
    }

    for (i = 0; i < pt->n; ++i) {
        switch (pt->val[i].type) {
        case HVAL_INT:
            (*((       long*)hdesc->ptr[i])) = pt->val[i].value.i; break;
        case HVAL_REAL:
            (*((     double*)hdesc->ptr[i])) = pt->val[i].value.r; break;
        case HVAL_STR:
            (*((const char**)hdesc->ptr[i])) = pt->val[i].value.s; break;
        default:
            hdesc->errstr = "Internal error: Invalid signature value type.";
            errno = EINVAL;
            return -1;
        }
    }
    return 0;
}
