/***
 *
 * include user defined headers
 *
 ***/
#include "hserver.h"
#include <iostream>
#include <assert.h>
#include <sstream>
#include <set>
#include <deque>
using namespace std;
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
static int debug_mode=1;
static int hserver_start_timestamp=0;
/*
 * the socket waiting for connections
 */
int listen_socket;
#define INTMAXVAL 2147483647
#define FLTMAXVAL 1E+37
#ifdef LINUX
struct in_addr listen_addr;     /* Address on which the server listens. */
#endif

#define TRUE 1
#define FALSE 0

/*
 * the port waiting for connections
 */
int listen_port;
fd_set listen_set;
list<int> socket_set;
//vector<int> code_timesteps;
int highest_socket;
list<int>::iterator socketIterator;

list<int> id_set;

/* the X file descriptor */
int xfd;

/*
 * The Tcl Interpreter
 */
Tcl_Interp *tcl_inter;

//list<Tcl_Interp*> tcl_inter;


char harmonyTclFile[256]="hconfig.pro.tcl";

/*
  the map containing client AppName and Socket
*/
map<string, int> clientInfo;
map<int, char *> clientName;
map<int, int> clientSignals;
map<int, int> clientSocket;
map<int, int> clientId;

/*
 * code generation specific parameters
*/
map<string, int> code_timesteps;
map<string, int> last_code_req_timesteps;
map<string, int> last_code_req_responses;

/* this map contains the configurations that we have seen so far and
   their corresponding performances.
 */
map<string,string> global_data;
map<string,string>::iterator it;

/***
 *
 * Check the arguments given to the server
 *
 ***/
