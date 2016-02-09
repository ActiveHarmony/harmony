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

/***
 *
 * include user defined headers
 *
 ***/
#include "hutil.h"
#include "hmesgs.h"
#include "hsockutil.h"
#include <fcntl.h>
#include "hclient.h"
#include <netinet/in.h>
#include <strings.h>
#include <sys/time.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include "Harmony.h"

#include "queue2.h"


/***
 *
 * Exit and send a mesage to stderr
 *
 ***/
void h_exit(char *errmesg){
  perror(errmesg);
  exit(1);
}


/******************************
 * 
 * Author: Cristian Tapus
 * History:
 *   Jan 5, 2001   - comments and updated by Cristian Tapus
 *   Nov 20, 2000  - changes made by Djean Perkovic
 *   Sept 23, 2000 - comments added
 *   July 12, 2000 - first version
 *   July 15, 2000 - comments and update added by Cristian Tapus
 *
 *******************************/

/***
 *
 * define functions used to pack, unpack message info
 *
 ***/

static int debug_mode;

// serialize the Variable definition 
// this class is defined in the hmesgs.h file
int VarDef::serialize(char *buf) {
  int buflen=0;
  unsigned int temp;
  
  // first serialize the variable type, which is an int and can
  // be one of the following: VAR_INT or VAR_STR
  memcpy(buf+buflen, &varType, sizeof(unsigned long int));
  buflen+=sizeof(unsigned long int);
  
  // then serialize the name of the variable.
  // when serializing strings put the length of the string first and
  // then the string itself
  temp=htonl(strlen(varName));
  memcpy(buf+buflen, &temp, sizeof(unsigned long int));
  buflen+=sizeof(unsigned long int);
  memcpy(buf+buflen, varName, strlen(varName));
  buflen+=strlen(varName);    
  
  // finally put the value of the variable into the buffer.
  // if the type is int, just add it to the buffer
  // if type is string, first add the length and then the string itself
  switch (getType()) {
  case VAR_INT:
    temp=htonl(*((int *)getPointer()));
    memcpy(buf+buflen, &temp, sizeof(unsigned long int));
    buflen+=sizeof(unsigned long int);
    break;
  case VAR_STR:
    temp=htonl(strlen((char *)getPointer()));
    memcpy(buf+buflen, &temp, sizeof(unsigned long int));
    buflen+=sizeof(unsigned long int);
    memcpy(buf+buflen, getPointer(), strlen((char *)getPointer()));
    buflen+=strlen((char *)getPointer());    
  }
  
  return buflen;
  
}


// deserialize the content of a variable 
// use the same order as above
int VarDef::deserialize(char *buf) {
  int bl=0;
  int t,tv, sl, ssl;
  char *s, *ss;

  memcpy(&t,buf+bl,sizeof(unsigned long int));
  bl+=sizeof(unsigned long int);
  setType(ntohl(t));


  memcpy(&tv,buf+bl,sizeof(unsigned long int));
  bl+=sizeof(unsigned long int);
  sl = ntohl(tv);

  s=(char *)malloc(sl+1);
  memcpy(s,buf+bl,sl);
  bl+=sl;
  s[sl]='\0';
  setName(s);


  switch (getType()) {
  case VAR_INT:
    memcpy(&tv, buf+bl,sizeof(unsigned long int));
    bl+=sizeof(unsigned long int);
    setValue(ntohl(tv));
    break;
  case VAR_STR:
    memcpy(&tv,buf+bl,sizeof(unsigned long int));
    bl+=sizeof(unsigned long int);
    ssl=ntohl(tv);
    ss=(char *)malloc(ssl+1);
    strncpy(ss,buf+bl,ssl);
    bl+=ssl;
    ss[ssl]='\0';
    setValue(ss);
    free(ss);
    break;
  }

  free(s);

  return bl;
}



// this is a function used for debug purposes. It prints the info
// about the variable
void VarDef::print() {
    fprintf(stderr, "Variable %s has type %d and value ",getName(), getType() );
    switch (getType()) {
    case VAR_INT:
      fprintf(stderr, " %d ; shadow %d", *((int *)getPointer()),*((int *)getShadow()), *((int*)getPointer()));
      break;
    case VAR_STR:
      fprintf(stderr, " [%s] ; shadow [%s], val [%s]",(char *)getPointer(), (char *)getShadow(), (char*)getPointer());
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "Pointers: %p ; %p ; %p ; %p\n",getPointer(), getShadow(), getName(), &varType);
}



// serialize the general message class
int HMessage::serialize(char *buf) {  
  int buflen=0;

  // include the message type and the version of the protocol
  memcpy(buf+buflen,&type, sizeof(unsigned long int));
  buflen+=sizeof(unsigned long int);
  memcpy(buf+buflen,&version, sizeof(unsigned long int));
  buflen+=sizeof(unsigned long int);
  
  return buflen;
}


// deserialize the general message class
int HMessage::deserialize(char *buf) {
  int bl=0;

  // deserialize the message type and the version of the protocol
  memcpy(&type,buf+bl, sizeof(unsigned long int));
  bl+=sizeof(unsigned long int);
  memcpy(&version,buf+bl, sizeof(unsigned long int));
  bl+=sizeof(unsigned long int);

  return bl;
}


// function used for debugging purposes that prints the type of the
// message
void HMessage::print() {
  fprintf(stderr, "Message type: %s %d\n",print_type[get_type()], get_type());
}



// serialize the Registration Message
int HRegistMessage::serialize(char *buf) {  
  int buflen;

  buflen=HMessage::serialize(buf);

  // to the serialization of the general message add the param
  // that HRegistMessage keeps track of
  memcpy(buf+buflen,&param, sizeof(unsigned long int));
  buflen+=sizeof(unsigned long int);
  memcpy(buf+buflen,&identification, sizeof(int));
  buflen+=sizeof(int);

  return buflen;
}


