/*
 * Copyright 2003-2016 Jeffrey K. Hollingsworth
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

typedef enum search_status {
    STATUS_CONVERGED = 0x1,
    STATUS_PAUSED    = 0x2
} search_status_t;

typedef struct http_log {
    struct timeval stamp;
    hpoint_t pt;
    double perf;
} http_log_t;

typedef struct sinfo {
    const char* name;
    int id;

    // Best known search point and performance.
    hpoint_t best;
    double best_perf;

    // List of connected clients.
    int* client;
    int client_len, client_cap;

    // Fields used by the HTTP server.
    struct timeval start;
    hspace_t space;
    char* strategy;
    unsigned int status;

    http_log_t* log;
    int log_len, log_cap;

    hpoint_t* fetched;
    int fetched_len, fetched_cap;
    int reported;
} sinfo_t;

extern sinfo_t* slist;
extern int slist_cap;

int search_open(void);
void search_close(sinfo_t* sinfo);
const char* search_getcfg(sinfo_t* sinfo, const char* key);
int search_setcfg(sinfo_t* sinfo, const char* key, const char* val);
int search_refresh(sinfo_t* sinfo);
int search_restart(sinfo_t* sinfo);

#ifdef __cplusplus
}
#endif

#endif // __HSERVER_H__
