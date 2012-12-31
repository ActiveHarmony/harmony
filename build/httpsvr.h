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
#ifndef __HTTPSVR_H__
#define __HTTPSVR_H__

#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct http_log {
    struct timeval stamp;
    hpoint_t pt;
    double perf;
} http_log_t;

extern unsigned int http_connection_limit;

int http_init(const char *basedir);
void http_send_error(int fd, int status, const char *message);
int http_session_data_send(int fd, const char *data);
int handle_http_socket(int fd);

#ifdef __cplusplus
}
#endif

#endif
