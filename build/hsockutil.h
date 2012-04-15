/*
 * Copyright 2003-2011 Jeffrey K. Hollingsworth
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

/***
 *
 * Data communication helper functions.
 *
 ***/

#ifndef __HSOCKUTIL_H__
#define __HSOCKUTIL_H__

#include "hmesgs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Magic number for messages between the harmony server and its clients. */
#define HARMONY_MAGIC 0x5261793a

/**
 * Loop until all data has been writen to fd.
 **/
void socket_write(int fd, const void *data, unsigned datalen);

/**
 * send a message on the given socket
 **/
int send_message(int sock, const hmesg_t *);

/**
 * read a message from the given socket
 **/
int receive_message(int sock, hmesg_t *);

#ifdef __cplusplus
}
#endif

#endif /* ifndef _HSOCKUTIL_H__ */
