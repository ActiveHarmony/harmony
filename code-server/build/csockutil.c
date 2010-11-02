/*
 * include user defined header
 */
#include "csockutil.h"

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
      case CMESG_CODE_GENERATION_REQ:
	// printf("case: HMESG_VAR_DESCR");
	m=new HDescrMessage();
	buflend = ((HDescrMessage *)m)->deserialize((char *)buf);
	break;
      case CMESG_CLIENT_REG:
      case CMESG_CLIENT_UNREG:
      case CMESG_CONFIRM:
      case CMESG_FAIL:
      case CMESG_CODE_GENERATION_REQ_CONFIRM:
      case CMESG_CODE_COMPLETE_QUERY:
      case CMESG_CODE_COMPLETE_RESPONSE:
	m=new HRegistMessage();
	buflend = ((HRegistMessage *)m)->deserialize((char *)buf);
	break;
      default:
	//printf("Message type: %d\n", get_message_type(buf));
	h_exit("Wrong message type!\n");
    }

    return m;
}
