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

/**
 * \page codesvr Code Server (codegen.so)
 *
 * This processing layer passes messages from the tuning session to a
 * running code generation server.  Details on how to configure and
 * run a code generation tuning session are provided in
 * `code-server/README` of the distribution tarball.
 *
 * ## Code Server URLs ##
 * The code server uses a proprietary set of URL's to determine the
 * destination for various communications.  They take the following
 * form:
 *
 *     dir://<path>
 *     ssh://[user@]<host>[:port]/<path>
 *     tcp://<host>[:port]
 *
 * All paths are considered relative.  Use an additional slash to
 * specify an absolute path.  For example:
 *
 *     dir:///tmp
 *     ssh://code.server.net:2222//tmp/codegen
 */

#include "session-core.h"
#include "hsig.h"
#include "hcfg.h"
#include "hmesg.h"
#include "hpoint.h"
#include "hutil.h"
#include "hsockutil.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*
 * Name used to identify this plugin layer.
 * All Harmony plugin layers must define this variable.
 */
const char harmony_layer_name[] = "codegen";

/*
 * Configuration variables used in this plugin.
 * These will automatically be registered by session-core upon load.
 */
hcfg_info_t plugin_keyinfo[] = {
    { CFGKEY_SERVER_URL, NULL,
      "Destination of messages from session to code server." },
    { CFGKEY_TARGET_URL, NULL,
      "Destination of binary files from code server to target application." },
    { CFGKEY_REPLY_URL, NULL,
      "Destination of reply messages from code server to session. "
      "A reasonable directory in " CFGKEY_TMPDIR " will be generated "
      "by default if left blank." },
    { CFGKEY_TMPDIR, "/tmp",
      "Session local path to store temporary files." },
    { CFGKEY_SLAVE_LIST, NULL,
      "Comma separated list of 'host n' pairs, where 'n' slaves will run "
      "on 'host'.  For example 'bunker.cs.umd.edu 4, nexcor.cs.umd.edu 4' "
      "will instruct the code server to spawn 8 total slaves between two "
      "machines." },
    { CFGKEY_SLAVE_PATH, NULL,
      "Path on slave host to store generated binaries before being sent "
      "to the target application." },
    { NULL }
};

typedef enum {
    CODEGEN_STATUS_UNKNOWN = 0x0,
    CODEGEN_STATUS_REQUESTED,
    CODEGEN_STATUS_COMPLETE,

    CODEGEN_STATUS_MAX
} codegen_status;

typedef struct codegen_log {
    codegen_status status;
    hpoint_t point;
} codegen_log_t;

codegen_log_t* cglog;
int cglog_len, cglog_cap;

hmesg_t mesg = HMESG_INITIALIZER;
int sockfd;

char* buf;
int buflen;

int codegen_callback(int fd, hflow_t* flow, int n, htrial_t** trial);
int url_connect(const char* url);
int cglog_insert(const hpoint_t* pt);
int cglog_find(const hpoint_t* pt);

/*
 * Invoked once on module load.
 *
 * This routine should return 0 on success, and -1 otherwise.
 */
int codegen_init(hsig_t* sig)
{
    const char* url;

    url = hcfg_get(session_cfg, CFGKEY_TARGET_URL);
    if (!url || url[0] == '\0') {
        session_error("Destination URL for"
                      " generated code objects not specified.");
        return -1;
    }

    url = hcfg_get(session_cfg, CFGKEY_SERVER_URL);
    if (!url || url[0] == '\0') {
        session_error("Codegen server URL not specified.");
        return -1;
    }

    buf = NULL;
    buflen = 0;
    sockfd = url_connect(url);
    if (sockfd == -1) {
        session_error("Invalid codegen server URL");
        return -1;
    }

    /* In the future, we should rewrite the code generator system to
     * avoid using hmesg_t types.  Until then, generate a fake
     * HMESG_SESSION message to maintain compatibility.
     */
    mesg.type = HMESG_SESSION;
    mesg.status = HMESG_STATUS_REQ;

    hsig_copy(&mesg.data.session.sig, sig);
    hcfg_copy(&mesg.data.session.cfg, session_cfg);

    /* Memory allocated for mesg is freed after mesg_send(). */
    if (mesg_send(sockfd, &mesg) < 1)
        return -1;

    if (mesg_recv(sockfd, &mesg) < 1)
        return -1;

    /* TODO: Need a way to unregister a callback for reinitialization. */
    if (callback_generate(sockfd, codegen_callback) != 0) {
        session_error("Could not register callback for codegen plugin");
        return -1;
    }

    return 0;
}