// deserialize the Registration Message
int HRegistMessage::deserialize(char *buf) {
  int bl=0;

  bl = HMessage::deserialize(buf);

  memcpy(&param,buf+bl, sizeof(unsigned long int));
  bl+=sizeof(unsigned long int);
  memcpy(&identification, buf+bl, sizeof(int));
  bl+=sizeof(int);

  return bl;
}


// print extra information about the HRegistMessage
void HRegistMessage::print(){
  HMessage::print();
  fprintf(stderr, "Parameter: %d\n",get_param());
}


// serialize the description message
int HDescrMessage::serialize(char *buf) {  
  int buflen;

  buflen = HMessage::serialize(buf);
  
  // serialize the description (which is a string)
  // by first adding the length to the buffer and then
  // the actual string
  memcpy(buf+buflen,&descrlen, sizeof(unsigned long int));
  buflen+=sizeof(unsigned long int);  
  memcpy(buf+buflen,descr, get_descrlen());
  buflen+=get_descrlen();

  return buflen;
}


// deserialize the description message
int HDescrMessage::deserialize(char *buf) {
  int bl=0;

  bl = HMessage::deserialize(buf);

  memcpy(&descrlen,buf+bl, sizeof(unsigned long int));
  bl+=sizeof(unsigned long int);
  free(descr);
  descr=(char *)malloc(get_descrlen()+1);
  memcpy(descr,buf+bl, get_descrlen());
  descr[get_descrlen()]='\0';
  bl+=get_descrlen();

  return bl;
}


// print info about the description message
void HDescrMessage::print() {
  HMessage::print();
  fprintf(stderr, "Description has size %d: [%s]\n",get_descrlen(),get_descr());
}



// returns the variable n from the HUpdateMessage
VarDef * HUpdateMessage::get_var(int n) {
  
  vector<VarDef>::iterator VarIterator = var_list.begin();

  for (int i=0;i<n;i++,VarIterator++);

  return VarIterator;
}


// adds a variable to the HUpdateMessage
void HUpdateMessage::set_var(VarDef v) {

  //printf("Set_Var!!!\n");

  //printf("var: %s\n",v.getName());

  set_nr_vars(get_nr_vars()+1);

  var_list.push_back(v);

  //printf("var pushed\n");
}


// compute the message size of the HUpdateMessage
// this depends on the length of each of the variables
// stored in the list of variables
// and that is why it could not be inlined.
int HUpdateMessage::get_message_size() { 
  int len;
  
  len = HMessage::get_message_size()+2*sizeof(unsigned long int);

  vector<VarDef>::iterator VarIterator;

  for (VarIterator=var_list.begin(); VarIterator!=var_list.end(); VarIterator++) {

    VarDef var=*VarIterator;
    len+=sizeof(unsigned long int); // var type
    
    len+=sizeof(unsigned long int); // name len
    len+=strlen((char *)var.getName());
    
    switch (var.getType()) {
    case VAR_INT:
      len+=sizeof(unsigned long int); //value
      break;
    case VAR_STR:
      len+=sizeof(unsigned long int); //strlen
      len+=strlen((char *)var.getPointer()); // size of the string
      break;
    }
  }

  return len;
}


// serialize the HUpdateMessage
int HUpdateMessage::serialize(char *buf) {
  int buflen=0;
  unsigned long int temp;

  buflen=HMessage::serialize(buf);
  
  // add the timestamp to the buffer
  memcpy(buf+buflen, &timestamp, sizeof(unsigned long int));
  buflen+=sizeof(unsigned long int);

  // add the number of the variables to the buffer
  memcpy(buf+buflen, &nr_vars, sizeof(unsigned long int));
  buflen+=sizeof(unsigned long int);

  vector<VarDef>::iterator VarIterator;

  // serialize the variables
  for (VarIterator=var_list.begin(); VarIterator!=var_list.end(); VarIterator++)
    buflen+=(*VarIterator).serialize(buf+buflen);

  return buflen;
}


// deserialize the HUpdateMessage
int HUpdateMessage::deserialize(char *buf) {
  int bl=0;
  int t,tv, sl, ssl;
  char *s, *ss;
  VarDef *vv;
  unsigned long int tstamp;

  bl = HMessage::deserialize(buf);

  // deserialize the timestamp
  memcpy(&tstamp,buf+bl, sizeof(unsigned long int));
  bl+=sizeof(unsigned long int);

  set_timestamp(tstamp);

  // deserialize the number of variables
  memcpy(&t,buf+bl, sizeof(unsigned long int));
  bl+=sizeof(unsigned long int);
  
  tv=ntohl(t);

  // deserialize the variables
  for (int i=0; i<tv;i++) {
    vv = new VarDef();
    bl+=vv->deserialize(buf+bl);
    set_var(*vv);
    //    if (debug_mode)
    //      vv->print();
  }

  return bl;
}


// print the info contained in the Update Message
// this means the variables sent for updating
void HUpdateMessage::print() {
  
  VarDef var;
  
  fprintf(stderr, "PRINT UPDATE MESSAGE:\n");
  HMessage::print();
  fprintf(stderr, "Nr vars: %d\n",get_nr_vars());
  fprintf(stderr, "Timestamp: %d\n",get_timestamp());

  vector<VarDef>::iterator VarIterator;

  //  for(VarIterator=var_list.begin();VarIterator!=var_list.end();VarIterator++) {
  //    var=VarIterator;
  for (int i=0;i<get_nr_vars();i++) {
    (get_var(i))->print();
  }
  fprintf(stderr, "DONE PRINTING!\n");
}


/*
 * include user defined header
 */
