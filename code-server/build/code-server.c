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

/* Ananta Tiwari
 *   University of Maryland at College Park
 * Design:
 *     code-server : receives and sends messages to the harmony server
 *		   : populates the primary and secondary configurations
 *     generator-manager : forked by code-server
 *			 : changes the done-flag to notify the code-server 
 * 			  that code-generation for primary configurations is done
 *     generators: forked by generator-manager to generate code using 
 *		   some external code generation tools. these die after
 *		   generating code for one configuration
 *                 Interface with the code-generation utility via a script file.
 *                 Once the code-generation utility's API is available, the script
 *                 can be replaced with API calls.
*/

/* includes */
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdlib.h>
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

#include "code-server.h"
#include "Tokenizer.h"
#include "generator_manager.c"

using namespace std;

/*
 * the socket waiting for connections
 */
int listen_socket;

// keeping track of the pid of the launching process
int code_server_pid=-1;
int code_manager_pid=-1;

#ifdef LINUX
struct in_addr listen_addr;	/* Address on which the server listens. */
#endif

// macros
#define TRUE 1
#define FALSE 0

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
  /* not implemented yet. this can be used to select the kernel(s) 
   *  that we generate code for.
   */
}

/*
 * locks the semaphore, for exclusive access to a resource.
 */
void sem_lock(int sem_set_id)
{
    /* structure for semaphore operations.   */
    struct sembuf sem_op;

    /* wait on the semaphore, unless it's value is non-negative. */
    sem_op.sem_num = 0;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
    semop(sem_set_id, &sem_op, 1);
}

/*
 * sem_unlock. un-locks the semaphore.
 */
void sem_unlock(int sem_set_id)
{
    /* structure for semaphore operations.   */
    struct sembuf sem_op;

    sem_op.sem_num = 0;
    sem_op.sem_op = 1;
    sem_op.sem_flg = 0;
    semop(sem_set_id, &sem_op, 1);
}

/* setup the semaphore 
 *  currently using just one semaphore to control all three shared
 *  memory segments. Need to perform some testing for deadlocks.
*/
int setup_semaphore()
{
  union semun sem_val; 
  /* create a semaphore set with id SEM_ID, with one semaphore   */
  /* in it, with access only to the owner.                    */
  sem_id = semget(SEM_ID, 1, IPC_CREAT | 0600);
  if (sem_id == -1) {
    perror("main: semget");
    exit(1);
  }

  /* intialize the first (and single) semaphore in our set to '1'. */
  sem_val.val = 1;
  int rc = semctl(sem_id, 0, SETVAL, sem_val);
  if (rc == -1) {
    perror("main: semctl");
    exit(1);
  }

  return rc;
}

/*
 *    setup shared memory segments: one each for primary confs,
 *      secondary confs and done indicator.
*/
void setup_shared_buffers() {
  // prmary buffer 
  shm_id_primary=shmget(IPC_PRIVATE, sizeof(struct primary_buf), 0666 | IPC_CREAT | IPC_EXCL);
  if(shm_id_primary == -1) {
    perror("primary: shmget failed\n");
    exit(-1);
  }

  primary_confs=shmat(shm_id_primary, (void*)0,0);
  if(primary_confs==(void*)-1){
    perror("primary: shmat failed\n");
    exit(-1);
  }	
  
  // secondary buffer
  shm_id_secondary=shmget(IPC_PRIVATE, sizeof(struct secondary_buf), 0666 | IPC_CREAT | IPC_EXCL);
  if(shm_id_secondary == -1) {
    perror("secondary: shmget failed\n");
    exit(-1);
  }

  secondary_confs=shmat(shm_id_secondary, (void*)0,0);
  if(secondary_confs==(void*)-1){
    perror("secondary: shmat failed\n");
    exit(-1);
  }	
    
  // done buffer
  shm_id_done=shmget(IPC_PRIVATE, sizeof(struct done_buf), 0666 | IPC_CREAT | IPC_EXCL);
  if(shm_id_done == -1) {
    perror("done: shmget failed\n");
    exit(-1);
  }

  done_indicator=shmat(shm_id_done, (void*)0,0);
  if(done_indicator==(void*)-1){
    perror("done: shmat failed\n");
    exit(-1);
  }	
  
  // initialize the shared memory segments
  memset(primary_confs, 0, sizeof(struct primary_buf));
  memset(secondary_confs, 0, sizeof(struct secondary_buf));	
  memset(done_indicator, 0, sizeof(struct done_buf));

}