/*
 * Called after search driver generates a candidate point, but before
 * that point is returned to the client API.
 *
 * This routine should fill the flow variable appropriately and return
 * 0 upon success.  Otherwise, it should call session_error() with a
 * human-readable error message and return -1.
 */
int codegen_generate(hflow_t* flow, htrial_t* trial)
{
    int i;

    i = cglog_find(&trial->point);
    if (i >= 0) {
        if (cglog[i].status == CODEGEN_STATUS_COMPLETE)
            flow->status = HFLOW_ACCEPT;

        if (cglog[i].status == CODEGEN_STATUS_REQUESTED)
            flow->status = HFLOW_WAIT;

        return 0;
    }

    if (cglog_insert(&trial->point) != 0) {
        session_error("Internal error: Could not grow codegen log");
        return -1;
    }

    mesg = hmesg_zero;
    mesg.type = HMESG_FETCH;
    mesg.status = HMESG_STATUS_OK;
    mesg.data.point = hpoint_zero;
    hpoint_copy(&mesg.data.point, &trial->point);

    if (mesg_send(sockfd, &mesg) < 1) {
        session_error( strerror(errno) );
        return -1;
    }

    flow->status = HFLOW_WAIT;
    return 0;
}

/*
 * Invoked after the tuning session completes.
 */
void codegen_fini(void)
{
    close(sockfd);
    free(buf);
}

int codegen_callback(int fd, hflow_t* flow, int n, htrial_t** trial)
{
    int i;

    if (mesg_recv(fd, &mesg) < 1)
        return -1;

    i = cglog_find(&mesg.data.point);
    if (i < 0) {
        session_error("Could not find point from code server in log");
        return -1;
    }
    cglog[i].status = CODEGEN_STATUS_COMPLETE;

    /* Search waitlist for index of returned point. */
    for (i = 0; i < n; ++i) {
        if (trial[i]->point.id == mesg.data.point.id) {
            flow->status = HFLOW_ACCEPT;
            return i;
        }
    }

    session_error("Could not find point from code server in waitlist");
    return -1;
}

int url_connect(const char* url)
{
    const char* ptr;
    char* helper_argv[2];
    int count;

    ptr = strstr(url, "//");
    if (!ptr)
        return -1;

    if (strncmp("dir:", url, ptr - url) == 0 ||
        strncmp("ssh:", url, ptr - url) == 0)
    {
        ptr = hcfg_get(session_cfg, CFGKEY_HARMONY_HOME);
        if (!ptr)
            return -1;

        count = snprintf_grow(&buf, &buflen, "%s/libexec/codegen-helper", ptr);
        if (count < 0)
            return -1;

        helper_argv[0] = buf;
        helper_argv[1] = NULL;
        return socket_launch(buf, helper_argv, NULL);
    }
    else if (strncmp("tcp:", url, ptr - url) == 0) {
        /* Not implemented yet. */
    }
    return -1;
}

int cglog_insert(const hpoint_t* point)
{
    if (cglog_len == cglog_cap)
        if (array_grow(&cglog, &cglog_cap, sizeof(codegen_log_t)) < 0)
            return -1;

    cglog[cglog_len].point = hpoint_zero;
    hpoint_copy(&cglog[cglog_len].point, point);
    cglog[cglog_len].status = CODEGEN_STATUS_REQUESTED;
    ++cglog_len;

    return 0;
}

int cglog_find(const hpoint_t* point)
{
    int i;
    int len = point->n * sizeof(hval_t);

    for (i = 0; i < cglog_len; ++i) {
        if (memcmp(point->val, cglog[i].point.val, len) == 0)
            return i;
    }

    return -1;
}