//#include "hsockutil.h"

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

  //  m->print();

  printf("Sending message type %d to socket %d\n",m->get_type(),sock);
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

  if ((buflen=recvfrom(sock, (char *)&nbuflen, sizeof(nbuflen),0, NULL, NULL))<0) {
	h_exit("Failed to receive message size!");
  }

  buf=malloc(ntohl(nbuflen));
  if (buf==NULL) {
	printf("Recv:%d\n",nbuflen);
    h_exit("Could not malloc!");
	}
  
         
  if ((buflen=recvfrom(sock,(char *) buf, ntohl(nbuflen),0, NULL, NULL))<0) {
    h_exit("Failed to receive message!");
  }


  printf("Received message type: %d from %d\n",get_message_type(buf),sock);

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
    m=new HUpdateMessage();
    buflend = ((HUpdateMessage *)m)->deserialize((char *)buf);
    break;
  default:
    printf("Message type: %d\n", get_message_type(buf));
    h_exit("Wrong message type!");
  }
  
  return m;
}





















/******************************
 * 
 * Author: Cristian Tapus
 * History:
 *   Dec 22, 2000 - comments and updates added by Cristian Tapus
 *   Nov 20, 2000 - Dejan added some features to the existing functions
 *   Sept 22, 2000 - comments added and debug moded added
 *   July 9, 2000 - first version
 *   July 15, 2000 - comments and update added by Cristian Tapus
 *
 *******************************/


/***
 * include other user written headers 
 ***/
/*
#include <fcntl.h>
#include "hclient.h"
#include <netinet/in.h>
#include <strings.h>
#include <sys/time.h>
#include <signal.h>
*/

/*
 * global variables
 */

// the purpose of this static variable is to define the debug mode
// when this value is set to 1, different values might be printed for
// debuginng purposes. However, the information that is outputed might 
// not make sens (except for the author! :)
//static int debug_mode=0;


// the client socket that will support the TCP connection to the server
int hclient_socket=-1;

// id that identifies an application (used for migration)
int hclient_id = -1;

// a global variable that has enables/disables the use of signals for
// getting the information from the server. For more information about
// this variable please consult the Programmer's Guide.
int huse_signals;


// this list of VarDef keeps track of all the variables registered by the
// application to the server. This information is only kept on the client
// side for now, as no use of it was needed on the server side as of now...
// For information about the VarDef class, please consult hmesgs.(ch)
list<VarDef> reg_vars;

// the timestamp used by the simplex minimization method to decide
// whether the performance reflects the changes of the parameters
unsigned long int hclient_timestamp = 0;


/***
 * prototypes of side functions
 ***/
void process_update(HUpdateMessage *m);


/***
 * function definitions
 ***/

/**
 *
 * returns the local socket used for connection to server
 *
 **/
int get_local_sock() {
  return hclient_socket;
}


// the purpose of these double definition is the use of different
// calls for the same function. I will clean this in the next version. 
int hget_local_sock() {
  return hclient_socket;
}


/***
 *
 * Exit and send a mesage to stderr
 *
 ***/
void hc_exit(char *errmesg){
  perror(errmesg);
  harmony_end();
  exit(1);
}


// this is an internal definition class 
// that serves the purposes of finding objects in a list. (STL stuff)
// this way we find a Variable in the list of registered variables
// by its name
class findVarbyName {
 private:
  char *n;
 public:
  findVarbyName(char *vN){
    n=(char *)malloc(strlen(vN));
    strcpy(n,vN);
  }
  
  bool operator () (VarDef& Var) {
    return (!strcmp(n, Var.getName()));
  }  
};

void (*app_sigio_handler)(int);

/*****
 * 
 * this is the definition of the sigio handler. This function gets called
 * if the user decided to use this mechanism to get information from the
 * server (rather then polling the server when it needs the information!)
 * 
 *****/
void hsigio_handler(int s) {
  // the purpose of this set of file descriptors is to see if the SIGIO
  // was generated by information received on the harmony communication 
  // socket
  fd_set modif_sock;

  //  if (debug_mode)
  fprintf(stderr, "***** Got SIGIO %d !\n",s);

  while(1){
  again:
 
    // initialize the set of modified file descriptor to check for
    // harmony socket only
    FD_ZERO(&modif_sock);
    FD_SET(get_local_sock(),&modif_sock);
  
    // set tv for polling
    struct timeval tv =  {0, 0};
    int res;

    if ((res = select(get_local_sock()+1,&modif_sock, NULL, NULL, &tv)) < 0) {
      if (errno = EINTR) 
	goto again;
      perror("ERROR in SELECT");
      hc_exit("Error in select");
    }
    
    if (!res)
      break;

    // if the harmony communication socket is one of the modified 
    // sockets, then we have a packet from the server
    if (FD_ISSET(get_local_sock(),&modif_sock)) {
      // check the message from the server
      
      // we can read data from the socket. define for this a message
      // we know that from the server we only get update messages
      HUpdateMessage *m;
      
      m = (HUpdateMessage *) receive_message(get_local_sock());
      
      // print the content of the message if in debug mode
      if (debug_mode)
	m->print();
      
      process_update(m);
      delete m;

#ifdef NOTDEF
	// BUGGG: There ARE more variables updated at a time!
	// we know that only one variable is updated at a time by SIGIO
	// we will find the variable
        // *** above comment added by Dejan
        // *** below response added by Cristian 
        // this is not necessarily a BUG. By the way the server currently 
        // sends the variables, we expect only one variable at a time
        // this is to be updated in a later version
	list<VarDef>::iterator VarIterator = 
	  find_if(reg_vars.begin(), reg_vars.end(),findVarbyName(m->get_var(0)->getName()));

	// if the variable was not found just print an error message and
	// continue. Maybe the server messed things up and data was sent to
	// the bad client. Who knows what might have happened!
	if (VarIterator==reg_vars.end()) {
	  fprintf(stderr, "(sigio handler)Harmony error: Variable not registerd: %s!\n",
		 m->get_var(0)->getName());
	  return;
	}
	else {
	  // the variable was found and its value needs to be updated 
	  
	  if (debug_mode)
	    VarIterator->print();
	  
	  // update shadow value for variable
	  // we only update the shadow value of the variable because we want
	  // the user to be the only one to decide when the actual value of the
	  // variable to get changed. There are only certain points in the 
	  // program where these changes might be reflected
	  switch (VarIterator->getType()) {
	  case VAR_INT:
	    VarIterator->setShadow(*(int *)m->get_var(0)->getPointer());
	    break;
	  case VAR_STR:
	    VarIterator->setShadow((char *)m->get_var(0)->getPointer());
	    break;
	  }
	  
	  if (debug_mode)
	    VarIterator->print();
	}
	// free the space that was previously allocated
	delete m;
#endif
    }
    else
      app_sigio_handler(s);
  }
}