// deprecated
struct secondary_buf * setup_secondary_buffer() {
  shm_id_secondary=shmget(IPC_PRIVATE, sizeof(struct secondary_buf), 0666 | IPC_CREAT | IPC_EXCL);
  if(shm_id_secondary == -1) {
    perror("secondary: shmget failed\n");
    exit(-1);
  }

  secondary_confs=shmat(shm_id_secondary, (void*)0,0);
  if(secondary_confs==(void*)-1){
    perror("secondary: shmat failed\n");
    exit(-1);
  }	
    
  memset(secondary_confs, 0, sizeof(struct secondary_buf));

  return (struct secondary_buf *) secondary_confs;
}

// deprecated
struct done_buf * setup_done_buffer() {
  shm_id_done=shmget(IPC_PRIVATE, sizeof(struct done_buf), 0666 | IPC_CREAT | IPC_EXCL);
  if(shm_id_done == -1) {
    perror("done: shmget failed\n");
    exit(-1);
  }

  done_indicator=shmat(shm_id_done, (void*)0,0);
  if(done_indicator==(void*)-1){
    perror("done: shmat failed\n");
    exit(-1);
  }	

  memset(done_indicator, 0, sizeof(struct done_buf));

  return (struct done_buf *) done_indicator;
}

/*
 * Revokes all tied up resources.
*/
void revoke_shared_mem_resources(){
  union semun semarg;

  // delete the semaphores and memory segments
  if(shmdt(primary_confs) == -1){
    perror("shmdt failed for primary confs\n");
  }

  if(shmctl(shm_id_primary, IPC_RMID,0) == -1) {
    perror("shmctl failed for primary confs\n");
  }
  
  if(shmdt(secondary_confs) == -1){
    perror("shmdt failed for secondary confs\n");
  }

  if(shmctl(shm_id_secondary, IPC_RMID,0) == -1) {
    perror("shmctl failed for secondary confs\n");
  }


  if(shmdt(done_indicator) == -1){
    perror("shmdt failed for done indicator \n");
  }

  if(shmctl(shm_id_done, IPC_RMID,0) == -1) {
    perror("shmctl failed for done confs\n");
  }

  if (semctl(sem_id, 0, IPC_RMID, semarg) < 0) {
    perror("free_resources: semid ");
  }
}

/*
 *  producer (code-server) puts in new confs and changes the indicator
 *    flag to 0.
 *  consumer (code-manager) changes the indicator to 1 once the code
 *    generation is complete.
 *  server_or_manager parameter indicates which process is calling this
 *    function.
*/
void write_to_primary_buffer(char* confs, int server_or_manager) {
 
  // get the lock
  sem_lock(sem_id);
  
  strcpy(((struct primary_buf *)primary_confs)->buffer,confs);
  if(server_or_manager == 1)
    ((struct primary_buf *)primary_confs)->indicator[0]='1';
  else if(server_or_manager == 0)
    ((struct primary_buf *)primary_confs)->indicator[0]='0';
    
  // release the resource
  sem_unlock(sem_id);
}

/*
 *  producer (code-server) puts in new confs and changes the indicator
 *    flag to '0'.
 *  consumer (code-manager) changes the indicator to '1' once the code
 *    generation is complete.
 * 
*/
void write_to_secondary_buffer(char* confs, int server_or_manager) {
 
  // attempt to get the lock first.
  sem_lock(sem_id);
  strcpy(((struct secondary_buf *)secondary_confs)->buffer,confs);

  
  if(server_or_manager == 1)
    ((struct secondary_buf *)secondary_confs)->indicator[0]='1';
  else if(server_or_manager == 0)
    ((struct secondary_buf *)secondary_confs)->indicator[0]='0';

  // release
  sem_unlock(sem_id);
}

void write_to_done_indicator(struct done_buf *done_i, int flag) {
  // attempt to get the lock first.
  sem_lock(sem_id);
  done_i->done=flag;
  sem_unlock(sem_id);
}

/*
 * consumer (code-manager) calls this function to read the configurations
 *  from the primary buffer.
 *
 * peek parameter will only check the indicator flag.
*/
char* read_from_primary(struct primary_buf *primary,int peek) {
  sem_lock(sem_id);
  char* tmp;
  if(peek)
    tmp=&(primary->indicator[0]);
  else
    tmp=primary->buffer;
  sem_unlock(sem_id);
  return tmp;
}

/*  
 * code-manager calls this function to read the configurations
 *  from the secondary buffer.
 * Will combine this with read_from_primary later to beautify the 
 *  implementation.
*/
char* read_from_secondary(struct secondary_buf *secondary) {
  sem_lock(sem_id);
  char* tmp=secondary->buffer;
  sem_unlock(sem_id);
  return tmp;

}

