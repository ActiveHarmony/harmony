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
#ifndef __HSERVER_H__
#define __HSERVER_H__

#include "hmesg.h"
#include "hpoint.h"
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct http_log {
    struct timeval stamp;
    hpoint_t pt;
    double perf;
} http_log_t;

typedef struct session_state {
    char *name;
    int fd, old_fd;
    int *client;
    int client_len, client_cap;
    hpoint_t best;
    double best_perf;

    /* Fields used by the HTTP server. */
    struct timeval start;
    hsignature_t sig;
    char *strategy;
    unsigned int status;

    http_log_t *log;
    int log_len, log_cap;
    hpoint_t *fetched;
    int fetched_len, fetched_cap;
    int reported;
} session_state_t;

extern session_state_t *slist;
extern int slist_cap;

extern hmesg_t mesg_in;

session_state_t *session_open(hmesg_t *mesg);
void session_close(session_state_t *sess);
const char *session_getcfg(session_state_t *sess, const char *key);
int session_setcfg(session_state_t *sess, const char *key, const char *val);
int session_restart(session_state_t *sess);

#ifdef __cplusplus
}
#endif

#endif /* __HSERVER_H__ */