/*********
 *        block_sigio(), unblock_sigio()
 * Description: (un)blocks sigio signals
 * Used from:   routines that send messages asynchronously to the server
 *
 ***********/

int hsigio_blocked = FALSE;
void block_sigio() {
#if     defined(SOLARIS) || defined(__linux)
    sigset_t    set;
    sigemptyset(&set);
    sigaddset(&set, SIGIO);
    sigaddset(&set, SIGALRM);
    sigprocmask(SIG_BLOCK, &set, NULL);
#else
    sigblock(sigmask(SIGIO));
#endif  //SOLARIS
    hsigio_blocked = TRUE;
};

void unblock_sigio() {
#ifdef DEBUG_SIGIO
  if (!hsigio_blocked) {
        fprintf(stderr, "WARNING: Improper state or use of unblock_sigio\n");
  }
#endif //DEBUG_SIGIO
    hsigio_blocked = FALSE;
#ifdef  SOLARIS
    sigset_t    set;
    sigemptyset(&set);
    sigprocmask(SIG_SETMASK, &set, NULL);
#else
    sigsetmask(0);
#endif  //SOLARIS
};


/* 
 * this variables are used for migration
 * we save the server and port of the server in case we have to migrate
 *
 */
char harmony_host[256];
int harmony_port;

/*
 * When a program registers with the harmony server it uses this function
 *
 * Params:
 *  use_sigs (int) - the client will use signals if the value is 1
 *                   the client will use polling if the value is 0
 *                   the client will use sort of a polling if value is 2
 *  relocated (int)- this checks if the client has been migrating lately
 *                   the use of this variable is not very clear yet, but 
 * 			Dejan insisted on having it.
 *  sport (int)    - the port of the server
 *  shost (char *) - the host of the server
 *
 * The return value is the socket that we will further use for the connection
 */

void harmony_startup(int use_sigs, int relocated = 0, int sport = 0, char *shost = NULL) {

  struct sockaddr_in address;
  struct hostent *hostaddr = NULL;
  char *hostname;
  HRegistMessage *mesg;

  char temps[BUFFER_SIZE];

  int fileflags;
  
  fprintf(stderr, "hs:%d\n", __LINE__);

  /* set the flag that tells if we use signals */
  huse_signals = use_sigs;

    /* create socket */
  if ((hclient_socket=socket(AF_INET,SOCK_STREAM,0))<0) {
    hc_exit("Could not create socket!");
  }

  /* connect to server */
  address.sin_family = AF_INET;
  
  // look for the HARMONY_S_PORT environment variable
  // to set the port of connection to the machine where the harmony
  // server is running

  if (sport == 0){
    if (getenv("HARMONY_S_PORT")==NULL)
      hc_exit("HARMONY_S_PORT variable not set!");
    sport = atoi(getenv("HARMONY_S_PORT"));
  }
  address.sin_port=htons(sport);

  //look for the HARMONY_S_HOST environment variable
  // to set the host where the harmony server resides 
  if (shost == NULL){
    if (getenv("HARMONY_S_HOST")==NULL) 
      hc_exit("HARMONY_S_HOST variable not set!");
    shost=getenv("HARMONY_S_HOST");
  }
  hostname = shost;

  if ((hostaddr=gethostbyname(hostname)) == NULL)
    hc_exit("Host not found!");

  /* save server hostname and port if necessary to migrate */
  strcpy(harmony_host, hostname);
  harmony_port = sport;

  /* set up the host address we will connect to */
#ifdef Linux

  inet_aton(inet_ntop(hostaddr->h_addrtype,hostaddr->h_addr,temps,sizeof(temps)), &address.sin_addr); 

#else

  bcopy((char *)hostaddr->h_addr, (char *)&address.sin_addr, hostaddr->h_length);
#endif

  int optVal = 1;
  int ret = setsockopt(hclient_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &optVal, sizeof(int));
  if (ret) {
    perror("setsockopt");
    hc_exit("<setsockopt>fd_create");
  }

  // try to connect to the server
  if (connect(hclient_socket,(struct sockaddr *)&address, sizeof(address))<0)
    hc_exit("Connection failed!");

  fprintf(stderr, "CONNECTED\n");

  /* send the registration message */
  mesg = new HRegistMessage(HMESG_CLIENT_REG, huse_signals);

  if (relocated){
    assert(hclient_id > 0);
    mesg->set_id(hclient_id);
  }
  send_message(mesg,hclient_socket);

  delete mesg;

  /* wait for confirmation */

  mesg = (HRegistMessage *)receive_message(hclient_socket);
  hclient_id = mesg->get_id();
  fprintf(stderr, "hclient_id: %d\n", hclient_id);
  assert(hclient_id > 0);
  delete mesg;

  if (huse_signals) {
    // install the SIGIO signal handler
    // and set which file descriptor to produce the SIGIO
#undef DONT_SAVE_OLD_HANDLER
#ifdef DONT_SAVE_OLD_HANDLER
    signal(SIGIO, hsigio_handler);
#else
    struct sigaction    ha_act, app_act;

    ha_act.sa_handler = hsigio_handler;
    sigemptyset(&ha_act.sa_mask);
    sigaddset(&ha_act.sa_mask, SIGALRM);
#if defined(__sun) && ! defined(__SVR4)
    ha_act.sa_flags = 0;
#else
    ha_act.sa_flags = SA_RESTART;
#endif
    if (sigaction(SIGIO, &ha_act, &app_act)){
        hc_exit("segv sigaction problem");
    }

    // save app handler to call it if SIGIO is not for Harmony
    app_sigio_handler = app_act.sa_handler;

#endif
    if (fcntl(hclient_socket, F_SETOWN, getpid()) < 0) {
      hc_exit("fcntl F_SETOWN");
    }
    
    if (fileflags = fcntl(hclient_socket, F_GETFL ) == -1) {
      hc_exit("fcntl F_GETFL");
    }

#ifdef LINUX
    // this does not work on SOLARIS
    if (fcntl(hclient_socket, F_SETFL, fileflags | FASYNC) == -1) 
      hc_exit("fcntl F_SETFL, FASYNC");
#endif

  }

  fprintf(stderr, "handler installed\n");
}


