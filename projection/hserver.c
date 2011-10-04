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

/***
 *
 * include user defined headers
 *
 ***/
#include "hserver.h"

/***
 *
 * Main variables
 *
 ***/

/* 
 * local variable
 * when value is 1 debug output is sent to stdout
 * if value is 0 there is no output 
 *
 */
static int debug_mode=0;

/*
 * the socket waiting for connections
 */
int listen_socket;

#ifdef LINUX
struct in_addr listen_addr;	/* Address on which the server listens. */
#endif

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
 * The Tcl Interpretor
 */
Tcl_Interp *tcl_inter;
char harmonyTclFile[25]="hconfig.tcl";


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

int update_client( ClientData clientData, Tcl_Interp *interp, int argc, char **argv) {

  fprintf(stderr, "************ Updating client!!! ****************\n");
  
  if (debug_mode) {
    printf("Variable : %s_bundle_%s has value %s\n",argv[1], argv[2], argv[3]);
    printf("ClientInfo[%s]=%d\n",argv[1], clientInfo[argv[1]]);
    printf("clientSignals[]=%d\n",clientSignals[clientInfo[argv[1]]]);
  }

  if (!clientSignals[clientInfo[argv[1]]])
    return TCL_OK;
  
  fprintf(stderr, "Do more stuff!\n");

  HUpdateMessage *m=new HUpdateMessage(HMESG_VAR_REQ, 0);

  // i am not sure of the use of the va variable for now
  // it looks like it counts the number of params 
  // it assumes I guess that you would call this function
  // with more than one variable
  // update_client(var1, val1, var2, val2 ...)
  int va = 2;

  while (va < argc) {
    char *varname = (char *)malloc(strlen(argv[1])+strlen(argv[va])+8);
    int vartype;
    VarDef *v;
    char **errorp=(char **)malloc(sizeof(char *));
  
    sprintf(varname, "%s", argv[va]); 
    strtol(argv[va+1], errorp, 10) ;
  
    if ((errorp==NULL) || (**errorp=='\0')) 
      vartype=VAR_INT;
    else 
      vartype=VAR_STR;
  
    v = new VarDef(varname, vartype);
  
    switch (vartype) {
    case VAR_INT:
      v->setValue(strtol(argv[va+1],NULL, 10));
      break;
    case VAR_STR:
      v->setValue(argv[va+1]);
      break;
    }
    
    m->set_var(*v);

    va += 2;

    free(v);
    free(varname);
    free(errorp);
  }
  //  m->print();

  send_message(m, clientInfo[argv[1]]);

  if (!strcmp(application_trigger, argv[1]))
    update_sent = TRUE;

  free(m);
  return TCL_OK; 
}


/***
 *
 * Start the server by initializing sockets and everything else that is needed
 *
 ***/
void server_startup() {
  
  /* temporary variable */
  char s[10];

  for(int i = 1; i <= 100; i++)
    id_set.push_back(i);

  /* used address */
  struct sockaddr_in sin;

  int optVal;

  int err;


  listen_port = SERVER_PORT;

  /* update environment variables */

  /*
    setenv("HARMONY_S_HOST","localhost",1);
    sprintf(s,"%d",SERVER_PORT);
    setenv("HARMONY_S_PORT",s,1);
    */


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

  /* Now we have to initialize the Tcl interpretor */
  tcl_inter = Tcl_CreateInterp();

  if ((err=Tcl_Init(tcl_inter)) != TCL_OK) {
    h_exit("Tcl Init failed!");
  }

#ifdef USE_TK
  if ((err=Tk_Init(tcl_inter)) != TCL_OK) {
    printf("Error message: %s\n",tcl_inter->result);
    h_exit("Tk Init failed!");
  }

  Tk_MapWindow(Tk_MainWindow(tcl_inter));
#endif /*USE_TK*/

  if ((err=Tcl_EvalFile(tcl_inter, harmonyTclFile)) != TCL_OK){
    printf("Tcl Error %d ; %s\n",err, tcl_inter->result);
    h_exit("TCL interpretor ");
  } 


  /*
    register update_client function with the Tcl interpretor
  */
  Tcl_CreateCommand(tcl_inter, "update_client", update_client, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);


  /*
    Set the file descriptor set
  */

  FD_ZERO(&listen_set);

  FD_SET(listen_socket,&listen_set);
  socket_set.push_back(listen_socket);
  highest_socket=listen_socket;


  /***
      get the fd that X is using for events
  ***/
#ifdef USE_TK  
  Display *UIMdisplay = Tk_Display (Tk_MainWindow(tcl_inter));
  xfd = XConnectionNumber (UIMdisplay);
  
  FD_SET(xfd,&listen_set);
  socket_set.push_back(xfd);
  if (xfd>highest_socket)
    highest_socket=xfd;
#else 
  FD_SET(0, &listen_set);
#endif /*USE_TK*/
  
}