int read_from_done(struct done_buf *done_i) {
  sem_lock(sem_id);
  int tmp=done_i->done;
  sem_unlock(sem_id);
  return tmp;
}

/*
 * Debugging purposes : 
 * random_delay. delay the executing process for a random number
 *     seconds.
 */
void
random_delay()
{
    static int initialized = 0;
    int random_num;
    struct timespec delay;

    if (!initialized) {
	srand(time(NULL));
	initialized = 1;
    }

    random_num = rand() % 5;
    //printf("random_num %d \n", random_num);
    delay.tv_sec = random_num;
    delay.tv_nsec = 500*random_num;
    nanosleep(&delay, NULL);
}


// debugging purposes -- can eliminate this
void
do_child_1()
{
  random_delay();
  for(int i=0; i <5; i++) {
    char * values_in_primary=read_from_primary((struct primary_buf *)primary_confs,0);
    //if(*values_in_primary=='$')
    // {
    printf("found %s at the beginning \n", values_in_primary);
	//      }
	//    else if(*values_in_primary=='*'){
	//      	printf("found * at the beginning \n");
      
	//    } else {
	//      printf("reader found %s in the beginning \n", *values_in_primary);
	//}	
    random_delay();
    printf("reader is waking up \n");
  }
  
}

// debugging purposes -- can eliminate this
void
do_parent_1()
{
  random_delay();
  char temp_[20]="$*$* 12 13 14 15";
  char * pointer =&temp_[0];
  for(int i=0; i <4; i++) {
    
    write_to_primary_buffer(pointer,1);
    random_delay();
    printf("writer is waking up \n");
    pointer=&temp_[i];
  }


}

/***
 * This function is called each time a operation requested by the client fails
 * It will return a FAIL message to the client
 ***/
