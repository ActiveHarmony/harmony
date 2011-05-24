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

/******************************
 *
 * Author: Cristian Tapus
 * History:
 *   Dec 22, 2000 - comments and updates added by Cristian Tapus
 *   Nov 20, 2000 - Dejan added some features to the existing functions
 *   Sept 22, 2000 - comments added and debug moded added
 *   July 9, 2000 - first version
 *   July 15, 2000 - comments and update added by Cristian Tapus
 *   2010 : fix for multiple server connection by George Teodoro
 *   2004-2010 : various fixes and additions by Ananta Tiwari
 *******************************/


/***
 * include other user written headers
 ***/
#include <fcntl.h>
#include "hclient.h"
#include <netinet/in.h>
#include <strings.h>
#include <sys/time.h>
#include <signal.h>
#include <set>
#include <sstream>
#include <assert.h>
#include <iostream>
#include "hserver.h"
using namespace std;

/* Tcl Interpreter */
Tcl_Interp *tcl_inter;

//char harmonyTclFile[256]="hconfig.pro.tcl";
//char harmonyTclFile[256]="hconfig.random.tcl";
char harmonyTclFile[256]="hconfig.nm.tcl";
//char harmonyTclFile[256]="hconfig.brute.tcl";


/* Map containing client AppName and socket */
map<string, int> clientInfo;
map<int, char *> clientName;
map<int, int> clientSignals;
map<int, int> clientSocket;
map<int, int> clientId;

/* this map contains the configurations that we have seen so far and
   their corresponding performances.
*/
map<string,string> global_data;
map<string,string>::iterator it;

#define INTMAXVAL 2147483647

/*
 * global variables
 */

// the purpose of this static variable is to define the debug mode
// when this value is set to 1, different values might be printed for
// debuginng purposes. However, the information that is outputed might
// not make sens (except for the author! :)
static int debug_mode=0;


// the client socket that will support the TCP connection to the server
vector<int>hclient_socket;

// id that identifies an application (used for migration)
vector<int>hclient_id;

int currentIndex=0;
int currentHighestSocketIndex=-1;

// this list of VarDef keeps track of all the variables registered by the
// application to the server. This information is only kept on the client
// side for now, as no use of it was needed on the server side as of now...
// For information about the VarDef class, please consult hmesgs.(ch)
vector<list<VarDef> > reg_vars;

// the timestamp used by the simplex minimization method to decide
// whether the performance reflects the changes of the parameters
vector<int>hclient_timestamp;

// a global variable that has enables/disables the use of signals for
// getting the information from the server. For more information about
// this variable please consult the Programmer's Guide.
int huse_signals;

/*
 * When tuning different functions independently, the users can connect to
 * multiple servers. However there is one limitation -- all the calls to
 * harmony_startups have to be made first before any other harmony calls.
 * Here we use this global variable to track this.
 */
int startup_call_not_allowed=0;

// each time a call to harmony_code_complete is made this variable must
// be updated. makes synchronization much easier at the server side.
unsigned int code_completion_request_timestep=0;
vector<int> code_timesteps;

/***
 * prototypes of side functions
 ***/
void process_update(HUpdateMessage *m);
void hsigio_handler(int s);
char *str_replace(char const * const original, char const * const pattern,char const * const replacement);

/***
 * function definitions
 ***/

/**
 *
 * returns the local socket used for connection to server
 *
 **/
