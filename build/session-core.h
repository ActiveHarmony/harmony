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

#ifndef __SESSION_CORE_H__
#define __SESSION_CORE_H__

#include "hmesg.h"

#ifdef __cplusplus
extern "C" {
#endif

extern hsession_t *sess;

/* Requests a callback to func() when the socket fd has data available
 * to read.
 *
 * The supplied routine should return 0 if it is completely done
 * processing the message.  Otherwise, -1 should be returned with
 * mesg->status (and possibly errno) set appropriately.
 */
int callback_register(int fd, int (*func)(hmesg_t *));
int callback_unregister(int fd);

#ifdef __cplusplus
}
#endif

#endif