/***
 *
 * Daemon registration adds the daemon information into the database
 *
 ***/
void daemon_registration(struct sockaddr_in address, int addr_len){
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

  if (id) {
    /* the client is relocating its communicating agent */
    /* the server dissacociates the old socket with the client, and
     * uses the new one instead */

    int cs = clientSocket[id];
    clientSocket[id] = client_socket;

    char *appName = clientName[cs];    
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

}

/***
 *
 * Node description gets the information about the node from the daemon
 *
 ***/
void get_node_description(struct sockaddr_in address, int addr_len){
}

/***
 *
 * Gets the application information from the client
 *
 ***/

void get_appl_description(HDescrMessage *mesg, int client_socket){
  int err;
  char *new_string=(char *)malloc(strlen(mesg->get_descr())+10);
  strcpy(new_string, mesg->get_descr());

  sprintf(strrchr(new_string, '}')+1, " %d", clientId[client_socket]);

  //  printf("Message size: %d\n",mesg->get_message_size());

  if ((err=Tcl_Eval(tcl_inter,new_string))!=TCL_OK) {
    printf("Error %d interpreting the tcl description!\n",err);
    printf("%s\n",tcl_inter->result);
    printf("[%s]\n",new_string);
    free(new_string);
    operation_failed(client_socket);
  }
  else {
        printf("111Send confimation !\n");
    /* send confirmation to client */
    HRegistMessage *m = new HRegistMessage(HMESG_CONFIRM, 0);
    
    send_message(m,client_socket);
    
    delete m;

    
    //    printf("Now get application name!\n");

    // add the information about the client in the clienInfo map
    
    // first just get the name of the application, so that we can link it 
    // with the socket #
    char *startp, *endp;
    
    startp=new_string;
    // get rid of begining spaces
    while (*startp==' ') startp++;
    // go to first space
    while (*startp!=' ') startp++;
    // get rid of separating spaces
    while (*startp==' ') startp++;
    // startp points now to the first char in the name of the application
    endp=startp;
    while (*endp!=' ') endp++;
    //    endp--;
    // endp points to the last char in the ame of the application
    
    //    printf("Got begining and end of name!\n");

    char appName[256];

    strncpy(appName, startp, (endp-startp));
    sprintf(&appName[endp-startp], "_%d", clientId[client_socket]);
    
    //    printf("Application name: %s\n", appName);
    // add info to the map
    clientInfo[appName]=client_socket;
    clientName[client_socket] = (char *)malloc(sizeof(appName));
    strcpy(clientName[client_socket], appName);

    free(new_string);
  }
  
}

/***
 *
 * Registers the variables from the client into the database
 *
 ***/
void variable_registration(HDescrMessage *mesg, int client_socket){

  char *s=NULL;
  //  char *ss=NULL;
  char **sss=(char **)malloc(sizeof(char *));

  //  printf("Variable registration!\n");

  char  *appName = clientName[client_socket];
  //  printf("%s\n", appName);

  /*
    ss=(char *)malloc(strlen(mesg->get_descr())+7+sizeof(appName)+8);
    char *ss2=(char *)malloc(strlen(mesg->get_descr())+7+sizeof(appName)+8);
  */
  char ss[256], ss2[256];
  sprintf(ss2,"%s(value)",mesg->get_descr());

  //  printf("%s\n",ss2);
  sprintf(ss, "%s_bundle_%s", appName, ss2);
  //  printf("%s\n",ss);
  // free(ss);


  if ((s=Tcl_GetVar(tcl_inter,ss,0))==NULL) {
    printf("Variable %s not found!\n", ss);
    free(sss);
    //    free(ss);

    operation_failed(client_socket);
    return;
  }

  //  printf("Result : %s\n",s);

  HUpdateMessage *m=new HUpdateMessage(HMESG_VAR_REQ,0);
  
  strtol(s,sss,10);

  VarDef *v;
  
  if (strlen(*sss)==0) {
    /* is int */
    //    printf("Var is INT: %s\n", mesg->get_descr());
    v=new VarDef(mesg->get_descr(),VAR_INT);
    //    printf("Var Def OK!\n");
    v->setValue(strtol(s,NULL,10));
    //    printf("Value set\n");
  }
  else {
    //    printf("Var is STR!\n");
    v=new VarDef(mesg->get_descr(),VAR_STR);
    //    printf("Type set!\n");
    v->setValue(s);
    //    printf("Value set!\n");
  }

  m->set_var(*v);

  //  printf("Var set \n");

  //  m->print();

  //  printf("Send update!\n");

  send_message(m, client_socket);

  //  printf("Update sent!\n");

  free(sss);
  //  free(ss);

  fprintf(stderr, "1\n");
  delete m;

}

/***
 *
 * Return the value of the variable from the database
 *
 ***/
void variable_request(HUpdateMessage *mesg, int client_socket){

  char *s;
  char ss[256], ss2[256];

  VarDef v;

  //  printf("\n\n\nVariable request!\n");

  for (int i=0; i<mesg->get_nr_vars();i++) {

    sprintf(ss2,"%s(value)",mesg->get_var(i)->getName());
    //    printf("%s\n",ss2);
    
    char  *appName = clientName[client_socket];
    //    printf("%s\n", appName);

    sprintf(ss, "%s_bundle_%s", appName, ss2);
        printf("[%s]\n",ss);

    s=Tcl_GetVar(tcl_inter,ss,0);

    if (s==NULL) {
      printf("Error getting value of variable!");
      free(ss);
      operation_failed(client_socket);
      return;
    }

    printf("Result : %s\n",s);
    
    VarDef *v;
    
    switch (mesg->get_var(i)->getType()) {
    case VAR_INT:
      /* is int */
      //      printf("Var %d is INT ; %d\n", i, strtol(s, NULL, 10));
      mesg->get_var(i)->setValue(strtol(s,NULL,10));
//      mesg->get_var(i)->print();
      break;
    case VAR_STR:
      //      printf("Var is STR!\n");
      mesg->get_var(i)->setValue(s);
      break;
    }
    
  }

  char  *appName = clientName[client_socket];
  
  sprintf(ss, "%s_simplex_time", appName);

  s=Tcl_GetVar(tcl_inter,ss,0);

  if (s==NULL) {
    printf("Error getting value of variable!");
    free(ss);
    operation_failed(client_socket);
    return;
  }

  mesg->set_timestamp(strtol(s,NULL,10));

  //  mesg->print();

  //  printf("Send update!\n");
  
  send_message(mesg, client_socket);

  //  printf("Update sent!\n");

  //  getchar();
}




/***
 *
 * Set the value of the variable into the database
 *
 ***/
void variable_set(HUpdateMessage *mesg, int client_socket){

  char *s;
  char *ss;
  int resint;

  //  printf("\n\nVariable set!\n");
  //  mesg->print();

  for (int i=0; i<mesg->get_nr_vars();i++) {

    ss=(char *)malloc(strlen(mesg->get_var(i)->getName())+7);
    sprintf(ss,"%s",mesg->get_var(i)->getName());
    
    switch (mesg->get_var(i)->getType()) {
    case VAR_INT:
      s=(char *)malloc(100);
      sprintf(s,"%d",*(int *)mesg->get_var(i)->getPointer());
      break;
    case VAR_STR:
      s=(char *)malloc(strlen((char *)mesg->get_var(i)->getPointer()));
      sprintf(s,"%s",(char *)mesg->get_var(i)->getPointer());
      break;
    }

    char *appName = clientName[client_socket];

    char ss2[256];
    sprintf(ss2, "%s_bundle_%s", appName, ss);

    //    printf("appName: %s  bundle: %s value: %s (%s)\n", appName, mesg->get_var(i)->getName(), s, ss2);

    char *res= "";
    // for  some malloc problem, the following does not work
    //    if (Tcl_VarEval(tcl_inter,"set ",ss2, " ", s, NULL)!=TCL_OK) {    
    if ((res = Tcl_SetVar2(tcl_inter, ss2, "value", s, TCL_GLOBAL_ONLY)) == NULL) {
      fprintf(stderr, "Error setting variable: %s", res);
      fprintf(stderr, "TCLres (varSet-Err): %s\n", tcl_inter->result);
      free(s);
      free(ss);
      operation_failed(client_socket);
      return;
    }
    fprintf(stderr, "TCLres(varSet-OK): %s ; %s ; %s\n", ss2, res, tcl_inter->result);

    char redraw_str[256];
    sprintf(redraw_str,"redraw_dependencies %s %s 0 0",mesg->get_var(i)->getName(), appName);
    if ((resint = Tcl_Eval(tcl_inter, redraw_str))==TCL_ERROR) {
      fprintf(stderr, "Error running redraw_dependencies: %s", resint);
      fprintf(stderr, "TCLres(varSet-Err2): %s\n", tcl_inter->result);
      free(s);
      free(ss);
      operation_failed(client_socket);
      return;
    }
    fprintf(stderr, "TCLres(varSet-OK2): %s ; %s\n", resint, tcl_inter->result);

    //I had to comment the scheduler part since it was not working properly
    // Cristian

    //#define SCHEDULER
#ifdef SCHEDULER
#ifdef notdef
    char *c;
    c = strchr(ss, '_');
    assert(c);
    *c = '\0';
    char *appName = ss;
    c += 8;
    char *vname = c;
#endif /*notdef*/


    char StartEvent[] = "START";
    char EndEvent[] = "END";
    char SyncEvent[] = "SYNC";
    char VarEvent[] = "VAR";
    char *event;

    if (!strcmp(mesg->get_var(i)->getName(), "AdaState")){
      fprintf(stderr, "%s\n", s);
      if (!strcmp(s, "APP_STARTED")) {
        /* register application in scheduler */
	//	printf("* START event\n");
	event = StartEvent;
      }else{
        /* call scheduler to take over */
	//        printf("* SYNC event\n");
	event = SyncEvent;
      }
    }
    else{
      //        printf("* VAR event\n");
	event = VarEvent;
    }

    application_trigger = appName;
    update_sent = FALSE;
    if (Tcl_VarEval(tcl_inter, "SchedulerEventHandler ", appName, " ", event, NULL)
	!= TCL_OK){
      fprintf(stderr, "SchedulerEventHandler failed: %s\n", tcl_inter->result);
      free(s); free(ss);
      operation_failed(client_socket);
    }


#elif SCHEDULER2

    char *c;
    c = strchr(ss, '_');
    assert(c);
    *c = '\0';
    appName = ss;
    c += 8;
    char *vname = c;

    //    char *appName = scheduler.socket_to_app(client_socket);
    //	char *sname = (char *)malloc(strlen(appName)+25);
    //      sprintf(sname, "%s_bundle_AdaState", appName);
    //    }

    if (!strcmp(vname, "AdaState")){
      if (!socket_to_app(sock)){
	/* register application in scheduler */
	app_start(appName, client_socket);
      }
      else{
	/* call scheduler to take over */
	scheduler.app_sync(client_socket);
      }
    }
    else
      scheduler.app_var(client_socket);

    free(sname);
#endif

    free(s);
    free(ss);
  } 

  //  printf("Send Confirmation!\n");

  if (!update_sent) {
    HRegistMessage *m = new HRegistMessage(HMESG_CONFIRM,0);
    send_message(m, client_socket);
    delete m;
  }
  update_sent = FALSE;
  //  printf("Set Variable Completed!\n");
    
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

  
  if (Tcl_VarEval(tcl_inter, "SchedulerEventHandler ", appName, " END", NULL) 
      != TCL_OK) {
    fprintf(stderr, "revoke_resources: ERROR in unsetting related tcl vars: %s\n"
	    , tcl_inter->result);
  }
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

  /* first run the Tcl function to clear after the client */
  if (Tcl_VarEval(tcl_inter,"harmony_cleanup ",clientName[client_socket], NULL)!=TCL_OK) {
    fprintf(stderr, "Error cleaning up after client! %d\n", tcl_inter->result);
  }

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


void performance_update(HUpdateMessage *m, int client_socket) {

  int err;

  char *appName = clientName[client_socket];
  
  char s[256];
  sprintf(s, "updateObsGoodness %s %d %d", appName,*(int *)m->get_var(0)->getPointer(), m->get_timestamp() );

  if ((err = Tcl_Eval(tcl_inter, s)) != TCL_OK) {
    fprintf(stderr, "Error setting performance function: %d", err);
    fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
    operation_failed(client_socket);
    return;
  }
  fprintf(stderr, "TCLres(perfUpdate):  %s\n", tcl_inter->result);

  send_message(m,client_socket);
}



int in_process_message = FALSE;
void process_message_from(int temp_socket){
  
  HMessage *m;
  assert(!in_process_message);
  in_process_message = TRUE;
  //  printf("Waiting...%d...%d...\n",getpid(),temp_socket);
  
  /* get the message type */
  m=receive_message(temp_socket);
  
  if (m == NULL){
    client_unregister((HRegistMessage*)m, temp_socket);
    in_process_message = FALSE;
    return;
  }

  //  printf("Message has type: %d\n", m->get_type());


  //Tcl_VarEval(tcl_inter, "puts ", "$ADR_bundle_test(value) ", NULL);
  
  switch (m->get_type()) {
  case HMESG_DAEMON_REG:
    //	daemon_registration(temp_addr, temp_addrlen);
    break;
  case HMESG_CLIENT_REG:
    client_registration((HRegistMessage *)m,temp_socket);
    break;
  case HMESG_NODE_DESCR:
    // get_node_description(temp_addr,temp_addrlen);
    break;
  case HMESG_APP_DESCR:
    get_appl_description((HDescrMessage *)m, temp_socket);
    break;
  case HMESG_VAR_DESCR:
    variable_registration((HDescrMessage *)m, temp_socket);
    break;
  case HMESG_VAR_REQ:
    variable_request((HUpdateMessage *)m, temp_socket);
    break;
  case HMESG_VAR_SET:
    variable_set((HUpdateMessage *)m, temp_socket);
    break;
  case HMESG_PERF_UPDT:
    performance_update((HUpdateMessage *)m, temp_socket);
    break;
  case HMESG_CLIENT_UNREG:
    client_unregister((HRegistMessage *)m, temp_socket);
    break;
 
  default:
    printf("Wrong message type!\n");
  }


  /* process Tk events */
  while (Tcl_DoOneEvent(TCL_DONT_WAIT)>0)
    ;

  in_process_message = FALSE;
}


/***
 *
 * The main loop of the server
 *
 * We only exit on Ctrl-C
 *
 ***/
void main_loop() {

  struct sockaddr_in temp_addr;
  int temp_addrlen;
  int temp_socket;
  int pid;

  int active_sockets;
  
  fd_set listen_set_copy;


  //  printf("Starting loop!\n");

  
  /* do listening to the socket */
  if (listen(listen_socket, 10)) {
    h_exit("Listen to socket failed!");
  }

    printf("Listening to port!\n");

 
   
  while (1) {
    listen_set_copy=listen_set;
        printf("Select\n");
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
      //      fprintf(stderr, "accepted a client\n");
      process_message_from(temp_socket);
    }

#ifdef USE_TK    
    if (FD_ISSET(xfd, &listen_set_copy)) {
      // we have to process Tk events
      while (Tcl_DoOneEvent(TCL_DONT_WAIT)>0)
	;
    }
#else
    if (FD_ISSET(xfd, &listen_set_copy)) {
      char line[1024];
      int ret1 = read(0, line, sizeof(line));
      assert (ret1 >= 0);
      line[ret1] = '\0';
      
      if (Tcl_Eval(tcl_inter, line) != TCL_OK) {
	printf("Tcl Error: %s\n", tcl_inter->result);
      } else {
	//	printf("%s\n", tcl_inter->result);
      }
      //      printf("harmony %% ");
      
      fflush(stdout);
    }
#endif /*USE_TK*/

    //    /* we have a communication request */ 
    //    for (socketIterator=socket_set.begin();socketIterator!=socket_set.end();socketIterator++) {
    //      if (FD_ISSET(*socketIterator, &listen_set_copy) && (*socketIterator!=listen_socket) && (*socketIterator!=xfd)) {
    //	/* we have data from this connection */
    //	process_message_from(*socketIterator);
    //      }
    //    }
  }
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

  server_startup();

  main_loop();
}