int hget_local_sock() {
    return hclient_socket[0];
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
 *  sport (int)    - the port of the server
 *  shost (char *) - the host of the server
 *  use_sigs (int) - the client will use signals if the value is 1
 *                   the client will use polling if the value is 0
 *                   the client will use sort of a polling if value is 2
 *  relocated (int)- this checks if the client has been migrating lately
 *                   the use of this variable is not very clear yet, but
 *
 * The return value is a handle that can be used to talk to different
 * harmony servers (if the user is using different servers).
 */

int harmony_startup(int sport, char *shost, int use_sigs, int relocated) {
	struct sockaddr_in address;
	struct hostent *hostaddr = NULL;
	char *hostname;
	HRegistMessage *mesg;


    // All startup calls have to be made before any other calls.
    if(startup_call_not_allowed)
    {
        printf("No more startup calls allowed. If you are trying to connect\n");
        printf("to multiple servers or you are trying to use the same server \n");
        printf("to tune multiple code-sections/functions, you have to make \n");
        printf("sure all harmony startup calls are made prior to making any \n");
        printf("other harmony calls.\n");
        hc_exit("No more startup calls allowed...");
    }


    currentHighestSocketIndex=currentHighestSocketIndex+1;
    int socketIndex=currentHighestSocketIndex;

    if (debug_mode)
        printf("Harmony: Current Socket Index %d %d \n", socketIndex, hclient_socket.size());

	// create local state variable for each new function to tune
	while(hclient_socket.size() <= currentHighestSocketIndex)
	{
        code_timesteps.push_back(1);
		hclient_socket.push_back(-1);
		hclient_id.push_back(-1);
        hclient_timestamp.push_back(-1);
		reg_vars.push_back(*(new list<VarDef>));
	}

    if(debug_mode)
        printf("Length of reg_vars is now: %d \n", reg_vars.size());

	char temps[BUFFER_SIZE];

	int fileflags;

	/* set the flag that tells if we use signals */
	huse_signals = use_sigs;

	/* create socket */
	if ((hclient_socket[socketIndex]=socket(AF_INET,SOCK_STREAM,0))<0) {
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

    if (debug_mode)
        printf("Connecting to %s at port %d \n", shost, sport);

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
	int ret = setsockopt(hclient_socket[socketIndex], SOL_SOCKET, SO_REUSEADDR, (char *) &optVal, sizeof(int));
	if (ret) {
		perror("setsockopt");
		hc_exit("<setsockopt>fd_create");
	}

	// try to connect to the server
	if (connect(hclient_socket[socketIndex],(struct sockaddr *)&address, sizeof(address))<0)
		hc_exit("Connection failed!");

	/* send the registration message */
	mesg = new HRegistMessage(HMESG_CLIENT_REG, huse_signals);

	if (relocated){
		assert(hclient_id[socketIndex] > 0);
		mesg->set_id(hclient_id[socketIndex]);
	}
	send_message(mesg,hclient_socket[socketIndex]);

	delete mesg;

	/* wait for confirmation */

	mesg = (HRegistMessage *)receive_message(hclient_socket[socketIndex]);
	hclient_id[socketIndex] = mesg->get_id();

    if (debug_mode)
        printf("check id: %d \n", mesg->get_id());

	assert(hclient_id[socketIndex] > 0);
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
		if (fcntl(hclient_socket[socketIndex], F_SETOWN, getpid()) < 0) {
			hc_exit("fcntl F_SETOWN");
		}

		if (fileflags = fcntl(hclient_socket[socketIndex], F_GETFL ) == -1) {
			hc_exit("fcntl F_GETFL");
		}

#ifdef LINUX
		// this does not work on SOLARIS
		if (fcntl(hclient_socket[socketIndex], F_SETFL, fileflags | FASYNC) == -1)
			hc_exit("fcntl F_SETFL, FASYNC");
#endif

	}
    return currentHighestSocketIndex;
}


/*
 * When a program announces the server that it will end it calls this
 * function.
 *  If the user is using only one server, then there is no need to supply
 * the parameter (defualt to 0). Otherwise, to disconnect a particular
 * connection, the handle returned by the startup call has to be sent in as
 * a parameter.
 */
void harmony_end(int socketIndex){
	HRegistMessage *m = new HRegistMessage(HMESG_CLIENT_UNREG,0);

	// send the unregister message to the server
	send_message(m,hclient_socket[socketIndex]);

	delete m;

	/* wait for server confirmation */

	m =(HRegistMessage *) receive_message(hclient_socket[socketIndex]);


	delete m;

	// close the actual connection
	close(hclient_socket[socketIndex]);
}


/*
 * Inform the Harmony server of the bundles and requirements of the
 * application.
 * If the user is using only one server, then there is no need to supply
 * the parameter (defualt to 0). Otherwise, to send specific application
 * description to specific server, the handle returned by the startup
 * call has to be sent in as
 * a parameter.
 */
void harmony_application_setup(char *description, int socketIndex){
    startup_call_not_allowed=1;

    HDescrMessage *mesg=new HDescrMessage(HMESG_APP_DESCR,description,strlen(description));
	send_message(mesg, hclient_socket[socketIndex]);
	delete mesg;

	/* wait for confirmation */
	HMessage *m;
	m=receive_message(hclient_socket[socketIndex]);

	if (m->get_type()!=HMESG_CONFIRM) {
		// something went wrong on the server side !
		delete m;
		hc_exit("Application description is not correct!\n");
	}
	delete m;
}

/***
 * this version of the function reads the description from a given filename
 * and passes it as a string to the above defined function
 ***/
void harmony_application_setup_file(char *fname, int socketIndex) {
    startup_call_not_allowed=1;
    if (debug_mode)
        printf("Socket Index: %d \n", socketIndex);
    struct stat *statb=(struct stat *)malloc(sizeof(struct stat));
    int fd;
    char *d;


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
    harmony_application_setup(d, socketIndex);
}

/*
 * Bind a local variable with a harmony bundle
 *
 * Params:
 * 	appName (char *) - the name of the application
 *   	bundleName (char *) - the name of the bundle we want to bind
 *	type (int) the type of the variable we want to bind: INT, STR
 *  socketINdex (int): to connect to specific server
 *	localonly (int) - The use is not very clear to me
 *			  I guess he doesn't want the variable to have the
 *				name defined as the others have, but rather
 *				take the name of the bundle (Have not tested this aspect thoroughly).
 */

void * harmony_add_variable(char *appName, char *bundleName, int type, int socketIndex, int localonly){
    startup_call_not_allowed=1;
    if (appName == NULL)
        appName = "client";

    if (localonly) {
        VarDef v(bundleName, type);
        v.alloc();
        reg_vars[socketIndex].push_back(v);
        // return the pointer of the variable
        void *r = reg_vars[socketIndex].rbegin()->getPointer();
        return r;
    }
    // the variable
    VarDef v;
    // the variable name as known by TCL
    char varN[MAX_VAR_NAME];
    if (debug_mode) 
    {
        fprintf(stderr, "appName:%s ; bundleName:%s socketIndex %d \n",appName, bundleName, socketIndex);
        fprintf(stderr, "appName:%s ; bundleName:%s\n",appName, bundleName);
    }

    // create the name of the variable as described in the Tcl code that
    // accompanies Harmony
    sprintf(varN,"%s_%d_bundle_%s",appName,hclient_id[socketIndex],bundleName);

    if (debug_mode) 
    {
        printf("Adding variable: %s \n", varN);
        fprintf(stderr, "Variable Tcl Name: %s ; Type %d ; Strlen:%d \n",varN, type, strlen(varN));
    }

    // send variable registration message
    HDescrMessage *m=new HDescrMessage(HMESG_VAR_DESCR, bundleName, strlen(bundleName));
    send_message(m, hclient_socket[socketIndex]);
    delete m;

    // wait for confirmation
    // the confirmation message also contains the initial value of the
    // variable
    HMessage *mesg;
    mesg=receive_message(hclient_socket[socketIndex]);

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

    if(debug_mode)
        fprintf(stderr, "v.getName()=%s\n", v.getName());

    delete mesg;

    reg_vars[socketIndex].push_back(v);
    //unblock_sigio();

    // return the pointer of the variable
    if (debug_mode)
        printf("Checking the first: %s \n", reg_vars[0].rbegin()->getName());

    return reg_vars[socketIndex].rbegin()->getPointer();
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
 * Get the value of a bound variable.
 */
void *  harmony_request_variable(char *variable, int socketIndex){
    startup_call_not_allowed=1;
    // lookup variable by the pointer in the registered variables list
    int  dummy = -100000;
    int *ip = &dummy;

    list<VarDef>::iterator VarIterator =
        find_if(reg_vars[socketIndex].begin(), reg_vars[socketIndex].end(),findVarbyName(variable));

    // if variable is not found just print an error message and continue
    if (VarIterator==reg_vars[socketIndex].end()) {
        fprintf(stderr, "(request_var)Harmony: Variable not registered: %s\n", ((VarDef*)variable)->getName());
        return (int*) ip;
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

            return (int *) ip;
        }

        // signals are not used

        // send request message
        HUpdateMessage *m=new HUpdateMessage(HMESG_VAR_REQ, 0);
        m->set_var(*VarIterator);


        if (debug_mode)
            m->print();

        send_message(m, hclient_socket[socketIndex]);
        delete m;

        // wait for answer
        HMessage *mesg = receive_message(hclient_socket[socketIndex]);

        if (mesg->get_type()==HMESG_FAIL) {
            // the server could not return the value of the variable
            delete mesg;
            fprintf(stderr, "Harmony error: Server could not return value of variable!\n");
            return ip;
        }

        m=(HUpdateMessage *)mesg;
        hclient_timestamp[socketIndex] = m->get_timestamp();
        if (debug_mode)
            m->print();

        // update the value
        switch (VarIterator->getType()) {
            case VAR_INT:
                VarIterator->setValue(*(int *)m->get_var(0)->getPointer());
                return (int *)m->get_var(0)->getPointer();
                break;
            case VAR_STR:
                VarIterator->setValue((char *)m->get_var(0)->getPointer());
                return (char *)m->get_var(0)->getPointer();
                break;
        }

        delete m;
    }

}


/*
 * Get the current value of all the bound variables
 *
 */

int harmony_request_all(int socketIndex, int pull)
{
  
    // here we also have to differentiate between use of signals and no signals
    list<VarDef>::iterator VarIterator = reg_vars[socketIndex].begin();
    
    if (huse_signals && !pull) {
        // we use signals, so we just have to copy the shadow to the actual value
        for (;VarIterator!=reg_vars[socketIndex].end();VarIterator++) {
            switch (VarIterator->getType()) {
                case VAR_INT:
                    VarIterator->setValue(*(int *)VarIterator->getShadow());
                    break;
                case VAR_STR:
                    VarIterator->setValue((char *)VarIterator->getShadow());
                    break;
            }
        }
        return 1;
    }

    // if signals are not used
    // send request message
    HUpdateMessage *m=new HUpdateMessage(HMESG_VAR_REQ, 0);

    for (;VarIterator!=reg_vars[socketIndex].end();VarIterator++)
        m->set_var(*VarIterator);

    if (debug_mode)
        m->print();

    send_message(m, hclient_socket[socketIndex]);
    delete m;

    // wait for the answer from the harmony server
    HMessage *mesg=(HMessage *)receive_message(hclient_socket[socketIndex]);

    printf("The message type is : %d \n", mesg->get_type());

    if (mesg->get_type()==HMESG_FAIL) 
    {
        // the server could not return the value of the variable
        delete mesg;
        fprintf(stderr, "Harmony error: Server could not return value of variable!\n");
	return 0;
      
    }
   
    else if (mesg->get_type()==HMESG_PERF_UPDT)
    {
      
      int i;

      //Updating the message
      m=(HUpdateMessage *)mesg;

      //Updating the timestamp
      hclient_timestamp[socketIndex] = m->get_timestamp();

      if (debug_mode)
	{
	  m->print();
	}
     
      //Updating the variables from the content of the message
      for (VarIterator=reg_vars[socketIndex].begin(),i=0;VarIterator!=reg_vars[socketIndex].end();VarIterator++,i++)
	{
	  switch (VarIterator->getType())
	    {
	       case VAR_INT:
		 VarIterator->setValue(*(int *)m->get_var(i)->getPointer());
		 break;
	       case VAR_STR:
		 VarIterator->setValue((char *)m->get_var(i)->getPointer());
		 break;
	    }
	}
      

      delete m;
      
      // 0 means we have to evaluate the conf and send in the performance 
      // using the harmony_performance_update
      return 0;    
    }

    else 
    {
      int i;

      // the perf is already evaluated
      printf("The performance has been evaluated before \n");
      
      //Extracting the performance value and then reporting the performance back to the server.
      //Updating the variable with the received performance
      //Also updating the message to be sent with the performance
      //Updating the message

      m=(HUpdateMessage *)mesg;

      //Updating the timestamp
      hclient_timestamp[socketIndex] = m->get_timestamp();

      if (debug_mode)
      {
        m->print();
      }


      // update the variable: REMEMBER: the last entry on the variable list is the performance VarDef
      //int num_tunable_params=mesg->get_nr_vars()-1;
      int num_tunable_params=m->get_nr_vars();

      for (VarIterator=reg_vars[socketIndex].begin(),i=0;VarIterator!=reg_vars[socketIndex].end();VarIterator++,i++)
      {
	switch (VarIterator->getType())
	{
	  case VAR_INT:
	     VarIterator->setValue(*(int *)m->get_var(i)->getPointer());
	     break;
	  case VAR_STR:
	     VarIterator->setValue((char *)m->get_var(i)->getPointer());
	     break;
	}
      }

      delete m;

      // 1 means the server has the performance value and we have already made a call to
      // harmony_performance_update. So, do not make a duplicate call.
      return 1;     
    }


}

/*
 * Send the performance function (int) result to the harmony server
 */
void harmony_performance_update(int value, int socketIndex)
{

    startup_call_not_allowed=1;
    // create a new variable
    VarDef *v = new VarDef("obsGoodness",VAR_INT);
    v->setValue(value);
    HUpdateMessage *m=new HUpdateMessage(HMESG_PERF_UPDT, 0);

    if (debug_mode)
        printf("index: %d socket %d timestamp %d performance %d \n", socketIndex, hclient_socket[socketIndex], hclient_timestamp[socketIndex], value);

    // first set the timestamp to be sent to the server
    m->set_timestamp(hclient_timestamp[socketIndex]);
    m->set_var(*v);

    if (debug_mode)
    {
        m->print();
        printf("Sending the message to the server \n");
    }

    send_message(m, hclient_socket[socketIndex]);
    delete m;
    
    // wait for the answer from the harmony server
    HMessage *mesg=(HMessage *)receive_message(hclient_socket[socketIndex]);
    if (debug_mode)
        printf("Receiving the message from the server \n");
    if (mesg->get_type()==HMESG_FAIL) {
        // could not send performance function
        delete mesg;
        hc_exit("Server failed to send configuration! \n");
    }
    delete mesg;
}

/*
 * redundant definition. will have to change this in the future.
 * Send the performance function (double) result to the harmony server
 */
void harmony_performance_update(double value, int socketIndex)
{
    // create a new variable
    VarDef *v = new VarDef("obsGoodness",VAR_STR);
    char c_value[256];
    sprintf(c_value, "%.15g", value);
    v->setValue(c_value);
    HUpdateMessage *m=new HUpdateMessage(HMESG_PERF_UPDT, 0);

    if (debug_mode)
        printf("socket %d timestamp %d \n", hclient_socket[socketIndex], hclient_timestamp[socketIndex]);

    // first set the timestamp to be sent to the server
    m->set_timestamp(hclient_timestamp[socketIndex]);
    m->set_var(*v);

    if (debug_mode)
        m->print();

    send_message(m, hclient_socket[socketIndex]);
    delete m;

    // wait for the answer from the harmony server
    HMessage *mesg=(HMessage *)receive_message(hclient_socket[socketIndex]);
    if (mesg->get_type()==HMESG_FAIL) {
        // could not send performance function
        delete mesg;
        hc_exit("Failed to send performance function! \n");
    }
    delete mesg;
}

/*
 * get the value of any tcl backend variable - for ex. next_iteration
 *  this handles int variables. NOT EXPOSED.
 */
void* harmony_request_tcl_variable_(char *variable, int socketIndex){
    // define the VarDef
    if (debug_mode)
        printf("tcl_request: %d %s \n", socketIndex, variable);
printf("tcl_request: %d %s \n", socketIndex, variable);
    VarDef *var = new VarDef(variable, VAR_STR);
    HUpdateMessage *m=new HUpdateMessage(HMESG_TCLVAR_REQ, 0);

    // set a dummy value
    var->setValue("tcl_var");
    m->set_timestamp(hclient_timestamp[socketIndex]);
    m->set_var(*var);

    if (debug_mode)
        printf("tcl_request: %d %s \n", socketIndex, variable);

    send_message(m,hclient_socket[socketIndex]);

    //delete var;
    delete m;

    // wait for answer
    HMessage *mesg = receive_message(hclient_socket[socketIndex]);
    if (mesg->get_type()==HMESG_FAIL) {
        // the server could not return the value of the variable
        printf("The Server failed to return the value of the tcl variable!!\n");
        delete mesg;
        /* this needs to be checked thoroughly. */
        hc_exit("error in tcl vairable retrieval. Is the variable defined in the tcl backend? \n");
    }

    m=(HUpdateMessage *)mesg;

    VarDef *v = m->get_var(0);
    void* result =  (void*)m->get_var(0)->getPointer();
    return result;
}

/*
 * psuedo barrier using: DO NOT USE THIS FOR ONLINE TUNING
 *  For large search spaces, it is possible that harmony takes quite some
 * time to calculate new configurations. Once the configurations are
 * calculated, the next_iteration variable in the tcl backend is set to
 * 1. We are watching for that variable. Use this very carefully. We are
 * using a spin-lock mechanism.
 */
void harmony_psuedo_barrier(int socketIndex)
{
    int next_iteration=0;
    // psuedo barrier using: DO NOT USE THIS FOR ONLINE TUNING
    next_iteration=atoi((char*)harmony_request_tcl_variable_("next_iteration", socketIndex));
    if(debug_mode)
        printf("%d \n ", next_iteration);
    while(next_iteration != 1)
    {
        sleep(1);
        next_iteration=atoi((char*)harmony_request_tcl_variable_("next_iteration", socketIndex));
    }
}

/*
 * Ask for the best configuration that we have seen so far during this
 * tuning run. The configuration is returned as a char pointer. The
 * parameters are separated by underscores. The application needs to parse this.
*/
char* harmony_get_best_configuration(int socketIndex)
{
    char* return_val = (char*)harmony_request_tcl_variable_("best_coord_so_far", socketIndex);
    char* underscore_sep=str_replace(return_val," ", "_");
    // make sure you free the return val in the client side.
    return underscore_sep;
}


/*
 * This checks whether the search algorithm is done.
 *
 */
int harmony_check_convergence(int socketIndex)
{
    char temporary[15]="search_done";
    if (debug_mode)
        printf("calling the tcl_var_req %d \n", socketIndex);
    void *return_val=harmony_request_tcl_variable_("search_done",socketIndex);

    if (debug_mode)
        printf("harmony_check_convergence: %s \n", (char*)return_val);

    if(atof((char*)return_val) == 1)
        return 1;
    return 0;
}

/*
 for compatability (some users might be using this function already).
 Will have to get rid of this in the future.
*/
int code_generation_complete(int socketIndex)
{
     // define the VarDef
    char* variable = "code_completion";
    VarDef *var = new VarDef(variable, VAR_INT);

    //printf("sending the code generation query \n");
    // define a message
    HUpdateMessage *m=new HUpdateMessage(HMESG_CODE_COMPLETION, 0);

    // first update the code_timestep
    int code_completion_req_timestep=code_timesteps[socketIndex];
    code_timesteps[socketIndex]=code_completion_req_timestep+1;
    var->setValue(code_completion_req_timestep);
    m->set_timestamp(code_completion_req_timestep);
    m->set_var(*var);

    send_message(m,hclient_socket[socketIndex]);

    //delete var;
    delete m;

    //printf("waiting for the code generation response socke: %d\n",hclient_socket[socketIndex]);
    // wait for answer
    HMessage *mesg = receive_message(hclient_socket[socketIndex]);
    //printf("heard back from the server \n");
    if (mesg->get_type()==HMESG_FAIL) {
        // the server could not return the value of the variable
        printf("The Server failed to return the respose to code generation!!\n");
        delete mesg;
        return -1;
    }

    m=(HUpdateMessage *)mesg;
    //printf("heard back from from the server about code generation  \n");
    VarDef *v = m->get_var(0);
    int result =  *(int *)m->get_var(0)->getPointer();
    //printf("Code generation result is %d \n", result);
    delete m;
    return result;
}

int harmony_code_generation_complete(int socketIndex)
{
    return code_generation_complete(socketIndex);
}


/*
 * ask for the performance value of previously evaluated
 * configurations. Mostly used within an offline tuning scenario to avoid
 * duplicate evaluation of configurations. The server uses a simple map to
 * keep track of previously evaluated configurations.
 * Utlimately, once performance databases become readily available, we can
 * modify this to pull data from those databases.
 *  Contextual information should be taken into account while pulling such
 *  data. (Just a note to future Active Harmony developers.)
*/


void* harmony_database_lookup(int socketIndex)
{
    VarDef *var = new VarDef("database", VAR_STR);
    // define a message
    HUpdateMessage *m=new HUpdateMessage(HMESG_DATABASE, 0);

    char  dummy[10]="-100000";
    char *ip = dummy;

    var->setValue("100");
    m->set_timestamp(hclient_timestamp[socketIndex]);
    m->set_var(*var);

    if (debug_mode)
        m->print();

    send_message(m,hclient_socket[socketIndex]);

    // wait for answer
    HMessage *mesg = receive_message(hclient_socket[socketIndex]);

    if (mesg->get_type()==HMESG_FAIL) {
        // the server could not return the value of the variable
        printf("The Server failed to return perf from database... \n");
        delete mesg;
        /* this needs to be checked thoroughly. */
        return ip;
    }

    m=(HUpdateMessage *)mesg;

    if (debug_mode)
        m->print();

    VarDef *v = m->get_var(0);

    void* perf_dbl=(void*)(m->get_var(0)->getPointer());

    //double result=atof(perf_dbl);
    return perf_dbl;
}


/* There might still be some users using this. This will be removed in the
 * next release.
 *
 * Get the value of a probe variable
 * for now, this is sort of a temporary solution. the probe variables are
 * registered in the tcl backend only.
 * A new message type has been defined to take care of a separate type
 * of Update Message.
 * Probe variables can be used to keep track of application variables and
 * metrics that do not get factored into the optimization process.
 */
int harmony_request_probe(char *variable, int type, int socketIndex){

    // define the VarDef

    VarDef *var = new VarDef(variable, VAR_INT);

    // define a message

    HUpdateMessage *m=new HUpdateMessage(HMESG_PROBE_REQ, 0);

    var->setValue(0);
    m->set_timestamp(hclient_timestamp[socketIndex]);
    m->set_var(*var);

    if (debug_mode)
        m->print();

    send_message(m, hclient_socket[socketIndex]);

    delete m;

    // wait for answer
    HMessage *mesg = receive_message(hclient_socket[socketIndex]);

    if (mesg->get_type()==HMESG_FAIL) {
        // the server could not return the value of the variable
        printf("The Server failed to return the value of the probe variable!!\n");
        delete mesg;
        /* this needs to be checked thoroughly. */
        return -1;
    }

    m=(HUpdateMessage *)mesg;

    if (debug_mode)
        m->print();

    VarDef *v = m->get_var(0);
    int result =  *(int *)m->get_var(0)->getPointer();
    delete m;

    return result;
}

/* this takes care of the updating for probe variables.
 */
void harmony_set_probe_perf(char *variable, int value, int socketIndex){
    // define the VarDef
    VarDef *var = new VarDef(variable, VAR_INT);
    // define a message
    HUpdateMessage *m=new HUpdateMessage(HMESG_PROBE_SET, 0);

    var->setValue(value);
    var->setType(VAR_INT);
    m->set_timestamp(hclient_timestamp[socketIndex]);
    m->set_var(*var);

    if (debug_mode)
        m->print();

    send_message(m, hclient_socket[socketIndex]);

    delete m;
    // wait for answer??

    return;
}

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

    if (debug_mode)
        fprintf(stderr, "***** Got SIGIO %d !\n",s);

    while(1){
      again:

        // initialize the set of modified file descriptor to check for
        // harmony socket only
        FD_ZERO(&modif_sock);
        FD_SET(hclient_socket[currentIndex],&modif_sock);

        // set tv for polling
        struct timeval tv =  {0, 0};
        int res;

        if ((res = select(hclient_socket[currentIndex]+1,&modif_sock, NULL, NULL, &tv)) < 0) {
            if (errno = EINTR)
                goto again;
            perror("ERROR in SELECT");
            hc_exit("Error in select");
        }

        if (!res)
            break;

        // if the harmony communication socket is one of the modified
        // sockets, then we have a packet from the server
        if (FD_ISSET(hclient_socket[currentIndex],&modif_sock)) {
            // check the message from the server

            // we can read data from the socket. define for this a message
            // we know that from the server we only get update messages
            HUpdateMessage *m;

            m = (HUpdateMessage *) receive_message(hclient_socket[currentIndex]);

            // print the content of the message if in debug mode
            if (debug_mode)
                m->print();

            process_update(m);
            delete m;
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
#endif  /*SOLARIS*/
    hsigio_blocked = TRUE;
};

void unblock_sigio() {
#ifdef DEBUG_SIGIO
    if (!hsigio_blocked) {
        fprintf(stderr, "WARNING: Improper state or use of unblock_sigio\n");
    }
#endif  /*DEBUG_SIGIO*/
    hsigio_blocked = FALSE;
#ifdef  SOLARIS
    sigset_t    set;
    sigemptyset(&set);
    sigprocmask(SIG_SETMASK, &set, NULL);
#else
    sigsetmask(0);
#endif  /*SOLARIS*/
};

/*
 * Send to the server the value of a bound variable
 */
void harmony_set_variable(void *variable, int socketIndex){
    startup_call_not_allowed=1;
    // get the variable associated with the pointer
    list<VarDef>::iterator VarIterator =
        find_if(reg_vars[socketIndex].begin(), reg_vars[socketIndex].end(),findVarbyPointer(variable));

    // if variable was not found just print an error and continue
    if (VarIterator==reg_vars[socketIndex].end()) {
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

        send_message(m, hclient_socket[socketIndex]);
        delete m;

        // wait for the answer from the harmony server
        HMessage *mesg=(HMessage *)receive_message(hclient_socket[socketIndex]);

        if (mesg->get_type()==HMESG_FAIL) {
            /* could not set variable value */
            delete mesg;
            hc_exit("Failed to set variable value!");
        } else if (mesg->get_type()==HMESG_VAR_REQ) {
            /* received an update */
            process_update((HUpdateMessage*)mesg);
        }
        delete mesg;
    }
}

/*****
      Name:        Process an update message
      Description: Set shadow values of updated variables to received values
      Called from: sigio_handler and harmony_set_variable
*****/
void process_update(HUpdateMessage *m) {
    for (int i = 0; i < m->get_nr_vars(); i++) {
        VarDef *v = m->get_var(i);
        list<VarDef>::iterator VarIterator =
            find_if(reg_vars[0].begin(), reg_vars[0].end(),findVarbyName(v->getName()));

        // if the variable was not found just print an error message and
        // continue. Maybe the server messed things up and data was sent to
        // the bad client. Who knows what might have happened!
        if (VarIterator==reg_vars[0].end()) {
            fprintf(stderr, "Harmony error: Variable not registerd: %s!\n", v->getName());
        } else {
            switch (v->getType()) {
                case VAR_INT:
                    VarIterator->setShadow(*(int*)v->getPointer());
                    break;
                case VAR_STR:
                    VarIterator->setShadow((char*)v->getPointer());
                    //fprintf(stderr, "new val: %s [%s] shadow [%s]\n", v->getName(),
                    //(char*)(VarIterator->getPointer()),
                    //        VarIterator->getShadow());
                    break;
            }
        }
    }
}

/*
 * Update bound variables on server'side.
 */
void harmony_set_all(int socketIndex){

    //block_sigio();
    // go through the entire list of registered variables and create a
    // message containing all these variables and their values

    list<VarDef>::iterator VarIterator;

    HUpdateMessage *m=new HUpdateMessage(HMESG_VAR_SET, 0);

    for (VarIterator=reg_vars[socketIndex].begin();VarIterator!=reg_vars[socketIndex].end(); VarIterator++) {
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
    send_message(m, hclient_socket[socketIndex]);
    delete m;

    // wait for the confirmation
    HMessage *mesg=receive_message(hclient_socket[socketIndex]);

    if (mesg->get_type()==HMESG_FAIL) {
        /* could not set variable value */
        delete mesg;
        hc_exit("Failed to set variables!");
    } else if (mesg->get_type()==HMESG_VAR_REQ) {
        /* received an update */
        process_update((HUpdateMessage*)mesg);
    }

    delete mesg;
}


char *str_replace(char const * const original,
    char const * const pattern,
    char const * const replacement) 
{
  size_t const replen = strlen(replacement);
  size_t const patlen = strlen(pattern);
  size_t const orilen = strlen(original);

  size_t patcnt = 0;
  const char * oriptr;
  const char * patloc;

  // find how many times the pattern occurs in the original string
  for (oriptr = original; patloc = strstr(oriptr, pattern); oriptr = patloc + patlen)
  {
    patcnt++;
  }

  //{
    // allocate memory for the new string
    size_t const retlen = orilen + patcnt * (replen - patlen);
    char * const returned = (char *) malloc( sizeof(char) * (retlen + 1) );

    if (returned != NULL)
    {
      // copy the original string, 
      // replacing all the instances of the pattern
      char * retptr = returned;
      for (oriptr = original; patloc = strstr(oriptr, pattern); oriptr = patloc + patlen)
      {
        size_t const skplen = patloc - oriptr;
        // copy the section until the occurence of the pattern
        strncpy(retptr, oriptr, skplen);
        retptr += skplen;
        // copy the replacement 
        strncpy(retptr, replacement, replen);
        retptr += replen;
      }
      // copy the rest of the string.
      strcpy(retptr, oriptr);
    }
    return returned;
    //}
}
