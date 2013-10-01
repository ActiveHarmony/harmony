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

#include "hsockutil.h"
#include "defaults.h"
#include "hmesg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int tcp_connect(const char *host, int port)
{
    struct sockaddr_in addr;
    struct hostent *h_name;
    char *portenv;
    int sockfd;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        return -1;

    /* Look up the address associated with the supplied hostname
     * string.  If no hostname is provided, use the HARMONY_S_HOST
     * environment variable, if defined.  Otherwise, resort to
     * default.
     */
    if (host == NULL) {
        host = getenv("HARMONY_S_HOST");
        if (host == NULL)
            host = DEFAULT_HOST;
    }

    h_name = gethostbyname(host);
    if (!h_name)
        return -1;
    memcpy(&addr.sin_addr, h_name->h_addr_list[0], sizeof(struct in_addr));

    /* Prepare the port of connection.  If the supplied port is 0, use
     * the HARMONY_S_PORT environment variable, if defined.
     * Otherwise, resort to default.
     */
    if (port == 0) {
        portenv = getenv("HARMONY_S_PORT");
        if (portenv)
            port = atoi(portenv);
        else
            port = DEFAULT_PORT;
    }
    addr.sin_port = htons(port);

    /* try to connect to the server */
    addr.sin_family = AF_INET;
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        return -1;

    return sockfd;
}

/***
 *
 * Here we define some useful functions to handle data communication
 *
 ***/
int socket_write(int fd, const void *data, unsigned datalen)
{
    int retval;
    unsigned count;

    count = 0;
    do {
        retval = send(fd, ((char *)data) + count,
                      datalen - count, MSG_NOSIGNAL);
        if (retval < 0) {
            if (errno == EINTR) continue;
            else return -1;
        }
        else if (retval == 0)
            break;

        count += retval;
    } while (count < datalen);

    return count;
}

int socket_read(int fd, const void *data, unsigned datalen)
{
    int retval;
    unsigned count = 0;

    do {
        retval = recv(fd, ((char *)data) + count,
                      datalen - count, MSG_NOSIGNAL);
        if (retval < 0) {
            if (errno == EINTR) continue;
            else return -1;
        }
        else if (retval == 0)
            break;

        count += retval;
    } while (count < datalen);

    return count;
}

int socket_launch(const char *path, char *const argv[], pid_t *return_pid)
{
    int sockfd[2];
    pid_t pid;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) < 0)
        return -1;

    pid = fork();
    if (pid < 0)
        return -1;

    if (pid == 0) {
        /* Child Case */
        close(sockfd[0]);
        if (dup2(sockfd[1], STDIN_FILENO) != STDIN_FILENO)
            return -1;

        if (dup2(sockfd[1], STDOUT_FILENO) != STDOUT_FILENO)
            return -1;

        if (execv(path, argv) < 0) {
            perror("Could not launch child executable");
            exit(-1);  /* Be sure to exit here. */
        }
    }

    /* Parent continues here. */
    if (return_pid)
        *return_pid = pid;

    close(sockfd[1]);
    return sockfd[0];
}

/*
 * send a message to the given socket
 */
int mesg_send(int sock, hmesg_t *mesg)
{
    int msglen;

    msglen = hmesg_serialize(mesg);
    if (msglen < 0)
        return -1;

    /* fprintf(stderr, "(%2d)<<< '%s'\n", sock, mesg->buf); */
    if (socket_write(sock, mesg->buf, msglen) < msglen)
        return -1;

    hmesg_scrub(mesg);
    mesg->type = HMESG_UNKNOWN;
    return 1;
}

/*
 * receive a message from a given socket
 */
int mesg_recv(int sock, hmesg_t *mesg)
{
    char hdr[HMESG_HDRLEN + 1];
    char *newbuf;
    int msglen, retval;
    unsigned int msgver;

    retval = recv(sock, hdr, sizeof(hdr), MSG_PEEK);
    if (retval <  0) goto error;
    if (retval == 0) return 0;

    if (ntohl(*(unsigned int *)hdr) != HMESG_MAGIC)
        goto invalid;

    hdr[HMESG_HDRLEN] = '\0';
    if (sscanf(hdr + sizeof(int), "%4d%2x", &msglen, &msgver) < 2)
        goto invalid;

    if (msgver != HMESG_VERSION)
        goto invalid;

    if (mesg->buflen <= msglen) {
        newbuf = (char *) realloc(mesg->buf, msglen + 1);
        if (!newbuf)
            goto error;
        mesg->buf = newbuf;
        mesg->buflen = msglen;
    }

    retval = socket_read(sock, mesg->buf, msglen);
    if (retval == 0) return 0;
    if (retval < msglen) goto error;
    mesg->buf[retval] = '\0';  /* A strlen() safety net. */
    /* fprintf(stderr, "(%2d)>>> '%s'\n", sock, mesg->buf); */

    hmesg_scrub(mesg);
    if (hmesg_deserialize(mesg) < 0)
        goto error;

    return 1;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}
