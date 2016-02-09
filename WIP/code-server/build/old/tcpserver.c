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

//TCP Concurrent Server using fork()
 	
 	
 //Includes
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <sys/wait.h>
#include <signal.h>
//Function Prototypes
void myabort(char *);
using namespace std;

void sighup(int signumber); /* routines child will call upon sigtrap */
void sigint(int signumber);
void sigquit(int signumber);

sigset_t       signalSet;

//Some Global Variables
int serverport = 2001;
char * eptr = NULL;
int listen_socket, client_socket;
struct sockaddr_in Server_Address, Client_Address;
int result;
socklen_t csize;
pid_t processid;
int childcount = 0;

void handle_client_req(){
  int    sig;
  int    err;
    

  //  err = sigwait ( &signalSet, &sig );

  //sigwait(SIGHUP);
  printf("got here now going to sleep\n");
  for(int i=0; i<2; i++)
    for(int j=0; j<2; j++)
      for(int k=0; k<2; k++)
	for(int l=0; l<1000; l++){
	  if(l%502==0)
	    printf("current l %d\n", l);
	}
  err = sigwait ( &signalSet, &sig );
  printf("waking up signal: %d \n", sig);
}

//main()
int
main(int argc, char **argv){
  
  char buf[100];
  char tmp[100];
  char * ptr;
  int n, sent, length;
  
  //Step 0: Process Command Line
  if (argc > 2){
    myabort("Usage: server <port>");
  }
  if (argc == 2){
    serverport =  (int) strtol(argv[1], &eptr, 10);
    if (*eptr != '\0') myabort("Invalid Port Number!");
  }
  
  
  //Step 1: Create a socket
  listen_socket = socket(PF_INET, SOCK_STREAM, 0);
  if (listen_socket == -1) myabort("socket()");
  
  
  //Step 2: Setup Address structure
  bzero(&Server_Address, sizeof(Server_Address));
  Server_Address.sin_family = AF_INET;
  Server_Address.sin_port = htons(serverport);
  Server_Address.sin_addr.s_addr = INADDR_ANY;
  
  
  //Step 3: Bind the socket to the port
  result = bind(listen_socket, (struct sockaddr *) &Server_Address, sizeof(Server_Address));
  if (result == -1) myabort("bind()");
  
  
  //Step 4:Listen to the socket
  result = listen(listen_socket, 1);
  if (result == -1) myabort("listen()");
  
  
  //Step 5: Setup an infinite loop to make connections
  while(1){
    
    
    //Accept a Connection
    csize = sizeof(Client_Address);
    client_socket = accept( listen_socket,
			    (struct sockaddr *) &Client_Address,
			    &csize
			    );
    if (client_socket == -1) myabort("accept()");
    
    cout << "Client Accepted!" << endl;
    
    
    
    //fork this process into a child and parent
    processid = fork();
    
    //Check the return value given by fork(), if negative then error,
    //if 0 then it is the child.
    if ( processid == -1){
      myabort("fork()");
    }else if (processid == 0){
      /*Child Process*/
      
      close(listen_socket);
      //loop until client closes
      while (1){
	signal(SIGHUP,sighup); /* set function calls */
       signal(SIGINT,sigint);
       signal(SIGQUIT, sigquit);
       sigfillset ( &signalSet );

	// handle client
	handle_client_req();
	
	//read string from client
	bzero(&buf, sizeof(buf));
	do{
	  bzero(&tmp, sizeof(tmp));
	  n = read(client_socket,(char *) &tmp, 100);
	  //cout << "server:  " << tmp;
	  tmp[n] = '\0';
	  if (n == -1) myabort("read()");
	  if (n == 0) break;
	  strncat(buf, tmp, n-1);
	  buf[n-1] = ' ';
	} while (tmp[n-1] != '\n');
	
	buf[ strlen(buf) ] = '\n';
	
	
	
	if (n == 0) break;
	
	
	//write string back to client
	sent = 0;
	ptr = buf;
	length = strlen(buf);
	while (sent < length ){
	  n = write(client_socket, ptr, strlen(ptr) );
	  if ( n == -1) myabort("write()");
	  sent += n;
	  ptr += n;
	}
      }//end inner while
      
      close(client_socket);
      
      //Child exits
      exit(0);
    }
    
    
    //Parent Process
     kill(processid,SIGHUP);
    cout << "Child process spawned with id number:  " << processid << endl;
    //increment the number of children processes
    childcount++;
    while(childcount){
      processid = waitpid( (pid_t) - 1, NULL, WNOHANG );
      if (processid < 0) myabort("waitpid()");
      else if (processid == 0) break;
      else childcount--;
    }	
    
    
  }
  close(listen_socket);
  
  exit(0);
  
}


void
myabort(char * msg){
  cout << "Error!:  " <<  msg << endl;
  exit(1);
}


void sighup(int signumber)
     
{  signal(SIGHUP,sighup); /* reset signal */
   printf("CHILD: I have received a SIGHUP\n");
}

void sigint(int signumber)

{  signal(SIGINT,sigint); /* reset signal */
   printf("CHILD: I have received a SIGINT\n");
   exit(0);
}

void sigquit(int signumber)

{ printf("My DADDY has Killed me!!!\n");
  exit(0);
}
