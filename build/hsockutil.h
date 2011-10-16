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

/*
 * include system headers
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>

/*
 * include user headers
 */
#include "hmesgs.h"

using namespace std;

#ifndef __HSOCKUTIL_H__
#define __HSOCKUTIL_H__


/* the maximum buffer size for a message is 64K */
#define MAX_BUFFER_SIZE 65535

/* Magic number for messages between the harmony server and its clients. */
#define HARMONY_MAGIC 0x001DEA00

/***
 *
 * Here we define some useful functions to handle data communication
 *
 ***/

/**
 * Loop until all data has been writen to fd.
 **/
void socket_write(int fd, const void *data, unsigned datalen);

/**
 * send a message on the given socket
 **/
void send_message(HMessage *, int sock);

/**
 * read a message from the given socket
 **/
HMessage *receive_message(int sock);

#endif /* ifndef _HSOCKUTIL_H__ */
