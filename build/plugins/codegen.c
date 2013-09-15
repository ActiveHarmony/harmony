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

#include "session-core.h"
#include "hsession.h"
#include "hmesg.h"
#include "hpoint.h"
#include "hutil.h"
#include "hcfg.h"
#include "hsockutil.h"
#include "defaults.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

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

codegen_log_t *cglog;
int cglog_len, cglog_cap;

/*
 * Name used to identify this plugin.  All Harmony plugins must define
 * this variable.
 */
const char harmony_layer_name[] = "codegen";

hmesg_t mesg;
int sockfd;

char *buf;
int buflen;

int codegen_callback(int fd, hflow_t *flow, int n, htrial_t **trial);
int url_connect(const char *url);
int cglog_insert(const hpoint_t *pt);
int cglog_find(const hpoint_t *pt);

/*
 * Invoked once on module load.
 *
 * This routine should return 0 on success, and -1 otherwise.
 */
int codegen_init(hsignature_t *sig)
{
    const char *url;
    hcfg_t *cfg;

    url = session_query(CFGKEY_CG_TARGET_URL);
    if (!url || url[0] == '\0') {
        session_error("Destination URL for"
                      " generated code objects not specified");
        return -1;
    }

    url = session_query(CFGKEY_CG_SERVER_URL);
    if (!url || url[0] == '\0') {
        session_error("Codegen server URL not specified");
        return -1;
    }

    sockfd = url_connect(url);
    if (sockfd == -1) {
        session_error("Invalid codegen server URL");
        return -1;
    }

    /* In the future, we should rewrite the code generator system to
     * avoid using hmesg_t types.  Until then, generate a fake
     * HMESG_SESSION message to maintain compatibility.
     */
    mesg = HMESG_INITIALIZER;

    hsignature_copy(&mesg.data.session.sig, sig);
    cfg = hcfg_alloc();
    hcfg_set(cfg, CFGKEY_CG_SERVER_URL, session_query(CFGKEY_CG_SERVER_URL));
    hcfg_set(cfg, CFGKEY_CG_TARGET_URL, session_query(CFGKEY_CG_TARGET_URL));
    hcfg_set(cfg, CFGKEY_CG_REPLY_URL, session_query(CFGKEY_CG_REPLY_URL));
    hcfg_set(cfg, CFGKEY_CG_SLAVE_LIST, session_query(CFGKEY_CG_SLAVE_LIST));
    hcfg_set(cfg, CFGKEY_CG_SLAVE_PATH, session_query(CFGKEY_CG_SLAVE_PATH));
    mesg.data.session.cfg = cfg;

    mesg.type = HMESG_SESSION;
    mesg.status = HMESG_STATUS_REQ;
    if (mesg_send(sockfd, &mesg) < 1)
        return -1;

    if (mesg_recv(sockfd, &mesg) < 1)
        return -1;

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
int codegen_generate(hflow_t *flow, htrial_t *trial)
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

    mesg = HMESG_INITIALIZER;
    mesg.type = HMESG_FETCH;
    mesg.status = HMESG_STATUS_OK;
    mesg.data.fetch.cand = HPOINT_INITIALIZER;
    mesg.data.fetch.best = HPOINT_INITIALIZER;
    hpoint_copy(&mesg.data.fetch.cand, &trial->point);

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
    free(buf);
}

int codegen_callback(int fd, hflow_t *flow, int n, htrial_t **trial)
{
    int i;

    if (mesg_recv(fd, &mesg) < 1)
        return -1;

    i = cglog_find(&mesg.data.fetch.cand);
    if (i < 0) {
        session_error("Could not find point from code server in log");
        return -1;
    }
    cglog[i].status = CODEGEN_STATUS_COMPLETE;

    /* Search waitlist for index of returned point. */
    for (i = 0; i < n; ++i) {
        if (trial[i]->point.id == mesg.data.fetch.cand.id) {
            flow->status = HFLOW_ACCEPT;
            return i;
        }
    }

    session_error("Could not find point from code server in waitlist");
    return -1;
}

int url_connect(const char *url)
{
    const char *ptr;
    char *helper_argv[2];
    int count;

    ptr = strstr(url, "//");
    if (!ptr)
        return -1;

    if (strncmp("dir:", url, ptr - url) == 0 ||
        strncmp("ssh:", url, ptr - url) == 0)
    {
        ptr = session_query(CFGKEY_HARMONY_ROOT);
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

int cglog_insert(const hpoint_t *point)
{
    if (cglog_len == cglog_cap)
        if (array_grow(&cglog, &cglog_cap, sizeof(codegen_log_t)) < 0)
            return -1;

    cglog[cglog_len].point = HPOINT_INITIALIZER;
    hpoint_copy(&cglog[cglog_len].point, point);
    cglog[cglog_len].status = CODEGEN_STATUS_REQUESTED;
    ++cglog_len;

    return 0;
}

int cglog_find(const hpoint_t *point)
{
    int i;
    int len = point->n * sizeof(hval_t);

    for (i = 0; i < cglog_len; ++i) {
        if (memcmp(point->val, cglog[i].point.val, len) == 0)
            return i;
    }

    return -1;
}