/*
 * When a program announces the server that it will end it calls this function
 */
void harmony_end(){

  fprintf(stderr, "he:%d\n", __LINE__);

  HRegistMessage *m = new HRegistMessage(HMESG_CLIENT_UNREG,0);

  // send the unregister message to the server
  send_message(m,hclient_socket);


  printf("Exit message sent!\n");

  delete m;

  /* wait for server confirmation */

  m =(HRegistMessage *) receive_message(hclient_socket);
  
  printf("Message received!?!\n");
  m->print();

  delete m;

  printf("Close socket!\n");

  // close the actual connection 
  close(hclient_socket);

}


/*
 * Inform the Harmony server of the bundles and requirements of the application
 */
void harmony_application_setup(char *description){

  // disable SIGIO
  //  block_sigio();
  //#ifdef NOTDEF
  //  signal(SIGIO, NULL);
  //#endif NOTDEF

  HDescrMessage *mesg=new HDescrMessage(HMESG_APP_DESCR,description,strlen(description));

  send_message(mesg, hclient_socket);

  delete mesg;


  /* wait for confirmation */
  HMessage *m;
  m=receive_message(hclient_socket);

  if (m->get_type()!=HMESG_CONFIRM) {
    // something went wrong on the server side !
    fprintf(stderr," Unexpected message type: %d\n",m->get_type());
    delete m;
    hc_exit("Application description is not correct!\n");
  }

  delete m;

  //#ifdef NOTDEF
  // enable SIGIO
  //  signal(SIGIO, hsigio_handler);
  //#endif NOTDEF
  //  unblock_sigio();
}



/***
 * this version of the function reads the description from a given filename
 * and passes it as a string to the above defined function
 ***/
void harmony_application_setup_file(char *fname) {
  
  struct stat *statb=(struct stat *)malloc(sizeof(struct stat));
  int fd;
  char *d;

  fprintf(stderr, "hasf:%d ; filename:%s\n", __LINE__, fname);


  // check the file stat
  if (stat(fname, statb)<0) 
    hc_exit("Error get file stat!");

  // open the file
  if ((fd=open(fname, 0))<0)
    hc_exit("Error open file!");

  // alloc the data
  d=(char *)malloc(statb->st_size+1);
  
  // read the data
  if (read(fd,d,statb->st_size)<0)
    hc_exit("Error reading description!");

  d[statb->st_size]='\0';

  // free file stat
  free(statb);

  // close the file
  if (close(fd)<0)
    hc_exit("Error closing file!");

  // send description to server
  harmony_application_setup(d);
}


/*
 * Bind a local variable with a harmony bundle 
 *
 * Params:
 * 	appName (char *) - the name of the application
 *   	bundleName (char *) - the name of the bundle we want to bind
 *	type (int) the type of the variable we want to bind: INT, STR
 *	localonly (int) - used by Dejan. The use is not very clear to me
 *			  I guess he doesn't want the variable to have the 
 *				name defined as the others have, but rather	
 *				take the name of the bundle
 */
void * harmony_add_variable(char *appName, char *bundleName, int type, int localonly = 0){
  if (appName == NULL)
    appName = "client";
  fprintf(stderr, "hav: %s %s %d\n", appName, bundleName, localonly);

  if (localonly) {
    VarDef v(bundleName, type);
    v.alloc();

    reg_vars.push_back(v);

    // return the pointer of the variable
    void *r = reg_vars.rbegin()->getPointer();
    fprintf(stderr, "hav out %x\n", r);
    return r;
  }

  // block_sigio();
  // the variable
  VarDef v;
  // the variable name as known by TCL
  char varN[MAX_VAR_NAME];

  if (debug_mode)
    fprintf(stderr, "appName:%s ; bundleName:%s\n",appName, bundleName);

  // create the name of the variable as described in the Tcl code that
  // accompanies Harmony
  sprintf(varN,"%s_%d_bundle_%s",appName,hclient_id,bundleName);

  if (debug_mode)
    fprintf(stderr, "Variable Tcl Name: %s ; Type %d ; Strlen:%d \n",varN, type, strlen(varN));

  // send variable registration message
  HDescrMessage *m=new HDescrMessage(HMESG_VAR_DESCR, bundleName, strlen(bundleName));
  send_message(m,get_local_sock());
  delete m;


  // wait for confirmation
  // the confirmation message also contains the initial value of the 
  // variable
  HMessage *mesg;
  mesg=receive_message(get_local_sock());


  if (debug_mode)
    mesg->print();

  if (mesg->get_type()==HMESG_FAIL) {
    /* failed to register variable with server */
    fprintf(stderr, "Failed to register variable %s from application %s \n",bundleName, appName);
    delete mesg;
    //unblock_sigio();
    hc_exit("Please check variable!");
  }

  // add the variable to the registered variable list
  v=*((HUpdateMessage *)mesg)->get_var(0);

  fprintf(stderr, "v.getName()=%s\n", v.getName());
  delete mesg;

  reg_vars.push_back(v);
  //unblock_sigio();

  // return the pointer of the variable
  return reg_vars.rbegin()->getPointer();

}


