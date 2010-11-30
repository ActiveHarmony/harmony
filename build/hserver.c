
/***
 *
 * include user defined headers
 *
 ***/
#include "hserver.h"
#include <iostream>
#include <assert.h>
#include <sstream>
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
static int debug_mode=0;

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
int highest_socket;
list<int>::iterator socketIterator;

list<int> id_set;

/* the X file descriptor */
int xfd;


/*
 * The Tcl Interpreter
 */
Tcl_Interp *tcl_inter;
char harmonyTclFile[256]="hconfig.tcl";


/*
  the map containing client AppName and Socket
*/
map<string, int> clientInfo;
map<int, char *> clientName;
map<int, int> clientSignals;
map<int, int> clientSocket;
map<int, int> clientId;


/* this map contains the configurations that we have seen so far and
   their corresponding performances.
 */

map<string,double> global_data;
map<string,double>::iterator it;

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

    for(int i = 1; i <= 300; i++)
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

    if ((s=Tcl_GetVar(tcl_inter,ss,0))==NULL) {

        free(sss);
        operation_failed(client_socket);
        return;
    }

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
void variable_request(HUpdateMessage *mesg, int client_socket){

    char *s;
    char ss[256], ss2[256];

    VarDef v;
    for (int i=0; i<mesg->get_nr_vars();i++) {

        sprintf(ss2,"%s(value)",mesg->get_var(i)->getName());
        char  *appName = clientName[client_socket];
        sprintf(ss, "%s_bundle_%s", appName, ss2);
        s=Tcl_GetVar(tcl_inter,ss,0);

        if (s==NULL) {
            printf("Error getting value of variable!\n");
            free(ss);
            operation_failed(client_socket);
            return;
        }

        VarDef *v;

        switch (mesg->get_var(i)->getType()) {
            case VAR_INT:
                /* is int */
                mesg->get_var(i)->setValue(strtol(s,NULL,10));
                break;
            case VAR_STR:
                mesg->get_var(i)->setValue(s);
                break;
        }

    }

    char  *appName = clientName[client_socket];

    sprintf(ss, "%s_simplex_time", appName);

    s=Tcl_GetVar(tcl_inter,ss,0);

    if (s==NULL) {
        printf("Error getting value of variable! ss:: %s \n", ss);
        free(ss);
        operation_failed(client_socket);
        return;
    }

    mesg->set_timestamp(strtol(s,NULL,10));

    send_message(mesg, client_socket);

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


    if(debug_mode)
        printf("received a tcl variable request \n");

    for (int i=0; i<mesg->get_nr_vars();i++) {
        sprintf(ss2,"%s",mesg->get_var(i)->getName());
        char  *appName = clientName[client_socket];
        sprintf(ss, "%s_%s", appName, ss2);
        if(debug_mode)
            printf("tcl string:: %s \n", ss);
        s=Tcl_GetVar(tcl_inter,ss,0);

        if(debug_mode)
            printf(" Tcl backend variable! %s\n",s);
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

int code_timestep = 1;

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

/*
  Check the code_complete.<timestep> file to see if the code generation is
  complete. This is used with the standalone version of the code generator.
*/

void check_code_completion(HUpdateMessage *mesg, int client_socket){

    char *s;
    char ss[256], ss2[256];

    // retrieve the timestep
    int timestep = *(int *)mesg->get_var(0)->getPointer();


    int return_val=0;

    // based on file system right now.
    sprintf(ss, "/scratch0/code_flags/code_complete.%d", timestep);
    if(file_exists(ss))
    {

        printf("CODE GENERATION IS COMPLETE for timestep: %d \n", timestep);
        return_val=1;
        std::remove(ss);
        sprintf(ss, "/scratch0/code_flags/code_complete.%d", timestep+1);
        std::remove(ss);
    }

    //VarDef *v;
    mesg->get_var(0)->setValue(return_val);

    send_message(mesg, client_socket);
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
        sprintf(curr_conf, "%s", tcl_inter->result);
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

/***
 *
 * The client unregisters! Send back a confirmation message
 *
 ***/
void client_unregister(HRegistMessage *m, int client_socket) {

    /* clear all the information related to the client */

    if(debug_mode)
    {
        printf("ENTERED CLIENT UNREGISTER!!!!!\n");
    }

    map<string,double>::iterator it;

    char temp[256];

    sprintf(temp,"unique_points");


    if(global_data.size() !=0)
    {
        //print out all the confs stored in the map
        FILE *pFile;
        pFile = fopen (temp,"a");
        char cmd[256];
        sprintf(cmd,"-----------------\n");
        if (pFile!=NULL)
        {
            fputs (cmd,pFile);
        }
        for ( it=global_data.begin() ; it != global_data.end(); it++ )
        {
            cout << (*it).first << " => " << (*it).second << endl;
            string first_part=((*it).first);
            char *char_star_first;
            char *char_star_second;
            char_star_first = new char[(*it).first.length() + 1];
            strcpy(char_star_first, (*it).first.c_str());
            if (pFile!=NULL)
            {
                fputs (char_star_first,pFile);
                fprintf(pFile, " ==> ");
                {
                    stringstream ss;
                    ss.str("");
                    ss << (*it).second;
                    fprintf(pFile," %s", ss.str().c_str());
                }
                fprintf(pFile," %d", (*it).second);
                fputs("\n",pFile);
            }
        }
        fclose (pFile);
        global_data.clear();
    }

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
            sprintf(curr_conf, "%s", tcl_inter->result);
        }

        string str_conf = curr_conf;

        if(performance != INTMAXVAL)
            global_data.insert(pair<string, double>(str_conf, (double)performance));
    }

    sprintf(s, "updateObsGoodness %s %d %d", appName,performance, m->get_timestamp());

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

    printf("performance is: %s \n", perf_dbl);

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
            sprintf(curr_conf, "%s", tcl_inter->result);
        }

        string str_conf = curr_conf;

        if(performance != INTMAXVAL)
            global_data.insert(pair<string, double>(str_conf, (double)performance));
    }


    sprintf(s, "updateObsGoodness %s %.15g %d", appName,performance, m->get_timestamp());

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

    global_data.insert(pair<string, int>(str_conf, *(int *)m->get_var(0)->getPointer()));

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


    /* do listening to the socket */
    if (listen(listen_socket, SOMAXCONN)) {
        h_exit("Listen to socket failed!");
    }

    while (1) {
        listen_set_copy=listen_set;
        //    printf("Select\n");
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