void operation_failed(int sock) {
  HRegistMessage *m = new HRegistMessage(CMESG_FAIL,0);
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
    HRegistMessage *mesg=new HRegistMessage(CMESG_CLIENT_REG,client_socket);
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
    if(getpid()==code_server_pid)
      revoke_shared_mem_resources();
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
        HRegistMessage *mesg = new HRegistMessage(CMESG_CONFIRM,0);

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
        case CMESG_CLIENT_REG:
            client_registration((HRegistMessage *)m,temp_socket);
            break;
        case CMESG_CLIENT_UNREG:
            client_unregister((HRegistMessage *)m, temp_socket);
            break;
        case CMESG_CODE_GENERATION_REQ:
	  printf("code-server received code generation request \n");
	  generate_code((HDescrMessage*)m, temp_socket);
	  break;
        case CMESG_CODE_COMPLETE_QUERY:
	  printf("code-server received the code completion query \n");
	  code_generation_complete((HRegistMessage*)m, temp_socket);
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
    


    /* do listening to the socket */
    if (listen(listen_socket, 100)) {
        h_exit("Listen to socket failed!");
    }
    while (1) {
        listen_set_copy=listen_set;
        //    printf("Select\n");
        active_sockets=select(highest_socket+1,&listen_set_copy, NULL, NULL, NULL);

        printf("We have %d requests\n",active_sockets);

	int temp=getpid();
	printf("here is my pid %d \n", temp);

        /* we have a communication request */
        for (socketIterator=socket_set.begin();socketIterator!=socket_set.end();socketIterator++) {
            if (FD_ISSET(*socketIterator, &listen_set_copy) && 
		(*socketIterator!=listen_socket) && (*socketIterator!=xfd)) {
                /* we have data from this connection */
                process_message_from(*socketIterator);
            }
        }
        if (FD_ISSET(listen_socket, &listen_set_copy)) {
            /* we have a connection request */

            if ((temp_socket = accept(listen_socket,(struct sockaddr *)&temp_addr,
				      (unsigned int *)&temp_addrlen))<0) {
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

/*
 * forks and starts the code generation manager.
*/

int fork_and_launch() {
  int pid;
  int ppid=getpid();

  /* 
   * fork to get the code generation manager
   */
  
  if ((pid = fork()) < 0) {
    perror("fork");
    exit(1);
  }
    
  if (pid == 0) { 
    printf("this is the child? \n");
    // we just forked the code generation manager.
    //  In future, the server process hands over all the configurations
    //   to this process for code-generation.
    generator_manager_main(ppid);
  } else {
    // parent returns and starts the listening cycle.
    return pid;
  }
  return pid;
}

// signal handlers
void sighup_user(int signumber) {  
  signal(SIGHUP,sighup_user); /* reset signal */
  printf("PARENT: I have received a SIGHUP\n");
}

void sigint_user(int signumber) {  

  signal(SIGINT,sigint_user); /* reset signal */
  int temp_pid=getpid();
  if(temp_pid==code_server_pid)
    printf("PARENT: I have received a SIGINT\n");
  else
    exit(0);
  
  if(temp_pid==code_server_pid)
    revoke_shared_mem_resources();
  //usr_interrupt=1;
  exit(0);
}

void sigquit_user(int signumber)
{ 
  int temp_pid=getpid();
  if(temp_pid==code_server_pid)
    revoke_shared_mem_resources();
  exit(0);
}

void setup_all_signals()
{
  signal(SIGHUP,sighup_user);	
  signal(SIGINT,sigint_user);	
  signal(SIGQUIT, sigquit_user);
  signal(SIGILL, sigquit_user);
  signal(SIGSEGV, sigquit_user);
  signal(SIGBUS, sigquit_user);
}


/***
 *
 * This is the main program.
 * It first checks the arguments.
 * It then initializes and goes for the main loop.
 *
 ***/

int main(int argc, char **argv) {

  // Synchronize C++ and C I/O subsystems! 
  ios::sync_with_stdio();
  // for book-keeping and clean-up, record the pid of the
  //   code-server
  code_server_pid=getpid();
  check_parameters(argc, argv);
  
  // setup all the shared memory stuff.
  setup_semaphore();
  setup_shared_buffers();
  
  // setup all the signals stuff.
  setup_all_signals();
  
  /*
    fork and launch the generator manager
  */
  generator_pid=fork_and_launch();
  code_manager_pid=generator_pid;

  printf("getpid(): %d vs. code_server_pid: %d vs. manager pid: %d \n", 
	 getpid(), code_server_pid, generator_pid);

  
  /* do all the connection related stuff */
  if(getpid()==code_server_pid)
    server_startup();
  else
    return 0;

  if(getpid()==code_server_pid)
    main_loop(generator_pid);
  else
    return 0;


  if(getpid()==code_server_pid)
     revoke_shared_mem_resources();
  else
    return 0;

  // should we wait for all the processes to die here?
  int child_status;
  wait(&child_status);
 
  return 0;
}

/*
 * Helper functions
 */

/*
 * generate_code: parses the message from the harmony server and 
 * puts the configurations to the promary and secondary bufers.
 */	
void generate_code(HDescrMessage *mesg, int client_socket) {

    // first get the query string
    char* temp = (char *) malloc(strlen(mesg->get_descr())+10);
    strcpy(temp, mesg->get_descr());
    
    if(debug_mode)
      printf("got the request data: %s \n", temp);

    /* the message string sent by the client will be in the following
     *  format:
     *   <primary confs> | <secondary confs>
     *    where <primary confs> and <secondary confs> are represented
     *    as: <param 1> : <param 2> : ...
     */
    string str_point(temp);
    Tokenizer strtok_double_colon;
    strtok_double_colon.set(temp, "|");	

    // primary confs
    string prim=strtok_double_colon.next();
    cout << "primary: " << prim << endl;

    // secondary confs
    string secon=strtok_double_colon.next();
    char *char_star;
    char_star = new char[prim.length() + 1];
    strcpy(char_star, prim.c_str());

    // put the primary confs in the shared space for primary confs
    write_to_primary_buffer(char_star,1);

    char_star = new char[secon.length() + 1];
    strcpy(char_star, secon.c_str());
    
    // put the secondary confs in the shared space for secondary confs
    write_to_secondary_buffer(char_star,1);

    // signal the code manager that there is a new set of primary confs
    kill(code_manager_pid, SIGUSR1);

    // all is good so far. send a confirmation message to client
    HRegistMessage *m=new HRegistMessage(CMESG_CONFIRM, 1);
    send_message(m,client_socket);
    delete m;
    delete mesg;
}


/*
 * reads the primary buffer and returns true if the code generation is complete.
 */
void code_generation_complete(HRegistMessage *mesg, int client_socket) {
  int return_val=0;
  char * values_in_primary=read_from_primary((struct primary_buf *)primary_confs,1);
  if(*values_in_primary=='0'){
    return_val=1;
  }
  HRegistMessage *m=new HRegistMessage(CMESG_CODE_COMPLETE_RESPONSE, return_val);
  send_message(m,client_socket);
  delete m;
  delete mesg;
}
