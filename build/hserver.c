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
#include <dirent.h>
#include <errno.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

//PID
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <map>

#include "hserver.h"
#include "httpsvr.h"
//Global Config 
#include "hglobal_config.h"

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
list<int> unknown_fds, harmony_fds, http_fds;
int highest_socket;
list<int>::iterator socketIterator;

list<int> id_set;

/* the X file descriptor */
int xfd;

/*
 * The Tcl Interpreter
 */
Tcl_Interp *tcl_inter;
char harmonyTclFile[256];

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

map<string,string> appName_confs;
map<string,string>::iterator it2;

vector<string> coord_history;

//Harmony server's Process ID info
int hs_process_id;
char new_code_path[256];

//Code-generation completion flag
char code_flags_path[256];

/***
 *
 * Check the arguments given to the server
 *
 ***/

int draw_windows=0;

string history_since(unsigned int last)
{
    stringstream ss;
    unsigned int max = coord_history.size();

    while (last < max) {
        ss << "coord:" << coord_history[last++] << "|";
    }
    return ss.str();
}

void check_parameters(int argc, char **argv) {

    // Gets the Search Algorithm and the config file 
    // that the user has selected in the hglobal_config file.
    char *searching_algo;
    
    searching_algo = get_search_algorithm();

    int search_algo;

    for(int i=0; searching_algo[i] != '\0'; i++)
    {
      searching_algo[i] = tolower(searching_algo[i]);
    }

    if(strcmp(searching_algo, "pro")==0)
    {
      search_algo = 1;
    }
    else if(strcmp(searching_algo, "nelder mead")==0)
    {
      search_algo = 2;
    }
    else if(strcmp(searching_algo, "random")==0)
    {
      search_algo = 3;
    }
    else if(strcmp(searching_algo, "brute force")==0)
    {
      search_algo = 4;
    }
    else
    {
      printf("Please check the config file \n");
      printf("No search algorithm has been entered \n");
    }

    // by default, no windows are drawn
    //  to disable Active Harmony server window, use "gmake server_no_tk" to build the framework.
    draw_windows=0;

    if( argc >= 2 )
    {
      // second argument is the graphics parameter
      draw_windows=atoi(argv[2]);
    }
 
    if(search_algo==1)
    {
      printf("[AH]: using the PRO algorithm. Please make sure the parameters in \n");
      printf("[AH]: ../tcl/pro_init_<appname>.tcl are defined properly.\n");
      char* config_file=get_config_file();
      sprintf(harmonyTclFile, "%s", config_file);
    }
    if(search_algo==2)
    {
      printf("[AH]: using the Nelder Mead Simplex Algorithm. \n");
      char* config_file=get_config_file();
      sprintf(harmonyTclFile, "%s", config_file);
    }
    if(search_algo == 3)
    {
      printf("[AH]: using random Algorithm. \n");
      char* config_file=get_config_file();
      sprintf(harmonyTclFile, "%s", config_file);
    }
    if(search_algo == 4)
    {
      printf("[AH]: using brute force Algorithm. \n");
      char* config_file=get_config_file();
      sprintf(harmonyTclFile, "%s", config_file);
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
        printf("[AH]: ************ Updating client!!! ****************\n");
        printf("[AH]: Variable : %s_bundle_%s has value %s\n",argv[1], argv[2], argv[3]);
        printf("[AH]: ClientInfo[%s]=%d\n",argv[1], clientInfo[argv[1]]);
        printf("[AH]: clientSignals[]=%d\n",clientSignals[clientInfo[argv[1]]]);
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

    char* PID_temp;

    /* temporary variable */
    char s[10];

    for(int i = 1; i <= 2048; i++)
        id_set.push_back(i);

    /* used address */
    struct sockaddr_in sin;

    int optVal;

    /* Harmony server process ID */
    hs_process_id = getppid();

    printf("\nHarmony Server's Process-Id (PID) for this run: %d \n", hs_process_id);

   //Extracts the path where the PID file is located
    PID_temp = get_hspid_path();

    FILE *hs_pid;

    hs_pid = fopen(PID_temp, "w+");

    fprintf(hs_pid, "%d", hs_process_id);

    fclose(hs_pid);

    int err;
    char * serv_port;
    // try to get the port info from the environment
    serv_port=getenv("HARMONY_S_PORT");
    if (serv_port != NULL)
    {
      listen_port=atoi(serv_port);
    } 
    else
    {
      // if the env var is not defined, use the default
      listen_port = atoi(server_portnum());
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
        h_exit("Error binding socket! Please check if there are previous \n instances of hserver still running.");
    }

    /* Now we have to initialize the Tcl interpreter */
    tcl_inter = Tcl_CreateInterp();

    if ((err=Tcl_Init(tcl_inter)) != TCL_OK) {
        h_exit("Tcl Init failed!");
    }

    if ((err=Tcl_EvalFile(tcl_inter, harmonyTclFile)) != TCL_OK){
        printf("[AH]: Tcl Error %d ; %s %s \n",err, tcl_inter->result, harmonyTclFile);
        h_exit("TCL Interpreter Error ");
    }

    /*
      register update_client function with the Tcl interpretor
    */
    // TODO: do we still rely on update_client procedure?
    Tcl_CreateCommand(tcl_inter, "update_client", update_client, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
    /*
      Set the file descriptor set
    */

    FD_ZERO(&listen_set);
    FD_SET(listen_socket, &listen_set);
    highest_socket=listen_socket;

    http_init();
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
 * Client registration adds the client information into the relevant maps
 *
 ***/
void client_registration(HRegistMessage *m, int client_socket){

    if (debug_mode) {
        printf("[AH]: Client registration! (%d)\n", client_socket);
    }

    /* store information about the client into the local database */
    clientSignals[client_socket]=m->get_param();
    int id = m->get_id();

    if (id) {
        /* the client is relocating its communicating agent */
        /* the server dissacociates the old socket with the client, and
         * uses the new one instead */
        printf("[AH]: Client id %d relocated\n", id);

        int cs = clientSocket[id];
        clientSocket[id] = client_socket;

        char *appName = clientName[cs];
        clientInfo[appName]=client_socket;
        clientName.erase(cs);
        clientName[client_socket] = appName;

        /* This code sequence is problematic.  It might disrupt the loop
         * inside main_loop() that iterates over harmony_fds.
         * Find a way to move it into main_loop().
         */
        FD_CLR(cs, &listen_set);
        if (cs==*socketIterator)
            socketIterator++;
        harmony_fds.remove(cs);
        if (highest_socket==cs)
            highest_socket--;
    } else {
        /* generate id for the new client */
        id = id_set.front();
        id_set.pop_front();
        clientSocket[id] = client_socket;
    }

    clientId[client_socket] = id;
    if (debug_mode) {
        printf("[AH]: Send registration (id %d) confimation !\n", id);
    }

    /* send the confirmation message with the client_id */
    /* ACK */
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
        printf("[AH]: Message size: %d\n",mesg->get_message_size());
    }
    
    if ((err=Tcl_Eval(tcl_inter,new_string))!=TCL_OK) {
        printf("[AH]: Error %d interpreting the tcl description!\n",err);
        printf("[AH]: %s\n",tcl_inter->result);
        printf("[AH]: [%s]\n",new_string);
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
        // Skip past intro comments and/or whitespace
        while (strspn(startp, " \t\n\r\f#")) {
            startp = startp + strspn(startp, " \t\n\r\f");
            if (*startp=='#')
                while (*(startp++)!='\n');
        }
        // go to first space
        while (*startp!=' ') startp++;
        // get rid of separating spaces
        while (*startp==' ') startp++;
        // startp points now to the first char in the name of the application
        endp=startp;
        while (*endp!=' ') endp++;
        // endp points to the last char in the ame of the application

        char appName[256];
        char temp_name[256];

        strncpy(appName, startp, (endp-startp));
        strncpy(temp_name, startp, (endp-startp));
        sprintf(&temp_name[endp-startp], "");

        code_timesteps[temp_name]=1;
        last_code_req_timesteps[temp_name]=0;
        last_code_req_responses[temp_name]=0;

        sprintf(&appName[endp-startp], "_%d", clientId[client_socket]);
        if (debug_mode) {
            printf("[AH]: Application name: %s\n", appName);
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

    // get the appName
    char  *appName = clientName[client_socket];

    // this is one of the most weirdest ways to name the variables!
    char ss[256], ss2[256];
    sprintf(ss2,"%s(value)",mesg->get_descr());
    sprintf(ss, "%s_bundle_%s", appName, ss2);

    if(debug_mode)
        printf("[AH]: tcl string : %s \n", ss);

    if ((s=Tcl_GetVar(tcl_inter,ss,0))==NULL) {

        free(sss);
        operation_failed(client_socket);
        return;
    }
    if(debug_mode)
        printf("[AH]: tcl value : %s \n", s);

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

    char* s;
    char s_new[80];
    char ss[256], ss2[256];
    char s_updt[128];
    char *appName = clientName[client_socket];

    char new_config[80];
    char next_configuration[80];
    int err;

    char current_config[80];

    char next_config[80];

    char* return_value;

    char new_res[100];
    
    // construct the key using get_test_configuration (look at the database function)
    // if the conf is in the database, either change the message type or construct a brand new message    
    // object. (set its type to already_evaluated)... otherwise, do the usual
    
    sprintf(ss, "get_test_configuration %s", appName);

    if ((err = Tcl_Eval(tcl_inter, ss)) != TCL_OK) 
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
    }
    
    /*************************************DATABASE CHECK********************************************/

    //check the database whether "current_config" is already available
    string str_config = current_config;
    
    it=global_data.find(str_config);
    
    // this is the case where the performance is not in the database.
    // if the iterator is at the end of the map, we never found the conf in the database.
    //  so simply use the old code (from the point after setting the values of the variables).

    if(it == global_data.end())
    {
      //The database search has concluded till the end and did not find the configuration

      printf("The following configuration has not seen before \n");


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
	  
	  //Appending the Harmony server's process id to the message that is going to be sent to the client
	  sprintf(ss, "%s_bundle_%s", appName, ss2);
       
	  if(debug_mode)
            printf("[AH]: tcl value string: %s \n", ss);
	   
	  // you are invoking the tcl interpreter to parse this tcl command.
	  s=Tcl_GetVar(tcl_inter,ss,0);
        
	  if(debug_mode)
            printf("[AH]: tcl value : %s \n", s);
        
	  if (s==NULL) 
	    {
	      printf("[AH]: Error getting value of variable!\n");
	      free(ss);
	      operation_failed(client_socket);
	      return;
	    }

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
   
      sprintf(ss, "%s_simplex_time", appName);

      if(debug_mode)
        printf("[AH]: Timestamp tcl string: %s \n", ss);
      
      s=Tcl_GetVar(tcl_inter,ss,0);

      if(debug_mode)
        printf("[AH]: Timestamp app: %s \n", s);

      if (s==NULL) {
        printf("[AH]: Error getting value of variable! ss:: %s \n", ss);
        free(ss);
        operation_failed(client_socket);
        return;
      }

      if(debug_mode)
        printf("[AH]: Value of the timestamp from the server: %s \n", s);

      mesg->set_timestamp(strtol(s,NULL,10));

      send_message(mesg, client_socket);
      printf("Server is sending the message of type : %d \n", mesg->get_type());
     
      delete mesg;
    
    }
    else
    {
      // this is the case where the configuration->performance mapping exists in the database.

      char* s_next;
      char ss_next[256], ss1_next[256], ss2_next[256]; 
      
      char s_next_updt[128];
      
      printf("The following configuration has seen before \n");

      printf("Server gets the unique configuration from the backend\n");

      for (int i=0; i<mesg->get_nr_vars();i++) 
      {

	sprintf(ss_next, "get_next_configuration %s", appName);

	 if((err = Tcl_Eval(tcl_inter, ss_next)) != TCL_OK)
	   {
	     fprintf(stderr, "Error retreiving the next configuration: %d", err);
	     fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
	     printf("FAILED HERE\n");
	     operation_failed(client_socket);
	     return;
	   }
	 else
	   {
	     printf("The backend says the value is: %s \n", tcl_inter->result);
	     char *next_startp, *next_endp;
	     next_startp=appName;
	     next_endp=appName;
	     while(*next_endp!='_')next_endp++;
	     char next_temp_name[256];
	     strncpy(next_temp_name, next_startp, (next_endp-next_startp));
	     sprintf(&next_temp_name[next_endp-next_startp], "");
	     sprintf(next_config, "%s_%s", next_temp_name, tcl_inter->result);
	     printf("Checking the map: %s \n", next_config);
	   }

	 //Maintaining the list of the new config associated to the appName
	 appName_confs.insert(pair<string, string>(appName, next_config));
      

 	 // OfflineT_1_bundle_TI(value)
	 sprintf(ss1_next, "%s_bundle_%s(value)", appName, mesg->get_var(i)->getName());
	 
	 if(debug_mode)
	   printf("Tcl value string: %s \n", ss1_next);

	 //invoking the TCL interpreter to parse this tcl command
	 s_next=Tcl_GetVar(tcl_inter,ss1_next,0);

	 if(debug_mode)
	   printf("Tcl value: %s \n", s_next);

	 if(s_next==NULL)
	 {
	   printf("Error getting the value of variable!\n");
	   free(ss1_next);
	   operation_failed(client_socket);
	   return;
	 }

	 //get the VarDef object at position i in the message and check its type
	 switch(mesg->get_var(i)->getType())
	 {
	   //for c++ example this will be always int
	   case VAR_INT:
	     //is int
	     //update the value
	     mesg->get_var(i)->setValue(strtol(s_next,NULL,10));
	     break;
	   case VAR_STR:
	     mesg->get_var(i)->setValue(s_next);
	     break;
	 }
	 


      }

	
    it2 = appName_confs.find(next_config);
		 
    if(it2 != it)
    {
      int updt_perf;

      //The search has concluded till the end and did not find the configuration
       sprintf(ss1_next, "%s_simplex_time", appName);

       if(debug_mode)
	    printf("Timestamp tcl string: %s \n", ss1_next);

       s_next=Tcl_GetVar(tcl_inter, ss1_next, 0);

       if(debug_mode)
	   printf("Timestamp app: %s \n", s_next);

       if (s_next==NULL) 
       {
	 printf("Error getting value of variable! ss1_next:: %s \n", ss1_next);
	 free(ss1_next);
	 operation_failed(client_socket);
	 return;
       }
       
       if(debug_mode)
        printf("Value of the timestamp from the server: %s \n", s_next);
      
       mesg->set_timestamp(strtol(s_next,NULL,10));

       //Appending the server's PID to the message and sending it to the client

       send_message(mesg, client_socket);

       delete mesg;
    }
    else
    {
          
       //Extracting the performance form the database
       //this performance has to come from the map
       string updated_performance;
       int updt_perf;
       
       //getting the perf from the map
       updated_performance = (*it).second;

       //This converts perf(string) to perf(int)
       updt_perf = atoi(updated_performance.c_str());
       printf("Performance from the database: %d \n", updt_perf);

       sprintf(ss1_next, "%s_simplex_time", appName);

       if(debug_mode)
         printf("Timestamp tcl string: %s \n", ss1_next);

       s_next=Tcl_GetVar(tcl_inter, ss1_next, 0);

       if(debug_mode)
         printf("Timestamp app: %s \n", s_next);

       if(s_next==NULL) {
         printf("Error getting value of variable! ss_next:: %s \n", ss_next);
         free(ss1_next);
         operation_failed(client_socket);
         return;
       }

       if(debug_mode)
         printf("Value of the timestamp from the server: %s \n", s_next);

       mesg->set_timestamp(strtol(s_next,NULL,10));

       send_message(mesg, client_socket);

       delete mesg;
    }
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

    if(debug_mode)
        printf("[AH]: received a tcl variable request \n");

     for (int i=0; i<mesg->get_nr_vars();i++) {
        sprintf(ss2,"%s",mesg->get_var(i)->getName());
        char  *appName = clientName[client_socket];
        sprintf(ss, "%s_%s", appName, ss2);
	if(debug_mode)
            printf("[AH]: tcl string:: %s \n", ss);
        s=Tcl_GetVar(tcl_inter,ss,0);

        if(debug_mode)
            printf("[AH]: Tcl backend variable! %s\n",s);
        if (s==NULL) {
            printf("[AH]: Error getting value of Tcl backend variable! \n");
            operation_failed(client_socket);
            return;
        }

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
  Check the code_complete.<timestep> file to see if the code generation is
  complete. This is used with the standalone version of the code generator.
*/
void check_code_completion(HUpdateMessage *mesg, int client_socket)
{
    char *s;
    char ss[256];
    char *appName = clientName[client_socket];
    
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
        printf("[AH]: Code Request Timestep: %d last_code_req_timestep %d code_timestep: %d \n",
           req_timestep,last_code_req_timestep,code_timestep);
    
    int return_val=0;
    if(req_timestep > last_code_req_timestep)
    {
        // based on file system right now.
        sprintf(ss, "%s/code_complete.%s.%d",code_flags_path, temp_name, code_timestep);
	//        if(debug_mode)
            printf("[AH]: Looking for %s \n", ss);
        if(file_exists(ss))
        {
            if(debug_mode)
                printf("[AH]: code generation is complete for timestep: %d \n", code_timestep);

            code_timesteps[temp_name]=code_timestep+1;
            return_val=1;
            last_code_req_response=return_val;
            last_code_req_responses[temp_name]=return_val;
            std::remove(ss);
            sprintf(ss, "%s/code_complete.%s.%d",code_flags_path, temp_name, code_timestep+1);
            printf("[AH]: removing: %s \n", ss);
            std::remove(ss);
        } else
        {
            last_code_req_responses[temp_name]=0;
        }
        last_code_req_timesteps[temp_name]=req_timestep;
    } else
    {
        return_val=last_code_req_responses[temp_name];
    }
    
    mesg->get_var(0)->setValue(return_val);
    
    send_message(mesg, client_socket);

    delete mesg;
}


/*
 * Check the global database to see if we have already evaluated a 
 * configuration that we handed to a given client.
*/

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
	 printf("[AH]: FAILED HERE\n");
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
        
        if ((res = Tcl_SetVar2(tcl_inter, ss2, "value", s, TCL_GLOBAL_ONLY)) == NULL) {
            fprintf(stderr, "Error setting variable: %s", res);
            fprintf(stderr, "TCLres (varSet-Err): %s\n", tcl_inter->result);
            free(s);
            free(ss);
            operation_failed(client_socket);
            return;
        }

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
            if (!strcmp(s, "APP_STARTED")) {
                /* register application in scheduler */
                event = StartEvent;
            }else{
                /* call scheduler to take over */
                event = SyncEvent;
            }
        }
        else{
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

    if (!update_sent) {
        HRegistMessage *m = new HRegistMessage(HMESG_CONFIRM,0);
        send_message(m, client_socket);
        delete m;
    }
    update_sent = FALSE;

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

    int stashed_id;

    char new_dir_name[512];
    char *default_file_ext = "default.so";
    char appname_default[512];
    struct dirent *drnt;
    char strcomparison[512];

    char* APPname_temp;

    DIR *dr;
    DIR *new_dr;

    /* clear all the information related to the client */
    char *appName = clientName[client_socket];
    if(debug_mode)
    {
        printf("[AH]: ENTERED CLIENT UNREGISTER!!!!!\n");
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

    stashed_id = clientId[client_socket];
    clientSocket.erase(clientId[client_socket]);
    clientId.erase(client_socket);
    
    if (m == NULL){
        /* revoke resources */
        revoke_resources(clientName[client_socket]);

    } else {
        HRegistMessage *mesg = new HRegistMessage(HMESG_CONFIRM,0);
        
        /* send back the received message */
        send_message(mesg,client_socket);
        delete mesg;
    }

    if (close(client_socket) < 0)
        printf("Error closing harmony client %d socket %d.",
               stashed_id, client_socket);

    //After the completion of code generation and auto tuning, the server puts all the 
    //newly generated files into a seperate folder. The new folder is named after the PID of
    //the harmony server. This is mainly used for the cleaning purpose.

    //Getting the name of the application that is tuned
    APPname_temp = get_application_name();

    sprintf(new_code_path, "%s", get_new_code_path());

    //Creating the new directory
    sprintf(new_dir_name, "%s/harmonyPID_%d", new_code_path, hs_process_id);
    sprintf(appname_default, "%s_%s", APPname_temp, default_file_ext);

    dr=opendir(new_code_path);  
    
    if((mkdir(new_dir_name,00777))==0)
    {
      printf("The new directory [harmonyPID_%d] has been created \n", hs_process_id);

      //Moving all the files created by the code-server to the new folder
    
      if(dr)
      {
	while(drnt=readdir(dr))
	{
	  sprintf(strcomparison, "%s", drnt->d_name);
         
	  if(strcmp(strcomparison, appname_default) != 0)
	  {  
	    if(strcmp(drnt->d_name, appname_default) !=0)
	    {
	      int x1, x2, x3;
	      char newcmd[512];

	      new_dr=opendir(new_dir_name);
	      printf("%s", drnt->d_name);
	      sprintf(newcmd, "cp /%s/* /%s", new_code_path, new_dir_name);
	      printf("Calling System(%s)\n", newcmd);
	      x1 = system(newcmd);
	      printf("system() returned %d\n",x1);
	      printf("calling system(%s)\n", newcmd);
	      x2 = system(newcmd);
	      printf("system() returned %d\n",x2);
	      sprintf(newcmd, "rm %s/%s", new_code_path, drnt->d_name);
	      printf("calling system(%s)\n", newcmd);
	      x3 = system(newcmd);
	      printf("system() returned %d\n", x3);
	    }
	  }
	  else
          {
            printf("Not allowed to copy %s to %s \n", drnt->d_name, new_dir_name);
          }
        }
        closedir(dr);
      }
      else
      {
        printf("Cannot open directory [%s] \n", new_code_path);
      }
    }
    else
    {
      printf("The directory [harmonyPID_%d] already exists \n", hs_process_id);
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
            printf("[AH]: FAILED HERE\n");
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

        char *val = strchr(curr_conf, '_');
        if (val) {
            sprintf(s, "%u000,%s,%s,", time(NULL), ++val, perf_string);
            for (val = s; *val != '\0'; ++val)
                if (*val == '_')
                    *val = ',';
            strcat(s, appName);

            coord_history.push_back(s);
        }
    }

    // let the search backend know what we have received the performance
    //  number from this client.
    sprintf(s, "updateObsGoodness %s %d %d", appName, performance, m->get_timestamp());

    if(debug_mode)
        printf("[AH]: goodness tcl string: %s \n", s);

    if ((err = Tcl_Eval(tcl_inter, s)) != TCL_OK) {
        fprintf(stderr, "Error setting performance function: %d", err);
        fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
        printf("[AH]: FAILED HERE: %d \n", __LINE__);
        operation_failed(client_socket);
        return;
    }
    send_message(m,client_socket);
}

// performance update function
void performance_update_double(HUpdateMessage *m, int client_socket) {

    int err;
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
            printf("[AH]: FAILED HERE %d \n", __LINE__);
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

        char *val = strchr(curr_conf, '_');
        if (val) {
            sprintf(s, "%u000,%s,%s,", time(NULL), ++val, perf_dbl);
            for (val = s; *val != '\0'; ++val)
                if (*val == '_')
                    *val = ',';
            strcat(s, appName);

            coord_history.push_back(s);
        }
    }


    sprintf(s, "updateObsGoodness %s %.15g %d", appName,performance, m->get_timestamp());
    if(debug_mode)
        printf("[AH]: goodness tcl string: %s \n", s);
    if ((err = Tcl_Eval(tcl_inter, s)) != TCL_OK) {
        fprintf(stderr, "Error setting performance function: %d", err);
        fprintf(stderr, "TCLres(perfUpdate-Err): %s\n", tcl_inter->result);
        printf("[AH]: FAILED HERE: %d \n", __LINE__);
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


/*****Server queries the database to know whether the performance is already evaluated or not *****/

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

      string str_conf = curr_conf;
      sprintf(perf_string, "%d", performance);
      global_data.insert(pair<string, string>(str_conf, (string)perf_string));

      char *val = strchr(curr_conf, '_');
      if (val) {
          sprintf(s, "%u000,%s,%s,", time(NULL), ++val, perf_string);
          for (val = s; *val != '\0'; ++val)
              if (*val == '_')
                  *val = ',';
          strcat(s, appName);

          coord_history.push_back(s);
      }
  }
  

  //Server checks the database whether the conf has already been evaluated in the past
  it=global_data.find(str_conf);

  //searching the database
  for (it = global_data.begin(); it != global_data.end(); ++it)
  {
    if(!str_conf)
    {

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

      string str_conf = curr_conf;
      string perf_num = perf_dbl;
      global_data.insert(pair<string, string>(str_conf, perf_num));

      char *val = strchr(curr_conf, '_');
      if (val) {
          sprintf(s, "%u000,%s,%s,", time(NULL), ++val, perf_dbl);
          for (val = s; *val != '\0'; ++val)
              if (*val == '_')
                  *val = ',';
          strcat(s, appName);

          coord_history.push_back(s);
      }
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

/****
 *
 * Return the .so files path variables from the global_config file
 *
 ****/

char* so_file_path_prefix_run_dir(HDescrMessage *mesg, int client_socket)
{
  char *ppdir;  
  ppdir = so_path_prefix_run();
  HDescrMessage *m1 = new HDescrMessage(HMESG_SO_PATH_PREFIX_RUN_DIR, ppdir, strlen(ppdir));
  send_message(m1,client_socket);
  return ppdir;
  delete m1;
}

char* so_file_path_prefix_def(HDescrMessage *mesg, int client_socket)
{
  char *ppdef;
  ppdef = so_path_prefix_def();
  HDescrMessage *m1 = new HDescrMessage(HMESG_SO_PATH_PREFIX_DEF, ppdef, strlen(ppdef));
  send_message(m1,client_socket);
  return ppdef;
  delete m1;
}

char* so_file_path_prefix_code(HDescrMessage *mesg, int client_socket)
{  
  char *ppcode;  
  ppcode = so_path_prefix_code();
  HDescrMessage *m1 = new HDescrMessage(HMESG_SO_PATH_PREFIX_CODE, ppcode, strlen(ppcode));
  send_message(m1,client_socket);
  return ppcode;
  delete m1;    
}

int in_process_message = FALSE;
void process_message_from(int temp_socket, int *close_fd){

    HMessage *m;
    assert(!in_process_message);
    in_process_message = TRUE;

    /* get the message type */
    m=receive_message(temp_socket);
    if (m == NULL){
        client_unregister((HRegistMessage*)m, temp_socket);
        if (close_fd != NULL)
            *close_fd = 1;

        in_process_message = FALSE;
        return;
    }

    /*
     *	Based on the message type sent by the clients, we do different things.
     */

    switch (m->get_type()) {
        case HMESG_DAEMON_REG:
            break;
        case HMESG_CLIENT_REG:
            client_registration((HRegistMessage *)m,temp_socket);
            break;
        case HMESG_NODE_DESCR:
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
            if (close_fd != NULL)
                *close_fd = 1;
            break;
        case HMESG_CODE_COMPLETION:
            check_code_completion((HUpdateMessage *)m, temp_socket);
            break;
	case HMESG_SO_PATH_PREFIX_RUN_DIR:
	    so_file_path_prefix_run_dir((HDescrMessage *)m, temp_socket);
	    break;
	case HMESG_SO_PATH_PREFIX_DEF:
	    so_file_path_prefix_def((HDescrMessage *)m, temp_socket);
	    break;
	case HMESG_SO_PATH_PREFIX_CODE:
	    so_file_path_prefix_code((HDescrMessage *)m, temp_socket);
	    break;
	default:
            printf("[AH]: Wrong message type!\n");
    }

    in_process_message = FALSE;
}

int handle_new_connection(int fd)
{
    unsigned int header;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    int new_fd = accept(fd, (struct sockaddr *)&addr, &addrlen);
    if (new_fd < 0) {
        printf("Error accepting connection");
        return -1;
    }

    /* The peer may not have sent data yet.  To prevent blocking, lets
     * stash the new socket in the unknown_list until we know it has data.
     */
    unknown_fds.push_back(new_fd);
    printf("[AH]: Accepted connection from %s as socket %d\n",
           inet_ntoa(addr.sin_addr), new_fd);
    return new_fd;
}

void handle_unknown_connection(int fd, int *close_fd)
{
    unsigned int header;
    int readlen;

    readlen = recv(fd, &header, sizeof(header), MSG_PEEK);
    if (readlen < sizeof(header)) {
        /* Error on recv, or insufficient data.  Close the connection. */
        if (close(fd) < 0)
            printf("Error on read, but error closing connection too.");
        if (close_fd != NULL)
            *close_fd = 1;
        printf("[AH]: Can't determine type for socket %d.  Closing.\n", fd);

    } else if (ntohl(header) == HARMONY_MAGIC) {
        /* This is a communication socket from a harmony client. */
        printf("[AH]: Socket %d is a harmony client.\n", fd);
        harmony_fds.push_back(fd);

    } else {
        /* Consider this an HTTP communication socket. */
        if (http_fds.size() < http_connection_limit) {
            printf("[AH]: Socket %d is a http client.\n", fd);
            http_fds.push_back(fd);

        } else {
            printf("[AH]: Socket %d is an http client, but we have too many already.  Closing.\n", fd);
            http_send_error(fd, 503, NULL);

            if (close(fd) < 0)
                printf("Error closing HTTP connection.");
            if (close_fd != NULL)
                *close_fd = 1;
        }
    }

    /* Do *not* remove fd from the unknown_list.  We might invalidate any
     * iterators that are currently traversing this list.
     */
}

/*****Server retrieves the necessary paths from the global_config file and sends to the code-generator*****/
void code_gen_paths()
{

  int soc_desc;
  struct sockaddr_in soc_add;
  int addrlen;
  int new_soc, tr;

  char *server_port;

  int portnum;

  char *str1 = all_code_gen_paths();
  char message1[1024];
  sprintf(message1, "%s", str1);
   
  //create the master socket and check it worked
  if ((soc_desc=socket(AF_INET,SOCK_STREAM,0))==0)
  {
    //if socket failed then display error and exit
    perror("Create socket");
    exit(EXIT_FAILURE);
  }

  //type of socket created
  soc_add.sin_family = AF_INET;
  soc_add.sin_addr.s_addr = INADDR_ANY;

  // try to get the port info from the environment
  server_port=getenv("HARMONY_S_PORT");
  if (server_port != NULL)
  {
      portnum=atoi(server_port);
  }
  else
  {
    // if the env var is not defined, use the default
    portnum = atoi(server_portnum());
  }

  soc_add.sin_port = htons(portnum);

  //make the port reusable
  if (setsockopt(soc_desc, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int)) < 0 ) 
  {
    perror("setsockopt");
  }

  //bind the socket to port
  if (bind(soc_desc,(struct sockaddr *)&soc_add,sizeof(soc_add))<0)
  {
    // if bind failed then display error message and exit
    perror("bind");
    exit(EXIT_FAILURE);
  }

  //server listens to the client on this socket
  if (listen(soc_desc,3)<0)
  {
    //if listen failed then display error and exit
    perror("listen");
    exit(EXIT_FAILURE);
  }

  //accept one connection, wait if no connection pending
  addrlen=sizeof(soc_add);
  if ((new_soc=accept(soc_desc,(struct sockaddr *)&soc_add,(unsigned int *)&addrlen))<0)
  {
    //if accept failed to return a socket descriptor, display error and exit
    perror("accept");
    exit(EXIT_FAILURE);
  }

  //inform user of socket number - used in send and receive commands 
  printf("New socket is %d\n",new_soc);

  //transmit message to new connection
  if (send(new_soc,message1,strlen(message1),0)!=strlen(message1))
  {
    //if send failed to send all the message, display error and exit
    perror("send");
    exit(EXIT_FAILURE);

    //shutdown master socket properly
    close(soc_desc);
  }
  else
  {
    puts("All path values have been sent to code-generator");
    close(new_soc);

    //shutdown master socket properly
    close(soc_desc);
  }
   
} 

/***
 *
 * The main loop of the server
 *
 * We only exit on Ctrl-C
 *
 ***/
void main_loop()
{
    int pid;
    int active_sockets;
    int new_fd, close_fd;
    fd_set listen_set_copy;

    /* Start listening for new connections */
    if (listen(listen_socket, SOMAXCONN)) {
        h_exit("Listen to socket failed!");
    }

    while (1) {
        listen_set_copy=listen_set;
        errno = 0;
        active_sockets=select(highest_socket+1,&listen_set_copy, NULL, NULL, NULL);
        if (active_sockets == -1) {
            if (errno == EINTR) {
                continue;
            }
            printf("[AH]: Error during select(): %s\n", strerror(errno));
            break;
        }
	if (debug_mode)
            printf("[AH]: Select:: Return value: %d \n", active_sockets);

        /* Handle new connections */
        if (FD_ISSET(listen_socket, &listen_set_copy)) {
            if (debug_mode)
                printf("[AH]: processing connection request\n");
            new_fd = handle_new_connection(listen_socket);
            if (new_fd > 0) {
                FD_SET(new_fd, &listen_set);
                if (highest_socket < new_fd)
                    highest_socket = new_fd;
            }
        }

        /* Handle unknown connections (Unneeded if we switch to UDP) */
        socketIterator = unknown_fds.begin();
        while (socketIterator != unknown_fds.end()) {
            close_fd = 0;
            if (FD_ISSET(*socketIterator, &listen_set_copy)) {
                if (debug_mode)
                    printf("[AH]: determining connection type\n");
                handle_unknown_connection(*socketIterator, &close_fd);
                if (close_fd) {
                    FD_CLR(*socketIterator, &listen_set);
                }
                socketIterator = unknown_fds.erase(socketIterator);

            } else {
                ++socketIterator;
            }
        }

        /* Handle harmony messages */
        socketIterator = harmony_fds.begin();
        while (socketIterator != harmony_fds.end()) {
            close_fd = 0;
            if (FD_ISSET(*socketIterator, &listen_set_copy)) {
		if(debug_mode)
                    if (clientId.count(*socketIterator) == 0) {
                        printf("[AH]: processing harmony message from unregistered client on socket %d\n", *socketIterator);
                    } else {
                        printf("[AH]: processing harmony message from client %d\n", clientId[*socketIterator]);
                    }
                process_message_from(*socketIterator, &close_fd);
            }

            if (close_fd) {
                FD_CLR(*socketIterator, &listen_set);
                socketIterator = harmony_fds.erase(socketIterator);

            } else {
                ++socketIterator;
            }
        }

        /* Handle http requests */
        socketIterator = http_fds.begin();
        while (socketIterator != http_fds.end()) {
            close_fd = 0;
            if (FD_ISSET(*socketIterator, &listen_set_copy)) {
		if(debug_mode)
                    printf("[AH]: processing http request on socket %d\n", *socketIterator);
                handle_http_socket(*socketIterator, &close_fd);
            }

            if (close_fd) {
                FD_CLR(*socketIterator, &listen_set);
                socketIterator = http_fds.erase(socketIterator);

            } else {
                ++socketIterator;
            }
        }

#if 0
        /*
          This section of the code is only needed if we want to use the
          commandline interface of hserver as a tcl interpreter.
	  This is useful for debugging sessions. The users can query the
	  tcl backend with tcl commands. Uncomment this if you are debugging
	  the backend.
        */
        /*
        if (FD_ISSET(xfd, &listen_set_copy)) {
            printf("should never get here if not using TK \n");
            
            char line[1024];
            int ret1 = read(0, line, sizeof(line));
            assert (ret1 >= 0);
            line[ret1] = '\0';

            if (Tcl_Eval(tcl_inter, line) != TCL_OK) {
                printf("Tcl Error: %s\n", tcl_inter->result);
            } else {
            }

            fflush(stdout);
        }
        */
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

    //Reads the global_config file and retrieves the values assigned to 
    //different variables in this file.
    read_cfg_file();

    check_parameters(argc, argv);

    printf("\nThe Harmony Server waits for the connection request from the code-server \n");

    code_gen_paths();

    server_startup();

    //Retrieving the code generation completion flag from global_config file
    char *cflagdp = code_flag_dest_path();
    strncpy(code_flags_path, cflagdp, sizeof(code_flags_path)-1);
    code_flags_path[255] = '\0';

    main_loop();
}
