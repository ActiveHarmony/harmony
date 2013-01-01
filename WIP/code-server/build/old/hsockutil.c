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

/*
 * include user defined header
 */
#include "hsockutil.h"

using namespace std;

/***
 *
 * Here we define some useful functions to handle data communication
 *
 ***/

/*
 * send a message to the given socket
 */

void send_message(HMessage *m, int sock) {
    void *buf;
    int buflen;
    unsigned nbuflen;

    // printf("Flag: %s \n", m->get_flag());

    //m->print();

    //printf("Sending message type %d to socket %d\n",m->get_type(),sock);
    buf=malloc(m->get_message_size());
    if (buf==NULL)
        h_exit("Could not malloc!");

    //  printf("Alloc ok!\n");

    buflen=m->serialize((char *)buf);

    //  printf("Serialize ok!\n");

    if (buflen!=m->get_message_size()) {
        printf(" ( %d vs. %d )", buflen, m->get_message_size());
        h_exit("Error serializing message!");
    }


    nbuflen = htonl(buflen);
    // first send the message length and then the actual message
    // helps when data is available on the socket. (at read!)

    if (send(sock, (char *)&nbuflen, sizeof(nbuflen), 0)<0) {
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
    unsigned nbuflen;


    // first read the size of the size of the message, allocate the space and
    // then the message

    //printf("Inside receive_message(int sock) %d\n", sock);
    
    if ((buflen=recvfrom(sock, (char *)&nbuflen, sizeof(nbuflen),0, NULL, NULL))<0) {
        h_exit("Failed to receive message size!");
    }

    buf=malloc(ntohl(nbuflen));
    if (buf==NULL) {
        printf("Recv:%d\n",nbuflen);
        h_exit("Could not malloc!");
    }

/* modified by I-Hsin Nov.11 '02 to fix the packet fragmentation  */

    char *p;
    int i=ntohl(nbuflen);
    p=(char *)buf;

    while(i>0) {
        if((buflen=recvfrom(sock,(char *)p, i,0, NULL, NULL))<0)
            h_exit("Failed to receive message!");
        i-=buflen;
        p+=buflen;
    }

/*
   if ((buflen=recvfrom(sock,(char *) buf, ntohl(nbuflen),0, NULL, NULL))<0) {
   h_exit("Failed to receive message!");
   }
*/

    //printf("Received message type: %d from %d\n",get_message_type(buf),sock);
    //printf("checkpoint____00 \n");
    
    switch (get_message_type(buf)) {
        case HMESG_APP_DESCR:
        case HMESG_NODE_DESCR:
        case HMESG_VAR_DESCR:
        case HMESG_PROJECTION:
        case HMESG_SIM_CONS_REQ:
        case HMESG_SIM_CONS_RESULT:
        case HMESG_PROJ_REQ:
        case HMESG_PROJ_RESULT:
            // printf("case: HMESG_VAR_DESCR");
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
        case HMESG_PROBE_SET:
        case HMESG_DATABASE:
        case HMESG_WITH_CONF:
            // printf("Case: HMESG_PERF_UPDT \n");
            m=new HUpdateMessage();
            //printf("checkpoint 1 \n");
            buflend = ((HUpdateMessage *)m)->deserialize((char *)buf);
            //printf("checkpoint 2 \n");
            break;
        default:
            //printf("Message type: %d\n", get_message_type(buf));
            h_exit("Wrong message type!\n");
    }

    return m;
}