// this is another helper class 
// that is designed for finding variables in the registered variables list
// by the address of the value of the variable
class findVarbyPointer {
 private:
  void *p;
 public:
  findVarbyPointer(void *vp){
    p=vp;
  }
  
  bool operator () (VarDef& Var) {
    return p==Var.getPointer();
  }  
};



/*
 * Send to the server the value of a bound variable
 */
void harmony_set_variable(void *variable){
  fprintf(stderr, "set variable\n");

  // get the variable associated with the pointer
  list<VarDef>::iterator VarIterator = 
    find_if(reg_vars.begin(), reg_vars.end(),findVarbyPointer(variable));

  // if variable was not found just print an error and continue
  if (VarIterator==reg_vars.end()) {
    fprintf(stderr, "(set_variable)Harmony error: Variable not registered: %s\n", ((VarDef*)variable)->getName());
    return;
  }
  else {
    //block_sigio();
    // send request message 
   HUpdateMessage *m=new HUpdateMessage(HMESG_VAR_SET, 0);

   switch (VarIterator->getType()) {
   case VAR_INT:
        VarIterator->setShadow(*(int *)VarIterator->getPointer());
        break;
   case VAR_STR:
        VarIterator->setShadow((char *)VarIterator->getPointer());
        break;
   }
   m->set_var(*VarIterator);
   
   if (debug_mode)
     m->print();

   send_message(m,get_local_sock());
   delete m;

   // wait for the answer from the harmony server
   HMessage *mesg=(HMessage *)receive_message(get_local_sock());

   if (mesg->get_type()==HMESG_FAIL) {
     /* could not set variable value */
     delete mesg;
     hc_exit("Failed to set variable value!");
   } else if (mesg->get_type()==HMESG_VAR_REQ) {
     /* received an update */
     process_update((HUpdateMessage*)mesg);
   }

   delete mesg;
   //unblock_sigio();
  }
}

/*****
     Name:        Process an update message
     Description: Set shadow values of updated variables to received values
     Called from: sigio_handler and harmony_set_variable     
*****/
void process_update(HUpdateMessage *m) {
  fprintf(stderr, "num vars in update: %d\n", m->get_nr_vars());
  for (int i = 0; i < m->get_nr_vars(); i++) {
    VarDef *v = m->get_var(i);
    fprintf(stderr, "varName: %s\n", v->getName());

    list<VarDef>::iterator VarIterator =
      find_if(reg_vars.begin(), reg_vars.end(),findVarbyName(v->getName()));

    // if the variable was not found just print an error message and
    // continue. Maybe the server messed things up and data was sent to
    // the bad client. Who knows what might have happened!
    if (VarIterator==reg_vars.end()) {
      fprintf(stderr, "Harmony error: Variable not registerd: %s!\n", v->getName());
    } else {
      switch (v->getType()) {
      case VAR_INT:
	VarIterator->setShadow(*(int*)v->getPointer());
	break;
      case VAR_STR:
	VarIterator->setShadow((char*)v->getPointer());
	fprintf(stderr, "new val: %s [%s] shadow [%s]\n", v->getName(),
		(char*)(VarIterator->getPointer()),
		VarIterator->getShadow());
	break;
      }
    }
    fprintf(stderr, "up\n");
  }
}

/*
 * Update bound variables on server'side.
 */
void harmony_set_all(){
  fprintf(stderr, "hsa:%d\n", __LINE__);

  //block_sigio();
  // go through the entire list of registered variables and create a 
  // message containing all these variables and their values

  list<VarDef>::iterator VarIterator;

  HUpdateMessage *m=new HUpdateMessage(HMESG_VAR_SET, 0);
    
  for (VarIterator=reg_vars.begin();VarIterator!=reg_vars.end(); VarIterator++) {
    switch (VarIterator->getType()) {
    case VAR_INT:
        VarIterator->setShadow(*(int *)VarIterator->getPointer());
        break;
    case VAR_STR:
        VarIterator->setShadow((char *)VarIterator->getPointer());
        break;
    }
    m->set_var(*VarIterator);
  }
  
  // send the message to the server
  send_message(m,get_local_sock());
  delete m;
  
  // wait for the confirmation 
  HMessage *mesg=receive_message(get_local_sock());
  
  if (mesg->get_type()==HMESG_FAIL) {
    /* could not set variable value */
    delete mesg;
    hc_exit("Failed to set variables!");
  } else if (mesg->get_type()==HMESG_VAR_REQ) {
    /* received an update */
    process_update((HUpdateMessage*)mesg);
  }

  
  delete mesg;
  //unblock_sigio();
}


/*
 * Get the value of a bound variable
 */