void check_parameters(int argc, char **argv) {

    // by default, we use PRO search algorithm
    int search_algo=1;

    // by default, no windows are drawn
    int draw_windows=0;

    if( argc >= 2 )
    {

        // first argument is the search algorithm
        search_algo=atoi(argv[1]);
    }
    if( argc == 3)
    {
        // second argument is the graphics parameter
         draw_windows=atoi(argv[2]);
    }

    if(search_algo==1)
    {
        sprintf(harmonyTclFile,"hconfig.pro.tcl");
        printf("using the PRO algorithm. Please make sure the parameters in \n");
        printf("../tcl/pro_init_<appname>.tcl are defined properly.\n");
    }
    if(search_algo==2)
    {
        printf("using the Nelder Mead Simplex Algorithm. \n");
        sprintf(harmonyTclFile,"hconfig.nm.tcl");
    }
    if(search_algo == 3)
    {
        printf("using random Algorithm. \n");
        sprintf(harmonyTclFile,"hconfig.random.tcl");
    }
    if(search_algo == 4)
    {
        printf("using brute force Algorithm. \n");
        sprintf(harmonyTclFile,"hconfig.brute.tcl");
    }
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

    if (debug_mode) {
        printf("************ Updating client!!! ****************\n");
        printf("Variable : %s_bundle_%s has value %s\n",argv[1], argv[2], argv[3]);
        printf("ClientInfo[%s]=%d\n",argv[1], clientInfo[argv[1]]);
        printf("clientSignals[]=%d\n",clientSignals[clientInfo[argv[1]]]);
    }

    if (!clientSignals[clientInfo[argv[1]]])
        return TCL_OK;

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

    for(int i = 1; i <= 2048; i++)
        id_set.push_back(i);

    /* used address */
    struct sockaddr_in sin;

    int optVal;

    int err;
    char * serv_port;
    // try to get the port info from the environment
    serv_port=getenv("HARMONY_S_PORT");
    if (serv_port != NULL)
    {
        listen_port=atoi(serv_port);
    } else
    {
        // if the env var is not defined, use the default
        listen_port = SERVER_PORT;
    }

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
#endif  /*LINUX*/
    sin.sin_port = htons(listen_port);

    /* Bind the socket to the desired port. */
    if (bind(listen_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        h_exit("Error binding socket!");
    }

    /* Now we have to initialize the Tcl interpreter */
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
#endif  /*USE_TK*/


    if ((err=Tcl_EvalFile(tcl_inter, harmonyTclFile)) != TCL_OK){
        printf("Tcl Error %d ; %s %s \n",err, tcl_inter->result, harmonyTclFile);
        h_exit("TCL Interpreter Error ");
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
        getthe fd that X is using for events
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
#endif  /*USE_TK*/

}

/***
 *
 * Daemon registration adds the daemon information into the database
 *
 ***/
void daemon_registration(struct sockaddr_in address, int addr_len){
    // not implemented
}

/***
 *
 * Client registration adds the client information into the database
 *
 ***/
void client_registration(HRegistMessage *m, int client_socket){

    if (debug_mode) {
        printf("Client registration! (%d)\n", client_socket);
    }

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
        //printf("This is a new client \n");

        id = id_set.front();
        id_set.pop_front();
        clientSocket[id] = client_socket;
    }

    clientId[client_socket] = id;
    if (debug_mode) {
        printf("Send registration (id %d) confimation !\n", id);
    }

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

    if (debug_mode) {
        printf("Message size: %d\n",mesg->get_message_size());
    }
    
    if ((err=Tcl_Eval(tcl_inter,new_string))!=TCL_OK) {
        printf("Error %d interpreting the tcl description!\n",err);
        printf("%s\n",tcl_inter->result);
        printf("[%s]\n",new_string);
        free(new_string);
        operation_failed(client_socket);
    }
    else {
        /* send confirmation to client */
        HRegistMessage *m = new HRegistMessage(HMESG_CONFIRM, 0);
        send_message(m,client_socket);
        delete m;

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

        char appName[256];
        char temp_name[256];

        strncpy(appName, startp, (endp-startp));
        strncpy(temp_name, startp, (endp-startp));
        sprintf(&temp_name[endp-startp], "");

        code_timesteps[temp_name]=1;
        last_code_req_timesteps[temp_name]=0;
        last_code_req_responses[temp_name]=0;

        //printf("initial value :::::::::::code_timestep: %d \n", code_timesteps[temp_name]);
        sprintf(&appName[endp-startp], "_%d", clientId[client_socket]);
        if (debug_mode) {
            printf("Application name: %s\n", appName);
        }

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
    char **sss=(char **)malloc(sizeof(char *));

    char  *appName = clientName[client_socket];

    // this is one of the most weirdest ways to name the variables!
    char ss[256], ss2[256];
    sprintf(ss2,"%s(value)",mesg->get_descr());
    sprintf(ss, "%s_bundle_%s", appName, ss2);

    if(debug_mode)
        printf("tcl string : %s \n", ss);

    if ((s=Tcl_GetVar(tcl_inter,ss,0))==NULL) {

        free(sss);
        operation_failed(client_socket);
        return;
    }
    if(debug_mode)
        printf("tcl value : %s \n", s);

    HUpdateMessage *m=new HUpdateMessage(HMESG_VAR_REQ,0);
    strtol(s,sss,10);

    VarDef *v;

    if (strlen(*sss)==0) {
        /* is int */
        v=new VarDef(mesg->get_descr(),VAR_INT);
        v->setValue(strtol(s,NULL,10));
    }
    else {
        v=new VarDef(mesg->get_descr(),VAR_STR);
        v->setValue(s);
    }

    m->set_var(*v);

    send_message(m, client_socket);

    free(sss);
    delete m;

}



/***
 *
 * Return the value of the variable from the database
 *
 ***/
void variable_request(HUpdateMessage *mesg, int client_socket)
{

    char *s;
    char *s_new;
    char ss[256], ss2[256];
    char s_updt[128];
    char *appName = clientName[client_socket];
    
    char new_config[80];
    char new_configuration[80];
    int err;

    char current_config[80];
    char* return_value;

    char new_res[100];
    
    VarDef v;

    // step 1: whats happening here is that you are updating the values for the tunable parameters.
    //  you get the values from the tcl backend using the Tcl_GetVar and you are setting the value of the
    //  variables in the message to whatever value you get from the tcl backend.
    for (int i=0; i<mesg->get_nr_vars();i++) 
    {
        // get the name of the parameter. e.g. x or y ...
        sprintf(ss2,"%s(value)",mesg->get_var(i)->getName());
    
	//char  *appName = clientName[client_socket];
	// constructing the tcl variable name: the way variables are represented in the backend is as follows:
	//  <appname>_<client socket>_bundle_<name of the variable>
	//  for c++ example: 
	//    appname is SimplexT
	//    name of the variables is x and y.
	//    if you are running 4 clients: client socket goes from 1 to 4.
	//   An example of the variable name in the backend will be:
	//    SimplexT_1_bundle_x
	// Now in the tcl backend, this variable has attributes:
	//  SimplexT_1_bundle_x(value)
    
	sprintf(ss, "%s_bundle_%s", appName, ss2);
	
        if(debug_mode)
            printf(" tcl value string: %s \n", ss);
	
	// you are invoking the tcl interpreter to parse this tcl command.
        s=Tcl_GetVar(tcl_inter,ss,0);
        
	if(debug_mode)
            printf("tcl value : %s \n", s);
        
	if (s==NULL) 
    	{
            printf("Error getting value of variable!\n");
            free(ss);
	    //free(s);
	    operation_failed(client_socket);
            return;
        }
        
	VarDef *v;
	
	// get the VarDef object at position i in the message. and check its type.
        switch (mesg->get_var(i)->getType()) 
    	{
	    // for c++ example, this will be always int.
            case VAR_INT:
                /* is int */
	        // update the value
                mesg->get_var(i)->setValue(strtol(s,NULL,10));
                break;
            case VAR_STR:
                mesg->get_var(i)->setValue(s);
                break;
        }
    }


    // construct the key using get_test_configuration (look at the database function)
    // if the conf is in the database, either change the message type or construct a brand new message    
    // object. (set its type to already_evaluated)... otherwise, do the usual
    
    sprintf(s, "get_test_configuration %s", appName);

    if ((err = Tcl_Eval(tcl_inter, s)) != TCL_OK) 
    {
      fprintf(stderr, "Error retreiving the current configuration: %d", err);
      fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
      printf("FAILED HERE\n");
      operation_failed(client_socket);
      return;
    } 
    else
    {
      char *startp, *endp;
      startp=appName;
      endp=appName;
      while (*endp!='_') endp++;
       char temp_name[256];
       strncpy(temp_name, startp, (endp-startp));
       sprintf(&temp_name[endp-startp], "");
       sprintf(current_config, "%s_%s", temp_name, tcl_inter->result);
       printf("Checking the map for %s \n", current_config);
    
      printf("Current configuration: %s \n", current_config);
    }
    
    ///////////////////////////////////////DATABASE CHECK/////////////////////////////////////

    //check the database whether "current_config" is already available
    string str_config = current_config;
    
    it=global_data.find(str_config);
    
    // this is the case where the performance is not in the database.
    // if the iterator is at the end of the map, we never found the conf in the database.
    //  so simply use the old code (from the point after setting the values of the variables).
    if(it == global_data.end())
    {
      //The database search has concluded till the end and did not find the configuration
      
      sprintf(ss, "%s_simplex_time", appName);
      if(debug_mode)
        printf("Timestamp tcl string: %s \n", ss);
      
      s=Tcl_GetVar(tcl_inter,ss,0);
      if(debug_mode)
        printf("Timestamp app: %s \n", s);

      if (s==NULL) {
        printf("Error getting value of variable! ss:: %s \n", ss);
        free(ss);
        operation_failed(client_socket);
        return;
      }
      if(debug_mode)
        printf("Value of the timestamp from the server: %s \n", s);
      mesg->set_timestamp(strtol(s,NULL,10));
      send_message(mesg, client_socket);
    }
    else
    {
      // this is the case where the configuration->performance mapping exists in the database.
      
      //  server contacts the TCL backend and gets the next configuration
      //  and sends the updated configuration to the client.
      //  The clients will not have any idea about the server's interaction with the database
      //  All they do is simply evaluate the "new config" that the server gives and sends the performance back.


      // 1. Extract the performance from the database. extract the value from the database. it->second
      // 2. Construct a VarDef
      // 3. Construct a new Message with a new type. (already_evaluated type)
      // 4. Go through the message received as an argument to this function 
      //    using " for (int i=0; i<mesg->get_nr_vars();i++) " construct.
      //    and copy over the VarDef's one by one to the new message.
      //    using the set_var function.
      // 5. Finally, put the VarDef for database performance at the end of the VarDefs.
      // 6. Send the message.

      int new_value;
      //int err;
      //char* return_value;

      char *s_new;
      //char s_next[80];
      char ss_new[256], ss2_new[256];

      printf("The following configuration has seen before \n");
      
      printf("For now updating the performance and sending back to the client\n");
     	
     

      // Contacting the TCL backend for the next config to be evaluated
      // tcl proc get_next_configuration appName returns the next configuration to be evaluated
      
      //sprintf(s_next, "get_next_configuration %s", appName);
      
      //if((err = Tcl_Eval(tcl_inter, s_next)) != TCL_OK)
      //{
      //fprintf(stderr, "Error retrieving the next configuration: %d", err);
      //fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
      //printf("FAILED TO GET THE NEXT CONFIGURATION \n");
      //operation_failed(client_socket);
      //return;
      //}
      //else
      //{
      //char *new_startp, *new_endp;
      //new_startp=appName;
      //new_endp=appName;
      //while(*new_endp!='_') new_endp++;
      //char new_temp_name[256];
      //strncpy(new_temp_name, new_startp, (new_endp-new_startp));
      //sprintf(&new_temp_name[new_endp-new_startp], "");
      //sprintf(new_configuration, "%s_%s", new_temp_name, tcl_inter->result);
      //}
      
      //string new_str_conf = new_configuration;

      //stringstream ss_new_conf;
      //ss_new_conf.str("");
      //ss_new_conf << (*it).second;
      //return_value = (char*)ss_new_conf.str().c_str();
      
      //constructing a new message for sending back to the client
      //HUpdateMessage *msg = new HUpdateMessage(HMESG_PERF_ALREADY_EVALUATED, 0);

      //switch (msg->get_var(0)->getType())
      //{
      // case VAR_INT:
      //      msg->get_var(0)->setValue(atoi(return_value));
      //      break;
      // case VAR_STR:
      //      msg->get_var(0)->setValue(return_value);
      //      break;
      //}

      //send_message(msg, client_socket);
      //printf("Server is sending the message of type : %d \n", msg->get_type());
      //delete msg;
      //delete mesg;
      //}
     
         
      // Extracting the performance from database.
      // this performance value has to come from the map.
     string updated_performance;
     int updt_perf;
      
     //getting it from the map
     updated_performance = (*it).second;

      //This converts the perf(string) to perf(int)
      updt_perf = atoi(updated_performance.c_str());

      printf("performance from the database: %d \n", updt_perf);
      
      printf("YAHOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO  \n");

      //constructing a new VarDef
      VarDef *updated_perf = new VarDef("perf", VAR_INT);
      updated_perf->setValue(updt_perf);

      //constructing a new message for sending back to the client
      HUpdateMessage *msg = new HUpdateMessage(HMESG_PERF_ALREADY_EVALUATED, 0);

      for (int i=0; i<mesg->get_nr_vars();i++)
      {
	// extract the VarDef at i.
	VarDef *v_temp = mesg->get_var(i);

	// set the VarDef on the new message.
	msg->set_var(*v_temp);
      }

      msg->set_var(*updated_perf);
      
      sprintf(ss_new, "%s_simplex_time", appName);
      s_new=Tcl_GetVar(tcl_inter,ss_new,0);
      if (s_new==NULL)
	{
	  printf("Error getting value of variable! ss_new:: %s \n", ss_new);
	  free(ss_new);
	  operation_failed(client_socket);
	  return;
	}
      
      msg->set_timestamp(strtol(s_new,NULL,10));
      
      // updating the goodness
      sprintf(s_updt, "updateObsGoodness %s %d %d", appName, updt_perf, msg->get_timestamp());
      
      if(debug_mode)
	printf("goodness tcl string from variable_request: %s \n", s_updt);
      
      if ((err = Tcl_Eval(tcl_inter, s_updt)) != TCL_OK) {
	printf("FAILED HERE\n");
	operation_failed(client_socket);
	return;
      }
      
      send_message(msg, client_socket);
      printf("Server is sending the message of type : %d \n", msg->get_type());
      delete msg;
      delete mesg;
    }

}


/*****
 *
 * Return the value of the variable from the tcl backend. Use this
 * carefully. You have to make sure the variable is defined in the
 * backend.
 *
 ***/
void var_tcl_variable_request(HUpdateMessage *mesg, int client_socket){

    char *s;
    char ss[256], ss2[256];

    VarDef v;


    /*
    if(debug_mode)
        printf("received a tcl variable request \n");
    */
    for (int i=0; i<mesg->get_nr_vars();i++) {
        sprintf(ss2,"%s",mesg->get_var(i)->getName());
        char  *appName = clientName[client_socket];
        sprintf(ss, "%s_%s", appName, ss2);
	/*
        if(debug_mode)
            printf("tcl string:: %s \n", ss);
	*/
        s=Tcl_GetVar(tcl_inter,ss,0);

	/*
	  if(debug_mode)
	  printf(" Tcl backend variable! %s\n",s);
	*/
        if (s==NULL) {
            printf("Error getting value of Tcl backend variable! \n");
            operation_failed(client_socket);
            return;
        }

        VarDef *v;

        switch (mesg->get_var(i)->getType()) {
            case VAR_INT:
                mesg->get_var(i)->setValue(strtol(s,NULL,10));
                break;
            case VAR_STR:
                mesg->get_var(i)->setValue(s);
                break;
        }

    }

    send_message(mesg, client_socket);
    delete mesg;
}


// check if a file exists in the file system
int file_exists (const char * fileName) {
  struct stat buffer;
  int i = stat ( fileName, &buffer );
  if ( i == 0 ) {
      return 1;
  }
  return 0;
}

/*
  Send a query to the code-server to see if code generation for current
  search iteration is done.

  1. The first method utilizes the code-server. The connection to the
     code-server is maintained in the tcl backend.
  2. The second method uses the file-system and checks for
     code.complete.<timestep> file to determine if the new code is ready.
     This method is useful when the machine that is running Active Harmony
     does not allow tcp connections to remote machines.
*/

/*
void check_code_completion_via_server(HUpdateMessage *mesg, int client_socket){
    int err;
    char ss[256], ss2[256];
    char s_code[80];
    int return_val = 0;
    char tcl_result[80];
    char *appName = clientName[client_socket];
    sprintf(s_code, "code_completion_query %s", appName);
    
    printf("%s \n", s_code);

    if ((err = Tcl_Eval(tcl_inter, s_code)) != TCL_OK) {
        fprintf(stderr, "Error getting code completion status: %d", err);
        fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
        printf("FAILED HERE\n");
        operation_failed(client_socket);
        return;
    } else
    {
        sprintf(tcl_result, "%s", tcl_inter->result);
    }

    return_val=atoi(tcl_result);

    // retrieve the timestep
    int timestep = *(int *)mesg->get_var(0)->getPointer();
    printf("CODE GENERATION IS %d for timestep: %d \n", return_val, code_timestep);

    //printf("timestep: %d \n", timestep);

    mesg->get_var(0)->setValue(return_val);

    send_message(mesg, client_socket);
    delete mesg;
}
*/
/*
  Check the code_complete.<timestep> file to see if the code generation is
  complete. This is used with the standalone version of the code generator.
*/
void check_code_completion(HUpdateMessage *mesg, int client_socket)
{
    char *s;
    char ss[256], ss2[256];
    char *appName = clientName[client_socket];
    //char *appName_global = (char *)malloc(strlen(appName));


    char *startp, *endp;
    startp=appName;
    endp=appName;
    while (*endp!='_') endp++;
    char temp_name[256];
    strncpy(temp_name, startp, (endp-startp));
    sprintf(&temp_name[endp-startp], "");

    // retrieve the timestep
    int req_timestep = *(int *)mesg->get_var(0)->getPointer();
    int code_timestep=code_timesteps[temp_name];
    int last_code_req_timestep=last_code_req_timesteps[temp_name];
    int last_code_req_response=last_code_req_responses[temp_name];

    if(debug_mode)
        printf("Code Request Timestep: %d last_code_req_timestep %d code_timestep: %d \n",
           req_timestep,last_code_req_timestep,code_timestep);
    
    int return_val=0;
    if(req_timestep > last_code_req_timestep)
    {
        // based on file system right now.
        sprintf(ss, "%s/code_complete.%s.%d",code_flags_path, temp_name, code_timestep);
        if(debug_mode)
            printf("Looking for %s \n", ss);
        if(file_exists(ss))
        {
            if(debug_mode)
                printf("code generation is complete for timestep: %d \n", code_timestep);

            code_timesteps[temp_name]=code_timestep+1;
            return_val=1;
            last_code_req_response=return_val;
            last_code_req_responses[temp_name]=return_val;
            std::remove(ss);
            sprintf(ss, "%s/code_complete.%s.%d",code_flags_path, temp_name, code_timestep+1);
            printf("removing: %s \n", ss);
            std::remove(ss);
        } else
        {
            last_code_req_responses[temp_name]=0;
        }
        //last_code_req_timestep=req_timestep;
        last_code_req_timesteps[temp_name]=req_timestep;
    } else
    {
        return_val=last_code_req_responses[temp_name];
        //return_val=last_code_req_response;
    }
    mesg->get_var(0)->setValue(return_val);
    send_message(mesg, client_socket);
    //printf("Sending code generation message \n");
    //mesg->print();
    delete mesg;
}



void database(HUpdateMessage *mesg, int client_socket){
    /*
      Make sure all pro instances have reported their performance value to
      the server and that the "next_iteration" flag is set to 1 before
      calling this function.
    */

   int err;
   char *appName = clientName[client_socket];
   char curr_conf[80];
   char s[80];
   char* return_value;


    // tcl proc get_test_configuration appName returns the current
    // configuration being tested
     sprintf(s, "get_test_configuration %s", appName);
     if ((err = Tcl_Eval(tcl_inter, s)) != TCL_OK) {
       fprintf(stderr, "Error retreiving the current configuration: %d", err);
	 fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
	 printf("FAILED HERE\n");
	 operation_failed(client_socket);
	 return;
     } else
       {
       char *startp, *endp;
	 startp=appName;
	 endp=appName;
	 while (*endp!='_') endp++;
	 char temp_name[256];
	 strncpy(temp_name, startp, (endp-startp));
	 sprintf(&temp_name[endp-startp], "");
	 sprintf(curr_conf, "%s_%s", temp_name, tcl_inter->result);
	 }

     string str_conf = curr_conf;
     it=global_data.find(str_conf);
     if(it == global_data.end())
       {
       stringstream ss;
	 ss.str("");
	 ss << INTMAXVAL;
	 return_value=(char*)ss.str().c_str();
	 }
     else
       {
       stringstream ss;
	 ss.str("");
	 ss << (*it).second;
	 return_value = (char*)ss.str().c_str();
	 }

     switch (mesg->get_var(0)->getType()) {
       case VAR_INT:
	   mesg->get_var(0)->setValue(atoi(return_value));
	     break;
	     case VAR_STR:
	  mesg->get_var(0)->setValue(return_value);
	     break;
	     }

        send_message(mesg, client_socket);
        delete mesg;

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
        //fprintf(stderr, "TCLres(varSet-OK): %s ; %s ; %s\n", ss2, res, tcl_inter->result);

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
#endif  /*notdef*/


        char StartEvent[] = "START";
        char EndEvent[] = "END";
        char SyncEvent[] = "SYNC";
        char VarEvent[] = "VAR";
        char *event;

        if (!strcmp(mesg->get_var(i)->getName(), "AdaState")){
            //fprintf(stderr, "%s\n", s);
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
    if (Tcl_VarEval(tcl_inter, "SchedulerEventHandler ", appName, " END", NULL)
        != TCL_OK) {
        fprintf(stderr, "revoke_resources: ERROR in unsetting related tcl vars: %s\n"
                , tcl_inter->result);
    }
    free(clientName[clientInfo[appName]]);
}


void filter(map<string,string> &m, vector<string> & result, vector<string> &values,
            string &condition)
{
    map<string,string>::iterator iter;
    for(iter=m.begin();iter!=m.end();iter++)
    {
           string key=iter->first; 
           size_t found=key.find(condition);
           if (found!=string::npos) 
           {
               result.push_back(key);
               values.push_back(iter->second);
               m.erase(iter);
           }
    }

}
/***
 *
 * The client unregisters! Send back a confirmation message
 *
 ***/
void client_unregister(HRegistMessage *m, int client_socket) {

    /* clear all the information related to the client */
    char *appName = clientName[client_socket];
    if(debug_mode)
    {
        printf("ENTERED CLIENT UNREGISTER!!!!!\n");
    }
    
    // derive the global appName
    char *startp, *endp;
    startp=appName;
    endp=appName;
    while (*endp!='_') endp++;

    char temp_name[128];
    char temp[128];

    strncpy(temp_name, startp, (endp-startp));
    sprintf(&temp_name[endp-startp], "");

    // unique_points filename
    sprintf(temp,"unique_points.%s.dat", temp_name);

    // configurations
    vector<string> configurations;

    // corresponding performance values
    vector<string> corresponding_values;
    // filter the keys by global appName
    string filter_text = temp_name;

    // get the unique configurations for this application
    filter(global_data,configurations, corresponding_values,filter_text);
    int index=0;
    FILE *pFile;
    pFile = fopen (temp,"a");
    char cmd[128];
    sprintf(cmd,"----------------- \n");
    if (pFile!=NULL)
    {
        fputs (cmd,pFile);
    }
    for (index=0; index < configurations.size(); index++) 
    {
        fprintf(pFile, (configurations[index]).c_str());
        fprintf(pFile, " ==> ");
        fprintf(pFile, (corresponding_values[index]).c_str());
        fprintf(pFile, " \n");
    }
    fclose(pFile);

    /* run the Tcl function to clear after the client */
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
        //printf("Send confimation !\n");
        HRegistMessage *mesg = new HRegistMessage(HMESG_CONFIRM,0);
        
        /* send back the received message */
        send_message(mesg,client_socket);
        delete mesg;
    }
}
void performance_update_int(HUpdateMessage *m, int client_socket) {

    int err;
    FILE *f1;
    char *appName = clientName[client_socket];
    char curr_conf[80];
    char s[80];
    int performance = INTMAXVAL;

    char perf_string[80];

    // get the current conf for this instance from the tcl backend.
    performance=*(int *)m->get_var(0)->getPointer();

    if(performance != INTMAXVAL)
    {

        sprintf(s, "get_test_configuration %s", appName);

        if ((err = Tcl_Eval(tcl_inter, s)) != TCL_OK) {
            fprintf(stderr, "Error retreiving the current configuration: %d", err);
            fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
            printf("FAILED HERE\n");
            operation_failed(client_socket);
            return;
        } else
        {
            char *startp, *endp;
            startp=appName;
            endp=appName;
            while (*endp!='_') endp++;
            char temp_name[256];
            strncpy(temp_name, startp, (endp-startp));
            sprintf(&temp_name[endp-startp], "");
            sprintf(curr_conf, "%s_%s", temp_name, tcl_inter->result);
        }

        sprintf(perf_string, "%d", performance);
        string str_conf = curr_conf;
        global_data.insert(pair<string, string>(str_conf, (string)perf_string));
    }

    sprintf(s, "updateObsGoodness %s %d %d", appName, performance, m->get_timestamp());

    if(debug_mode)
        printf("goodness tcl string: %s \n", s);

    if ((err = Tcl_Eval(tcl_inter, s)) != TCL_OK) {
        fprintf(stderr, "Error setting performance function: %d", err);
        fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
        printf("FAILED HERE\n");
        operation_failed(client_socket);
        return;
    }
    send_message(m,client_socket);
}

// performance update function
void performance_update_double(HUpdateMessage *m, int client_socket) {

    int err;
    FILE *f1;
    char *appName = clientName[client_socket];
    char curr_conf[80];
    char s[80];
    double performance = FLTMAXVAL;

    char* perf_dbl=(char*)(m->get_var(0)->getPointer());

    performance=atof(perf_dbl);

    // get the current conf for this instance from the tcl backend.

    if(performance != INTMAXVAL)
    {

        sprintf(s, "get_test_configuration %s", appName);

        if ((err = Tcl_Eval(tcl_inter, s)) != TCL_OK) {
            fprintf(stderr, "Error retreiving the current configuration: %d", err);
            fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
            printf("FAILED HERE\n");
            operation_failed(client_socket);
            return;
        } else
        {
            char *startp, *endp;
            startp=appName;
            endp=appName;
            while (*endp!='_') endp++;
            char temp_name[256];
            strncpy(temp_name, startp, (endp-startp));
            sprintf(&temp_name[endp-startp], "");
            sprintf(curr_conf, "%s_%s", temp_name, tcl_inter->result);
        }

        string str_conf = curr_conf;
        string perf_num = perf_dbl;
        global_data.insert(pair<string, string>(str_conf, perf_num));
    }


    sprintf(s, "updateObsGoodness %s %.15g %d", appName,performance, m->get_timestamp());
    if(debug_mode)
        printf("goodness tcl string: %s \n", s);
    if ((err = Tcl_Eval(tcl_inter, s)) != TCL_OK) {
        fprintf(stderr, "Error setting performance function: %d", err);
        fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
        printf("FAILED HERE\n");
        operation_failed(client_socket);
        return;
    }

    send_message(m,client_socket);
}


// performance update function
void performance_update(HUpdateMessage *m, int client_socket) {
    switch (m->get_var(0)->getType()) {
        case VAR_INT:
            performance_update_int(m,client_socket);
            break;
        case VAR_STR:
            performance_update_double(m,client_socket);
            break;
    }
}




/* OLD Design
 * Server after receiving the performance values from the client, sends the corresponding configuration
 * back to the client. Client then queries the server whether it has seen the configuration before.
 */


//NEW DESIGN
//Server checks the database whether the performance is already evaluated or not 

void performance_already_evaluated_int(HUpdateMessage *m, int client_socket) 
{
   
  int err;
  FILE *f1;
  char *appName = clientName[client_socket];
  
  char curr_conf[80];
  char str_conf[80];
  
  int i;

  char s[80];
  int performance = INTMAXVAL;

  char perf_string[80];

  char new_res[100];

  // get the current conf for this instance from the tcl backend.
  performance=*(int *)m->get_var(0)->getPointer();

  if(performance != INTMAXVAL)
  {

      sprintf(s, "get_test_configuration %s", appName);

      if ((err = Tcl_Eval(tcl_inter, s)) != TCL_OK) 
      {
	fprintf(stderr, "Error retreiving the current configuration: %d", err);
	fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
	printf("FAILED HERE\n");
	operation_failed(client_socket);
	return;
      } 
      else
      {
	  char *startp, *endp;
	  startp=appName;
	  endp=appName;
	  while (*endp!='_') endp++;
	  char temp_name[256];
	  strncpy(temp_name, startp, (endp-startp));
	  sprintf(&temp_name[endp-startp], "");
	  sprintf(curr_conf, "%s_%s", temp_name, tcl_inter->result);
      }

      //sprintf(perf_string, "%d", performance);
      //for(i=0;i<=80;i++)
      //{
      //  str_conf[i] = curr_conf[i];
      //}
      //global_data.insert(pair<string, string>(str_conf, (string)perf_string));

      string str_conf = curr_conf;
      sprintf(perf_string, "%d", performance);
      global_data.insert(pair<string, string>(str_conf, (string)perf_string));

  }
  

  //Server checks the database whether the conf has already been evaluated in the past
  it=global_data.find(str_conf);

  //searching the database
  for (it = global_data.begin(); it != global_data.end(); ++it)
  {
    if(!str_conf)
    {

      //printf("The following configuration has not seen before \n");
      //printf("Updating the performance and sending the configuration to the client\n");
      
      performance_update_int(m, client_socket);

      if(debug_mode)
        printf("goodness tcl string: %s \n", s);

      if ((err = Tcl_Eval(tcl_inter, s)) != TCL_OK)
        {
          fprintf(stderr, "Error setting performance function: %d", err);
          fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
          printf("FAILED HERE\n");
          operation_failed(client_socket);
          return;
        }
      send_message(m,client_socket);
    }
    else
    {
	printf("The following configuration has already seen \n");

	//For now using the update function again
	performance_update_int(m, client_socket);

	if(debug_mode)
	  printf("goodness tcl string: %s \n", s);

	if ((err = Tcl_Eval(tcl_inter, s)) != TCL_OK)
	{
          fprintf(stderr, "Error setting performance function: %d", err);
          fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
          printf("FAILED HERE\n");
          operation_failed(client_socket);
          return;
        }
	send_message(m,client_socket);

    }

    send_message(m,client_socket);
  
   }

}


void performance_already_evaluated_double(HUpdateMessage *m, int client_socket)
{

  int err;
  FILE *f1;
  char *appName = clientName[client_socket];

  char curr_conf[80];
  char str_conf[80];

  int i;

  char s[80];
  double performance = FLTMAXVAL;
  
  //get the current conf for this instance from the tcl backend
  char* perf_dbl=(char*)(m->get_var(0)->getPointer());
  
  performance = atof(perf_dbl);

  char perf_string[80];

  char new_res[100];

  if(performance != INTMAXVAL)
  {

      sprintf(s, "get_test_configuration %s", appName);

      if ((err = Tcl_Eval(tcl_inter, s)) != TCL_OK)
      {
	fprintf(stderr, "Error retreiving the current configuration: %d", err);
	fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
	printf("FAILED HERE\n");
	operation_failed(client_socket);
	return;
      }
      else
      {
        char *startp, *endp;
        startp=appName;
        endp=appName;
        while (*endp!='_') endp++;
        char temp_name[256];
        strncpy(temp_name, startp, (endp-startp));
        sprintf(&temp_name[endp-startp], "");
        sprintf(curr_conf, "%s_%s", temp_name, tcl_inter->result);
      }

      //sprintf(perf_string, "%d", performance);
      //for(i=0;i<=80;i++)
      //  {
      //    str_conf[i] = curr_conf[i];
      //  }
      //global_data.insert(pair<string, string>(str_conf, (string)perf_string));

      string str_conf = curr_conf;
      string perf_num = perf_dbl;
      global_data.insert(pair<string, string>(str_conf, perf_num));

  }

  //Server checks the database whether the conf has already been evaluated in the past
  it=global_data.find(str_conf);

  //searching the database
  for (it = global_data.begin(); it != global_data.end(); ++it)
  {
      if(!str_conf)
      {

      	printf("The following configuration has not seen before \n");
       	printf("Updating the performance and sending the configuration to the client\n");

       	performance_update_int(m, client_socket);

       	if(debug_mode)
       	  printf("goodness tcl string: %s \n", s);

       	if ((err = Tcl_Eval(tcl_inter, s)) != TCL_OK)
	{
          fprintf(stderr, "Error setting performance function: %d", err);
          fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
          printf("FAILED HERE\n");
          operation_failed(client_socket);
          return;
        }
	send_message(m,client_socket);
      }
      else
      {
	printf("The following configuration has already seen \n");

	//For now using the update function again
	performance_update_int(m, client_socket);

	if(debug_mode)
	  printf("goodness tcl string: %s \n", s);

	if ((err = Tcl_Eval(tcl_inter, s)) != TCL_OK)
	{
	  fprintf(stderr, "Error setting performance function: %d", err);
	  fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
	  printf("FAILED HERE\n");
	  operation_failed(client_socket);
	  return;
	}
      	send_message(m,client_socket);
      }
 
      send_message(m,client_socket);
  }
}

// performance already evaluated function
void performance_already_evaluated(HUpdateMessage *m, int client_socket) {
  switch (m->get_var(0)->getType()) {
          case VAR_INT:
	    performance_already_evaluated_int(m,client_socket);
	    break;
          case VAR_STR:
	    performance_already_evaluated_double(m,client_socket);
	    break;
    }
}

// not used anymore... but has some use for debugging. so keeping it intact.
void performance_update_with_conf(HUpdateMessage *m, int client_socket) {

    int err;

    char *appName = clientName[client_socket];
    char s[512];
    char *conf = (char *)m->get_var(0)->getName();

    printf(" appName: %s \n\n", appName);

    sprintf(conf,"%s",m->get_var(0)->getName());

    string str_conf = conf;
    printf("configuration: %s \n", conf);

    //global_data.insert(pair<string,string>(str_conf, *(int *)m->get_var(0)->getPointer()));

    sprintf(s, "updateObsGoodness %s %d %d", appName,*(int *)m->get_var(0)->getPointer(), m->get_timestamp());

    printf("perf_update_str=\n(%s)\n",s);

    if ((err = Tcl_Eval(tcl_inter, s)) != TCL_OK) {
        fprintf(stderr, "Error setting performance function: %d", err);
        fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
        printf("FAILED HERE\n");
        operation_failed(client_socket);
        return;
    }
    send_message(m,client_socket);
}



int in_process_message = FALSE;
void process_message_from(int temp_socket){

    HMessage *m;
    assert(!in_process_message);
    in_process_message = TRUE;

    /* get the message type */
    m=receive_message(temp_socket);
    if (m == NULL){
        client_unregister((HRegistMessage*)m, temp_socket);
        in_process_message = FALSE;
        return;
    }

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
        case HMESG_PERF_ALREADY_EVALUATED:
	    performance_already_evaluated((HUpdateMessage *)m, temp_socket);
	    break;
        case HMESG_TCLVAR_REQ:
            var_tcl_variable_request((HUpdateMessage *)m, temp_socket);
            break;
        case HMESG_DATABASE:
            database((HUpdateMessage *)m, temp_socket);
            break;
        case HMESG_WITH_CONF:
            performance_update_with_conf((HUpdateMessage *)m, temp_socket);
            break;
        case HMESG_CLIENT_UNREG:
            client_unregister((HRegistMessage *)m, temp_socket);
            break;
        case HMESG_CODE_COMPLETION:
            check_code_completion((HUpdateMessage *)m, temp_socket);
            break;
        default:
            printf("Wrong message type!\n");
    }

//#ifdef USE_TK
    /* process Tk events */
    while (Tcl_DoOneEvent(TCL_DONT_WAIT)>0)
        ;
//#endif
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


    /* do listening to the socket */
    if (listen(listen_socket, SOMAXCONN)) {
        h_exit("Listen to socket failed!");
    }

    while (1) {
        listen_set_copy=listen_set;
        //printf("Select\n");
        active_sockets=select(highest_socket+1,&listen_set_copy, NULL, NULL, NULL);
        /* we have a communication request */
        for (socketIterator=socket_set.begin();socketIterator!=socket_set.end();socketIterator++) {
            if (FD_ISSET(*socketIterator, &listen_set_copy) && (*socketIterator!=listen_socket) && (*socketIterator!=xfd)) {
                /* we have data from this connection */
                process_message_from(*socketIterator);
            }
        }
        if (FD_ISSET(listen_socket, &listen_set_copy)) {
            /* we have a connection request */
            /* via George Teodoro*/
            temp_addrlen = sizeof(temp_addr);
            if ((temp_socket = accept(listen_socket,(struct sockaddr *)&temp_addr, (unsigned int *)&temp_addrlen))<0) {
                // have to investigate more
                h_exit("Accepting connections failed!");
            }
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

            fflush(stdout);
        }
#endif
    }
}


char* get_timestamp ()
{
  time_t now = time (NULL);
  return asctime(localtime (&now));
}

/***
 *
 * This is the main program.
 * It first checks the arguments.
 * It then initializes and goes for the main loop.
 *
 ***/
int main(int argc, char **argv) {
    time_t start_time;
    time (&start_time);
    hserver_start_timestamp=start_time;
    check_parameters(argc, argv);
    server_startup();
    main_loop();
}
