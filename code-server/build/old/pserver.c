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
#include "hserver.h"
#include "StringTokenizer.h"
#include <sys/wait.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include "generator_parent.c"
//#include "utilities.h"
using namespace std;

/*
 * the socket waiting for connections
 */
int listen_socket;

#ifdef LINUX
struct in_addr listen_addr;	/* Address on which the server listens. */
#endif

#define TRUE 1
#define FALSE 0

// some ANN declarations
int maxpts;
double eps = 0;

/*
 * the port waiting for connections
 */
int listen_port;
fd_set listen_set;
list<int> socket_set;
int highest_socket;
list<int>::iterator socketIterator;

list<int> id_set;

/* the X file descriptor */
int xfd;


/*
  the map containing client AppName and Socket
*/
map<string, int> clientInfo;
map<int, char *> clientName;
map<int, int> clientSignals;
map<int, int> clientSocket;
map<int, int> clientId;

/***
 *
 * Check the arguments given to the server
 *
 ***/
void check_parameters(int argc, char **argv) {

}
void sighup_parent(int signumber)
     
{  signal(SIGHUP,sighup); /* reset signal */
   printf("PARENT: I have received a SIGHUP\n");
}

void sigint_parent(int signumber)

{  signal(SIGINT,sigint); /* reset signal */
   printf("PARENT: I have received a SIGINT\n");
   usr_interrupt=1;
   //exit(0);
}

void sigquit_parent(int signumber)

{ printf("My DADDY has Killed me!!!\n");
  exit(0);
}

/***
 * This function is called each time a operation requested by the client fails
 * It will return a FAIL message to the client
 ***/
void operation_failed(int sock) {
  HRegistMessage *m = new HRegistMessage(HMESG_FAIL,0);
  send_message(m,sock);
  delete m;
}

int update_sent = FALSE;
char *application_trigger = "";


/***
 *
 * Start the server by initializing sockets and everything else that is needed
 *
 ***/

void server_startup() {
    /* temporary variable */
    char s[10];

    for(int i = 1; i <= 100000; i++)
        id_set.push_back(i);

    /* used address */
    struct sockaddr_in sin;
    int optVal;
    int err;

    listen_port = SERVER_PORT;

    /* create a listening socket */
    if ((listen_socket = socket (AF_INET, SOCK_STREAM,0)) < 0) {
        h_exit("Error initializing socket!");
    }

    /* set socket options. We try to make the port reusable and have it
       close as fast as possible without waiting in unnecessary wait states
       on close. */
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &optVal, sizeof(optVal))) {
        h_exit("Error setting socket options!");
    }

    /* Initialize the socket address. */
    sin.sin_family = AF_INET;
#ifdef LINUX
    sin.sin_addr = listen_addr;
#else
    unsigned int listen_addr = INADDR_ANY;
    sin.sin_addr = *(struct  in_addr*)&listen_addr;
#endif /*LINUX*/
    sin.sin_port = htons(listen_port);

    /* Bind the socket to the desired port. */
    if (bind(listen_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        h_exit("Error binding socket!");
    }

    /*
      Set the file descriptor set
    */
    FD_ZERO(&listen_set);
    FD_SET(listen_socket,&listen_set);
    socket_set.push_back(listen_socket);
    highest_socket=listen_socket;
    FD_SET(0, &listen_set);
}


/***
 *
 * Client registration adds the client information into the database
 *
 ***/
void client_registration(HRegistMessage *m, int client_socket){

    printf("Client registration! (%d)\n", client_socket);

    /* store information about the client into the local database */

    FD_SET(client_socket,&listen_set);
    socket_set.push_back(client_socket);
    if (client_socket>highest_socket)
        highest_socket=client_socket;

    clientSignals[client_socket]=m->get_param();
    int id = m->get_id();

    printf("id :: %d \n", id);

    if (id) {
        /* the client is relocating its communicating agent */
        /* the server dissacociates the old socket with the client, and
         * uses the new one instead */
        int cs = clientSocket[id];
        clientSocket[id] = client_socket;
        char *appName = clientName[cs];
        printf("appName:: %s \n", appName);
        clientInfo[appName]=client_socket;
        clientName.erase(cs);
        clientName[client_socket] = appName;

        FD_CLR(cs, &listen_set);
        if (cs==*socketIterator)
            socketIterator++;
        socket_set.remove(cs);
        if (highest_socket==cs)
            highest_socket--;
    } else {
        /* generate id for the new client */
        id = id_set.front();
        id_set.pop_front();
        clientSocket[id] = client_socket;
    }
    clientId[client_socket] = id;
    printf("Send registration (id %d) confimation !\n", id);

    /* send the confirmation message with the client_id */
    HRegistMessage *mesg=new HRegistMessage(HMESG_CLIENT_REG,client_socket);
    mesg->set_id(id);
    send_message(mesg, client_socket);
    delete mesg;
}


/****
     Name:         revoke_resource
     Description:  calls scheduler to revoke allocated resources,
     remove tcl names related to the disrupted application, and
     start a new scheduling round
     Called from:  cilent_unregister
****/
void revoke_resources(char *appName) {
    fprintf(stderr, "revoke_resources: %s\n", appName);
    free(clientName[clientInfo[appName]]);
}

/***
 *
 * The client unregisters! Send back a confirmation message
 *
 ***/