void harmony_request_variable(void *variable){

  fprintf(stderr, "hrv:%d\n", __LINE__);

  // lookup variable by the pointer in the registered variables list
  list<VarDef>::iterator VarIterator = 
    find_if(reg_vars.begin(), reg_vars.end(),findVarbyPointer(variable));
  
  // if variable is not found just print an error message and continue
  if (VarIterator==reg_vars.end()) {
    fprintf(stderr, "(request_var)Harmony: Variable not registered: %s\n", ((VarDef*)variable)->getName());
    return;
  }
  else {
    // here we have to differentiate between using and not using signals
    // if signals are used then we just have to copy the shadow value of the 
    // variable into the actual value
    // otherwise we have to send a message to the server, requesting
    // for the value of the variable.

    if (huse_signals) {
      // we use signals so values have been updated!
      // just copy the shadow into the value

      switch (VarIterator->getType()) {
      case VAR_INT:
	VarIterator->setValue(*(int *)VarIterator->getShadow());
	break;
      case VAR_STR:
	VarIterator->setValue((char *)VarIterator->getShadow());
	break;
      }

      return;
    }

    // signals are not used 

    // send request message 
    HUpdateMessage *m=new HUpdateMessage(HMESG_VAR_REQ, 0);
    
    m->set_var(*VarIterator);
    
    
    if (debug_mode)
      m->print();
    
    send_message(m,get_local_sock());
    delete m;
    
    // wait for answer
    HMessage *mesg = receive_message(get_local_sock());
    
    if (mesg->get_type()==HMESG_FAIL) {
      // the server could not return the value of the variable
      delete mesg;
      fprintf(stderr, "Harmony error: Server could not return value of variable!\n");
      return;
    }
    
    m=(HUpdateMessage *)mesg;
    
    if (debug_mode)
      m->print();
    
    // update the value
    switch (VarIterator->getType()) {
    case VAR_INT:
      VarIterator->setValue(*(int *)m->get_var(0)->getPointer());
      break;
    case VAR_STR:
      VarIterator->setValue((char *)m->get_var(0)->getPointer());
      break;
    }
    
    delete m;
  }
  
}


/*
 * Get the current value of all the bound variables
 *
 * Params:
 *   pull (int) - used by Dejan. The use is not very clear to me yet.
 */
void harmony_request_all(int pull = 0){

  // here we also have to differentiate between use of signals and
  // no signals
  fprintf(stderr, "hra:%d\n", __LINE__);

  list<VarDef>::iterator VarIterator = reg_vars.begin();
  //  fprintf(stderr, "ALL\n");
  if (huse_signals & !pull) {
    // we use signals, so we just have to copy the shadow to the actual value
    for (;VarIterator!=reg_vars.end();VarIterator++) {
      //      fprintf(stderr, "A %s\n", VarIterator->getName());
      switch (VarIterator->getType()) {
      case VAR_INT:
	VarIterator->setValue(*(int *)VarIterator->getShadow());
	break;
      case VAR_STR:
	VarIterator->setValue((char *)VarIterator->getShadow());
	break;
      }
    }
    return;
  }

  // if signals are not used 
  // send request message 
  HUpdateMessage *m=new HUpdateMessage(HMESG_VAR_REQ, 0);
  
  for (;VarIterator!=reg_vars.end();VarIterator++)
    m->set_var(*VarIterator);
  
  if (debug_mode)
    m->print();
  
  send_message(m,get_local_sock());
  delete m;
  
  
  HMessage *mesg = receive_message(get_local_sock());
  
  if (mesg->get_type()==HMESG_FAIL) {
    // the server could not return the value of the variable
    delete mesg;
    fprintf(stderr, "Harmony error: Server could not return value of variable!\n");
    return;
  }
   
   m=(HUpdateMessage *)mesg;
   
   // first update the timestamp
   hclient_timestamp = m->get_timestamp();

   if (debug_mode)
     m->print();

   // update variables from the content of the message
   int i;

   for (VarIterator=reg_vars.begin(),i=0;VarIterator!=reg_vars.end();VarIterator++,i++) {
   switch (VarIterator->getType()) {
   case VAR_INT:
     VarIterator->setValue(*(int *)m->get_var(i)->getPointer());
     break;
   case VAR_STR:
     VarIterator->setValue((char *)m->get_var(i)->getPointer());
     break;
   }
   }
   delete m;
}



/*
 * Send the performance function result to the harmony server
 */
void harmony_performance_update(int value){
  VarDef *v = new VarDef("obsGoodness",VAR_INT);

  v->setValue(value);
  
  HUpdateMessage *m=new HUpdateMessage(HMESG_PERF_UPDT, 0);

  // first set the timestamp to be sent to the server
  m->set_timestamp(hclient_timestamp);

  m->set_var(*v);
  
  if (debug_mode)
    m->print();
  
  send_message(m,get_local_sock());
  delete m;
  
  // I am not sure that you should actually wait for confirmation
  // for now we will not wait for reply. However, we might change this
  // in the future
  
  // wait for the answer from the harmony server
  
  HMessage *mesg=(HMessage *)receive_message(get_local_sock());
  
  if (mesg->get_type()==HMESG_FAIL) {
    // could not send performance function 
    delete mesg;
    hc_exit("Failed to send performance function!");
  } 
    
  delete mesg;
  
}


struct varnode {

	char *appname;
	char *bundlename;
	jint type;
	void *variable;
	struct varnode *next;
	struct varnode *prev;
};


struct varnode *varlist;

JNIEXPORT void JNICALL Java_Harmony_Startup (JNIEnv *env, jobject obj, jint use_signals) {

	InitDQ(varlist,struct varnode);
	harmony_startup(use_signals);
	return ;
}

JNIEXPORT void JNICALL Java_Harmony_ApplicationSetup(JNIEnv *env, jobject obj, jstring st) {

	const char *str = env->GetStringUTFChars(st, 0);
        //printf("%s\n", str);
	harmony_application_setup((char *)str);
        env->ReleaseStringUTFChars( st, str);

	return;

}


