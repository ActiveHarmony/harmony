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
#ifndef __HSERVER_H__
#define __HSERVER_H__

#include "hmesg.h"
#include "hpoint.h"
#include "httpsvr.h"
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct session_state {
    char *name;
    int fd;
    int *client;
    int client_len, client_cap;
    hpoint_t best;
    double best_perf;

    /* Fields used by the HTTP server. */
    struct timeval start;
    hsignature_t sig;
    http_log_t *log;
    int log_len, log_cap;
    int reported;
} session_state_t;

extern session_state_t *slist;
extern int slist_cap;

session_state_t *session_open(hmesg_t *mesg);
void session_close(session_state_t *sess);

#ifdef __cplusplus
}
#endif

#endif /* __HSERVER_H__ */
