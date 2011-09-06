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
 * include user defined header
 */
#include <errno.h>
#include "hsockutil.h"

using namespace std;

/***
 *
 * Here we define some useful functions to handle data communication
 *
 ***/
void socket_write(int fd, const void *data, unsigned datalen)
{
    unsigned count = 0;
    int retval;

    do {
        errno = 0;
        retval = write(fd, data, datalen);
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

void send_message(HMessage *m, int sock) {
    void *buf;
    int buflen;
    unsigned int preamble[2];

    buf=malloc(m->get_message_size());
    if (buf==NULL)
        h_exit("Could not malloc!");

    buflen=m->serialize((char *)buf);

    if (buflen!=m->get_message_size()) {
        printf(" ( %d vs. %d )", buflen, m->get_message_size());
        h_exit("Error serializing message!");
    }


    preamble[0] = htonl(HARMONY_MAGIC);
    preamble[1] = htonl(buflen);
    // first send the message length and then the actual message
    // helps when data is available on the socket. (at read!)

    if (send(sock, (char *)preamble, sizeof(preamble), 0)<0) {
        h_exit("Failed to send size of message!");
    }


    if (send(sock,(char *)buf, m->get_message_size(), 0)<0) {
        h_exit("Failed to send message!");
    }

    free(buf);

}


int get_message_type(void *buf) {
    return ntohl(*(unsigned long int*)buf);
}


/*
 * receive a message from a given socket
 */
HMessage * receive_message(int sock) {
    void *buf;
    int buflen;
    HMessage *m;
    int buflend;
    unsigned int preamble[2];

    // first read the size of the size of the message, allocate the space and
    // then the message
    
    if ((buflen=recvfrom(sock, (char *)preamble, sizeof(preamble),0, NULL, NULL))<0) {
        h_exit("Failed to receive message size!");
    }

    if (buflen == 0) {
        /* Client closed socket */
        return NULL;
    }

    preamble[0] = ntohl(preamble[0]);
    if (preamble[0] != HARMONY_MAGIC) {
        printf("Expected 0x%x, got 0x%x\n", HARMONY_MAGIC, preamble[0]);
        h_exit("Received malformed harmony message.");
    }

    buf=malloc(ntohl(preamble[1]));
    if (buf==NULL) {
        printf("Recv:%d\n",preamble[1]);
        h_exit("Could not malloc!");
    }

    /* modified by I-Hsin Nov.11 '02 to fix the packet fragmentation  */
    //printf("Got the message size %d sock: %d \n", buflen, sock);
    char *p;
    int i=ntohl(preamble[1]);
    p=(char *)buf;

    while(i>0) {
        if((buflen=recvfrom(sock,(char *)p, i,0, NULL, NULL))<0)
            h_exit("Failed to receive message!");
        i-=buflen;
        p+=buflen;
        //printf("stuck here \n");
    }

    switch (get_message_type(buf)) {
    case HMESG_APP_DESCR:
    case HMESG_NODE_DESCR:
    case HMESG_VAR_DESCR:
        m=new HDescrMessage();
        buflend = ((HDescrMessage *)m)->deserialize((char *)buf);
        break;
    case HMESG_DAEMON_REG:
    case HMESG_CLIENT_REG:
    case HMESG_CLIENT_UNREG:
    case HMESG_CONFIRM:
    case HMESG_FAIL:
        m=new HRegistMessage();
        buflend = ((HRegistMessage *)m)->deserialize((char *)buf);
        break;
    case HMESG_VAR_REQ:
    case HMESG_VAR_SET:
    case HMESG_PERF_UPDT:
    case HMESG_PROBE_REQ:
    case HMESG_TCLVAR_REQ:
    case HMESG_TCLVAR_REQ_2:
    case HMESG_PROBE_SET:
    case HMESG_DATABASE:
    case HMESG_WITH_CONF:
    case HMESG_CODE_COMPLETION:
    case HMESG_PERF_ALREADY_EVALUATED:
        //printf("The message type is: %d \n \n",get_message_type(buf) );
        m=new HUpdateMessage();
        buflend = ((HUpdateMessage *)m)->deserialize((char *)buf);
        break;
    default:
        h_exit("Wrong message type!\n");
    }

    return m;
}