JNIEXPORT jint JNICALL Java_Harmony_AddVariableINT (JNIEnv *env, jobject obj, jstring appName, jstring bundleName /*, jint type*/) {

	struct varnode *p,*q;
	jint value;
	const char *app = env->GetStringUTFChars(appName,0);
	const char *bundle = env->GetStringUTFChars(bundleName,0);

	for(p=varlist->next;p!=varlist;p=p->next) {

                if((strcmp(p->appname,(char *)app)==0) && \
			(strcmp(p->bundlename,bundle)==0)) {
			/* duplicated adding variable case */
				value=*(int *)(p->variable);
				//return value;
		}
	}

	if(p==varlist) { // adding new variable
		q=(struct varnode *)malloc(sizeof(struct varnode));
		q->appname=strdup(app);
		q->bundlename=strdup(bundle);
		q->type=VAR_INT;
		q->variable=harmony_add_variable((char *)app,(char *)bundle,VAR_INT);
		value=*(int *)(q->variable);
		InsertDQ(varlist,q);
	}
	env->ReleaseStringUTFChars(appName,app);
	env->ReleaseStringUTFChars(bundleName,bundle);

	return value;

}

JNIEXPORT jstring JNICALL Java_Harmony_AddVariableSTR (JNIEnv *env, jobject obj, jstring appName, jstring bundleName /*, jint type */) {

        struct varnode *p,*q;
        jstring value;
        const char *app = env->GetStringUTFChars(appName,0);
        const char *bundle = env->GetStringUTFChars(bundleName,0);

        for(p=varlist->next;p!=varlist;p=p->next) {

                if((strcmp(p->appname,(char *)app)==0) && \
                        (strcmp(p->bundlename,bundle)==0)) {
                        /* duplicated adding variable case */
				value=env->NewStringUTF((const char *)(p->variable));
                                //return value;
                }
        }

        if(p==varlist) { // adding new variable
                q=(struct varnode *)malloc(sizeof(struct varnode));
                q->appname=strdup(app);
                q->bundlename=strdup(bundle);
                q->type=VAR_STR;
                q->variable=harmony_add_variable((char *)app,(char *)bundle,VAR_STR);
                value=env->NewStringUTF((const char *)(q->variable));
                InsertDQ(varlist,q);
        }
        env->ReleaseStringUTFChars(appName,app);
        env->ReleaseStringUTFChars(bundleName,bundle);

        return value;

}




JNIEXPORT void JNICALL Java_Harmony_SetVariableINT (JNIEnv *env, jobject obj, jstring appName, jstring bundleName, jint currentvalue) {

	struct varnode *p;
	const char *app = env->GetStringUTFChars(appName,0);
        const char *bundle = env->GetStringUTFChars(bundleName,0);

	for(p=varlist->next;p!=varlist;p=p->next) {

                if((strcmp(p->appname,(char *)app)==0) && \
                        (strcmp(p->bundlename,bundle)==0)) {
                        /* found */
				*(int *)(p->variable)=currentvalue;
				harmony_set_variable(p->variable);
                }
        }
	env->ReleaseStringUTFChars(appName,app);
        env->ReleaseStringUTFChars(bundleName,bundle);

	return;
}

JNIEXPORT void JNICALL Java_Harmony_SetVariableSTR (JNIEnv *env, jobject obj, jstring appName, jstring bundleName, jstring currentvalue) {

        struct varnode *p;
        const char *app = env->GetStringUTFChars(appName,0);
        const char *bundle = env->GetStringUTFChars(bundleName,0);
	const char *value = env->GetStringUTFChars(currentvalue,0);

        for(p=varlist->next;p!=varlist;p=p->next) {

                if((strcmp(p->appname,(char *)app)==0) && \
                        (strcmp(p->bundlename,bundle)==0)) {
                        /* found */
				free(p->variable);
                                p->variable=strdup(value);
                                harmony_set_variable(p->variable);
                }
        }
        env->ReleaseStringUTFChars(appName,app);
        env->ReleaseStringUTFChars(bundleName,bundle);

        return;
}


/*
JNIEXPORT void JNICALL Java_Harmony_SetAll
  (JNIEnv *, jobject);
*/


JNIEXPORT jint JNICALL Java_Harmony_RequestVariableINT (JNIEnv *env, jobject obj, jstring appName, jstring bundleName) {

        struct varnode *p;
        const char *app = env->GetStringUTFChars(appName,0);
        const char *bundle = env->GetStringUTFChars(bundleName,0);
	jint value;

        for(p=varlist->next;p!=varlist;p=p->next) {

                if((strcmp(p->appname,(char *)app)==0) && \
                        (strcmp(p->bundlename,bundle)==0)) {

				//bug implementation 
				// harmony_request_variable(p->variable);

				harmony_request_all();

				value=*(int *)(p->variable);

//printf("requested result =(%d)\n",*(int *)(p->variable));

                }
        }


        env->ReleaseStringUTFChars(appName,app);
        env->ReleaseStringUTFChars(bundleName,bundle);

	return value;
}


JNIEXPORT jstring JNICALL Java_Harmony_RequestVariableSTR (JNIEnv *env, jobject obj, jstring appName, jstring bundleName) {

        struct varnode *p;
        const char *app = env->GetStringUTFChars(appName,0);
        const char *bundle = env->GetStringUTFChars(bundleName,0);
        jstring value;

        for(p=varlist->next;p!=varlist;p=p->next) {

                if((strcmp(p->appname,(char *)app)==0) && \
                        (strcmp(p->bundlename,bundle)==0)) {

                                //harmony_request_variable(p->variable);
				harmony_request_all();
                                value=env->NewStringUTF((const char*)(p->variable));

                }
        }


        env->ReleaseStringUTFChars(appName,app);
        env->ReleaseStringUTFChars(bundleName,bundle);

        return value;
}


/*
JNIEXPORT void JNICALL Java_Harmony_RequestAll
  (JNIEnv *, jobject);
*/


JNIEXPORT void JNICALL Java_Harmony_PerformanceUpdate (JNIEnv *env, jobject obj, jint performance) {


//printf("performace update=%d\n",performance);
	harmony_performance_update(performance);

	return;
}

JNIEXPORT void JNICALL Java_Harmony_End (JNIEnv *env, jobject obj) {

	harmony_end();

	return;


} 
