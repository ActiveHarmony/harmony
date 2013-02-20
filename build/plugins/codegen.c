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

typedef enum {
    CODEGEN_STATUS_UNKNOWN = 0x0,
    CODEGEN_STATUS_REQUESTED,
    CODEGEN_STATUS_COMPLETE,

    CODEGEN_STATUS_MAX
} codegen_status;

typedef struct codegen_log {
    codegen_status status;
    hpoint_t pt;
} codegen_log_t;

/*
 * Name used to identify this plugin.  All Harmony plugins must define
 * this variable.
 */
const char harmony_plugin_name[] = "codegen";

hsession_t *sess;
hmesg_t cg_mesg;
int cg_sock;

char *buf;
int buflen;

codegen_log_t *cglog;
int cglog_len, cglog_cap;

int codegen_callback(hmesg_t *mesg);
int url_connect(const char *url);
int cglog_insert(hpoint_t *pt);
int cglog_find(hpoint_t *pt);

/*
 * Invoked once on module load.
 *
 * This routine should return 0 on success, and -1 otherwise.
 */
int codegen_init(hmesg_t *mesg)
{
    const char *url;

    sess = &mesg->data.session;

    cg_mesg = HMESG_INITIALIZER;
    buf = NULL;
    buflen = 0;

    url = hcfg_get(sess->cfg, CFGKEY_CG_TARGET_URL);
    if (!url || url[0] == '\0') {
        mesg->data.string =
            "Destination URL for generated code objects not specified";
        return -1;
    }

    url = hcfg_get(sess->cfg, CFGKEY_CG_SERVER_URL);
    if (!url || url[0] == '\0') {
        mesg->data.string = "Codegen server URL not specified";
        return -1;
    }

    cg_sock = url_connect(url);
    if (cg_sock == -1) {
        mesg->data.string = "Invalid codegen server URL";
        return -1;
    }

    cg_mesg.type = HMESG_SESSION;
    cg_mesg.status = HMESG_STATUS_REQ;
    if (hsession_copy(&cg_mesg.data.session, &mesg->data.session) < 0) {
        mesg->data.string = "Internal error copying session data";
        return -1;
    }

    if (mesg_send(cg_sock, &cg_mesg) < 1)
        return -1;

    if (mesg_recv(cg_sock, &cg_mesg) < 1)
        return -1;

    if (callback_register(cg_sock, codegen_callback) < 0) {
        mesg->data.string = "Could not register callback for codegen plugin";
        return -1;
    }

    return 0;
}

/*
 * Called after search driver generates a candidate point, but before
 * that point is returned to the client API.
 *
 * This routine should return 0 if it is completely done processing
 * the message.  Otherwise, -1 should be returned with mesg->status
 * (and possibly errno) set appropriately.
 */
int codegen_fetch(hmesg_t *mesg)
{
    int i;

    i = cglog_find(&mesg->data.fetch.cand);
    if (i >= 0) {
        if (cglog[i].status == CODEGEN_STATUS_COMPLETE)
            return 0;

        if (cglog[i].status == CODEGEN_STATUS_REQUESTED) {
            hmesg_scrub(mesg);
            mesg->status = HMESG_STATUS_BUSY;
            return -1;
        }
    }

    if (cglog_insert(&mesg->data.fetch.cand) < 0) {
        hmesg_scrub(mesg);
        mesg->status = HMESG_STATUS_FAIL;
        mesg->data.string = "Internal error growing codegen log";
        return -1;
    }

    cg_mesg.dest = mesg->dest;
    cg_mesg.type = mesg->type;
    cg_mesg.status = mesg->status;
    cg_mesg.index = mesg->index;
    cg_mesg.data.fetch.cand = HPOINT_INITIALIZER;
    cg_mesg.data.fetch.best = HPOINT_INITIALIZER;
    hpoint_copy(&cg_mesg.data.fetch.cand, &mesg->data.fetch.cand);

    hmesg_scrub(mesg);
    if (mesg_send(cg_sock, &cg_mesg) < 1)
        mesg->status = HMESG_STATUS_FAIL;
    else
        mesg->status = HMESG_STATUS_BUSY;

    /* Either an error has occurred, or the message has been forwarded
     * to the code server for processing.  In both cases, we should
     * return -1 and inform our parent session that the workflow has
     * been interrupted.
     */
    return -1;
}

/*
 * Invoked after the tuning session completes.
 */
void codegen_fini(void)
{
    free(buf);
}

int codegen_callback(hmesg_t *mesg)
{
    int i;

    i = cglog_find(&mesg->data.fetch.cand);
    if (i < 0)
        return -1;

    if (mesg->status != HMESG_STATUS_OK)
        return -1;

    cglog[i].status = CODEGEN_STATUS_COMPLETE;
    return 0;
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
        ptr = hcfg_get(sess->cfg, CFGKEY_HARMONY_ROOT);
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

int cglog_insert(hpoint_t *pt)
{
    if (cglog_len == cglog_cap)
        if (array_grow(&cglog, &cglog_cap, sizeof(codegen_log_t)) < 0)
            return -1;

    cglog[cglog_len].pt = HPOINT_INITIALIZER;
    hpoint_copy(&cglog[cglog_len].pt, pt);
    cglog[cglog_len].status = CODEGEN_STATUS_REQUESTED;
    ++cglog_len;

    return 0;
}

int cglog_find(hpoint_t *pt)
{
    int i;

    for (i = 0; i < cglog_len; ++i)
        if (memcmp(pt->val, cglog[i].pt.val, pt->n * sizeof(hval_t)) == 0)
            return i;

    return -1;
}
