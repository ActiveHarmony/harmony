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

#if defined(SO_NOSIGPIPE)
void init_socket(int sockfd)
{
    static const int set = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(int));
}
#endif

int tcp_connect(const char* host, int port)
{
    struct sockaddr_in addr;
    struct hostent* h_name;
    int sockfd;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        return -1;

    h_name = gethostbyname(host);
    if (!h_name)
        return -1;
    memcpy(&addr.sin_addr, h_name->h_addr_list[0], sizeof(struct in_addr));
    addr.sin_port = htons((unsigned short)port);

    // Try to connect to the server.
    addr.sin_family = AF_INET;
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        return -1;

#if defined(SO_NOSIGPIPE)
    init_socket(sockfd);
#endif

    return sockfd;
}

/***
 *
 * Here we define some useful functions to handle data communication
 *
 ***/
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0x0
#endif

int socket_write(int fd, const void* data, unsigned len)
{
    int retval;
    unsigned count;

    count = 0;
    do {
        retval = send(fd, ((char*)data) + count, len - count, MSG_NOSIGNAL);
        if (retval < 0) {
            if (errno == EINTR) continue;
            else return -1;
        }
        else if (retval == 0)
            break;

        count += retval;
    } while (count < len);

    return count;
}

int socket_read(int fd, const void* data, unsigned datalen)
{
    int retval;
    unsigned count = 0;

    do {
        retval = recv(fd, ((char*)data) + count,
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

int socket_launch(const char* path, char* const argv[], pid_t* return_pid)
{
    int sockfd[2];
    pid_t pid;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) < 0)
        return -1;

    pid = fork();
    if (pid < 0)
        return -1;

    if (pid == 0) {
        // Child Case.
        close(sockfd[0]);
        if (dup2(sockfd[1], STDIN_FILENO) != STDIN_FILENO)
            perror("Could not duplicate socket onto child STDIN");

        if (dup2(sockfd[1], STDOUT_FILENO) != STDOUT_FILENO)
            perror("Could not duplicate socket onto child STDOUT");

        if (execv(path, argv) < 0)
            perror("Could not launch child executable");

        exit(-1); // Be sure to exit here.
    }

    // Parent continues here.
    if (return_pid)
        *return_pid = pid;

#if defined(SO_NOSIGPIPE)
    init_socket(sockfd[0]);
#endif

    close(sockfd[1]);
    return sockfd[0];
}

/*
 * send a message to the given socket
 */
int mesg_send(int sock, hmesg_t* mesg)
{
    int msglen = hmesg_pack(mesg);
    if (msglen < 0)
        return -1;

    // fprintf(stderr, "(%2d)<<< '%s'\n", sock, mesg->send_buf);
    if (socket_write(sock, mesg->send_buf, msglen) < msglen)
        return -1;

    return 1;
}

/*
 * Forward a message to the given socket.
 *
 * If no changes were made to an hmesg_t after it was unpacked, the
 * original payload may be forwarded to a different destination by
 * replacing null bytes with the string delimiting character (").
 */
int mesg_forward(int sock, hmesg_t* mesg)
{
    if (mesg->origin < -1 || mesg->origin >= 255) {
        fprintf(stderr, "Error: mesg_forward():"
                "Origin (%d) is out of range [-1, 254]\n", mesg->origin);
        return -1;
    }

    int msglen;
    if (sscanf(mesg->recv_buf + HMESG_LENGTH_OFFSET, "%4d", &msglen) < 1)
        return -1;

    for (int i = 0; i < msglen; ++i) {
        if (mesg->recv_buf[i] == '\0')
            mesg->recv_buf[i] = '"';
    }

    char origin[HMESG_ORIGIN_SIZE + 1];
    snprintf(origin, sizeof(origin), "%02x", mesg->origin);
    memcpy(mesg->recv_buf + HMESG_ORIGIN_OFFSET, origin, HMESG_ORIGIN_SIZE);

    // fprintf(stderr, "(%2d)<<< '%s'\n", sock, mesg->recv_buf);
    if (socket_write(sock, mesg->recv_buf, msglen) < msglen)
        return -1;

    return 1;
}

/*
 * receive a message from a given socket
 */
int mesg_recv(int sock, hmesg_t* mesg)
{
    char hdr[HMESG_HDR_SIZE + 1];
    char* newbuf;
    int msglen, retval;
    unsigned int magic, msgver;

    retval = recv(sock, hdr, HMESG_HDR_SIZE, MSG_PEEK);
    if (retval <  0) goto error;
    if (retval == 0) return 0;

    memcpy(&magic, hdr, HMESG_MAGIC_SIZE);
    if (ntohl(magic) != HMESG_MAGIC)
        goto invalid;

    hdr[HMESG_HDR_SIZE] = '\0';
    if (sscanf(hdr + HMESG_LENGTH_OFFSET, "%4d%2x", &msglen, &msgver) < 2)
        goto invalid;

    if (msgver != HMESG_VERSION)
        goto invalid;

    if (mesg->recv_len <= msglen) {
        newbuf = realloc(mesg->recv_buf, msglen + 1);
        if (!newbuf)
            goto error;
        mesg->recv_buf = newbuf;
        mesg->recv_len = msglen + 1;
    }

    retval = socket_read(sock, mesg->recv_buf, msglen);
    if (retval == 0) return 0;
    if (retval < msglen) goto error;
    mesg->recv_buf[retval] = '\0'; // A strlen() safety net.
    // fprintf(stderr, "(%2d)>>> '%s'\n", sock, mesg->recv_buf);

    if (hmesg_unpack(mesg) < 0)
        goto error;

    return 1;

  invalid:
    errno = EINVAL;
  error:
    return -1;
}
