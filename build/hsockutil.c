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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "hsockutil.h"
#include "hmesgs.h"

/***
 *
 * Here we define some useful functions to handle data communication
 *
 ***/
void socket_write(int fd, const void *data, unsigned datalen)
{
    int retval;
    unsigned count = 0;

    do {
        errno = 0;
        retval = write(fd, ((char *)data) + count, datalen - count);
        if (retval <= 0 && errno != EINTR)
            break;
        /*printf("Wrote: %.*s\n", retval, data);*/
        count += retval;
    } while (count < datalen);

    if (retval <= 0 && errno != 0) {
        printf("[AH]: Socket write error: %s.\n", strerror(errno));
    }
}

/*
 * send a message to the given socket
 */
int send_message(int sock, const hmesg_t *mesg)
{
    static const unsigned int hdrlen = sizeof(unsigned int);
    char buf[4096];
    int buflen = hdrlen;

    *(unsigned int *)buf = htonl(HARMONY_MAGIC);
    buflen += hmesg_serialize(buf + hdrlen, sizeof(buf) - hdrlen, mesg);
    if (buflen < hdrlen)
        return -1;
    assert(buflen == strlen(buf));

    /*fprintf(stderr, "SEND: '%.*s'\n", buflen, buf);*/
    if (sendto(sock, buf, buflen, 0, NULL, 0) < 0)
        printf("* Warning: sendto failure.\n");

    return 0;
}

/*
 * receive a message from a given socket
 */
int receive_message(int sock, hmesg_t *mesg)
{
    static const unsigned int hdrlen = sizeof(unsigned int);
    char buf[4096];
    int buflen;

    buflen = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);
    buf[buflen] = '\0';
    /*fprintf(stderr, "RECV: '%s'\n", buf);*/
    if (buflen == 0) {
        /* Client closed socket */
        return 0;
    }
    else if (buflen < 0) {
        fprintf(stderr, "Failed to receive message.\n");
        return -1;
    }
    else if (ntohl(*(unsigned int *)buf) != HARMONY_MAGIC) {
        fprintf(stderr, "Expected 0x%x, got 0x%x\n", HARMONY_MAGIC,
                ntohl(*(unsigned int *)buf));
        return -1;
    }

    if (hmesg_deserialize(mesg, buf + hdrlen) < 0) {
        fprintf(stderr, "Error deserializing message.\n");
        return -1;
    }

    return 0;
}