void client_unregister(HRegistMessage *m, int client_socket) {

    /* clear all the information related to the client */

    printf("ENTERED CLIENT UNREGISTER!!!!!\n");

    FD_CLR(client_socket, &listen_set);
    if (client_socket==*socketIterator)
        socketIterator++;
    socket_set.remove(client_socket);
    if (highest_socket==client_socket)
        highest_socket--;
    clientSocket.erase(clientId[client_socket]);
    clientId.erase(client_socket);

    if (m == NULL){
        /* revoke resources */
        revoke_resources(clientName[client_socket]);
    } else {
        printf("Send confimation !\n");
        HRegistMessage *mesg = new HRegistMessage(HMESG_CONFIRM,0);

        /* send back the received message */
        send_message(mesg,client_socket);
        delete mesg;
    }

    fprintf(stderr, "Exit UNREGISTER!\n");
}

int in_process_message = FALSE;
int generator_pid=0;
void process_message_from(int temp_socket){

    HMessage *m;
    assert(!in_process_message);
    in_process_message = TRUE;
    printf("Waiting...%d...%d...\n",getpid(),temp_socket);

    /* get the message type */
    m=receive_message(temp_socket);

    if (m == NULL){
        client_unregister((HRegistMessage*)m, temp_socket);
        in_process_message = FALSE;
        return;
    }

    printf("Message has type: %d\n", m->get_type());

    switch (m->get_type()) {
        case HMESG_CLIENT_REG:
            client_registration((HRegistMessage *)m,temp_socket);
            break;
        case HMESG_CLIENT_UNREG:
            client_unregister((HRegistMessage *)m, temp_socket);
            break;
        case HMESG_PROJECTION:
	  printf("received projection message \n");
	   kill(generator_pid, SIGHUP);
	  //projection_is_valid((HDescrMessage*)m, temp_socket);
	    HRegistMessage *m = new HRegistMessage(HMESG_CONFIRM,1);
	    send_message(m,temp_socket);
	    delete m;
            break;
        case HMESG_SIM_CONS_REQ:
	  //kill(generator_pid, SIGHUP);
	  //simplex_construction((HDescrMessage*)m, temp_socket);
            break;
        case HMESG_PROJ_REQ:
	   kill(generator_pid, SIGHUP);	
	  //do_projection((HDescrMessage*)m, temp_socket);
            break;
        default:
            printf("Wrong message type!\n");
    }
    in_process_message = FALSE;
}


/***
 *
 * The main loop of the server
 *
 * We only exit on Ctrl-C
 *
 ***/
void main_loop(int generator_pid) {
	
    struct sockaddr_in temp_addr;
    int temp_addrlen;
    int temp_socket;
    int pid;
    int active_sockets;
    fd_set listen_set_copy;
    kill(generator_pid, SIGHUP);
    /* do listening to the socket */
    if (listen(listen_socket, 10)) {
        h_exit("Listen to socket failed!");
    }
    while (1) {
        listen_set_copy=listen_set;
        //    printf("Select\n");
        active_sockets=select(highest_socket+1,&listen_set_copy, NULL, NULL, NULL);

        printf("We have %d requests\n",active_sockets);

        /* we have a communication request */
        for (socketIterator=socket_set.begin();socketIterator!=socket_set.end();socketIterator++) {
            if (FD_ISSET(*socketIterator, &listen_set_copy) && (*socketIterator!=listen_socket) && (*socketIterator!=xfd)) {
                /* we have data from this connection */
                process_message_from(*socketIterator);
            }
        }
        if (FD_ISSET(listen_socket, &listen_set_copy)) {
            /* we have a connection request */

            if ((temp_socket = accept(listen_socket,(struct sockaddr *)&temp_addr, (unsigned int *)&temp_addrlen))<0) {
                //	h_exit("Accepting connections failed!");
            }
            fprintf(stderr, "accepted a client\n");
            process_message_from(temp_socket);
        }
        if (FD_ISSET(xfd, &listen_set_copy)) {
            char line[1024];
            int ret1 = read(0, line, sizeof(line));
            assert (ret1 >= 0);
            line[ret1] = '\0';
            fflush(stdout);
        }
    }
}



int fork_and_launch()
{
  int pid;
  int ppid=getpid();

  /* get child process */
  
   if ((pid = fork()) < 0) {
        perror("fork");
        exit(1);
    }
    
   if (pid == 0)
     { /* child */
       //int pid_2=fork();
       //signal(SIGHUP,sighup); /* set function calls */
       //signal(SIGINT,sigint);
       //signal(SIGQUIT, sigquit);
       printf("this is the child? \n");
       child_main(ppid);
     } else {
     // parent returns
     return pid;
   }
   return pid;
}


/***
 *
 * This is the main program.
 * It first checks the arguments.
 * It then initializes and goes for the main loop.
 *
 ***/
int main(int argc, char **argv) {

    check_parameters(argc, argv);
    

    /* do all the connection related stuff */
    server_startup();

    /*
      fork and launch the generator
     */
    generator_pid=fork_and_launch();
    
    /*
    signal(SIGHUP,sighup_parent);	
    signal(SIGINT,sigint_parent);	
    signal(SIGQUIT, sigquit_parent);
    */
    main_loop(generator_pid);
}

