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
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <errno.h>
#include <string.h>

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <map>

#include "hserver.h"
#include "httpsvr.h"
#include "hglobal_config.h"

using namespace std;

/*
 * Function prototypes
 */
int codegen_init();

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
static int debug_mode = 1;

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
int next_id = 1;

/*
 * The Tcl Interpreter
 */
Tcl_Interp *tcl_inter;
char harmonyTclFile[256];

/*
 * the map containing client AppName and Socket
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
 * their corresponding performances.
 */
map<string,string> global_data;
map<string,string>::iterator it;

map<string,string> appName_confs;
map<string,string>::iterator it2;

vector<string> coord_history;

char new_code_path[256];

/* Code-generation completion flag */
char *flag_dir;

/*
 * Other global variables
 */
int update_sent = FALSE;
char *application_trigger = "";

string history_since(unsigned int last)
{
    stringstream ss;
    unsigned int max = coord_history.size();

    while (last < max) {
        ss << "coord:" << coord_history[last++] << "|";
    }
    return ss.str();
}

/***
 *
 * Check the arguments given to the server
 *
 ***/
int check_parameters()
{
    char *search_algo;

    /* Gets the Search Algorithm and the config file 
     * that the user has selected in the hglobal_config file.
     */
    search_algo = cfg_getlower("search_algorithm");
    if (search_algo == NULL)
    {
        printf("[AH]: Error: Search algorithm unspecified.\n");
        return -1;
    }

    if (strcmp(search_algo, "pro") == 0)
    {
        printf("[AH]: Using the PRO search algorithm.  Please make sure\n");
        printf("[AH]:   the parameters in ../tcl/pro_init_<appname>.tcl are"
               " defined properly.\n");
        sprintf(harmonyTclFile, "hconfig.pro.tcl");
    }
    else if (strcmp(search_algo, "nelder mead") == 0)
    {
        printf("[AH]: Using the Nelder Mead Simplex search algorithm.\n");
        sprintf(harmonyTclFile, "hconfig.nm.tcl");
    }
    else if (strcmp(search_algo, "random") == 0)
    {
        printf("[AH]: Using random search algorithm.\n");
        sprintf(harmonyTclFile,"hconfig.random.tcl");
    }
    else if (strcmp(search_algo, "brute force") == 0)
    {
        printf("[AH]: Using brute force search algorithm.\n");
        sprintf(harmonyTclFile, "hconfig.brute.tcl");
    }
    else
    {
        printf("[AH]: Error: Invalid search algorithm '%s'\n", search_algo);
        printf("[AH]: Valid entries are:\n"
               "[AH]:\tPRO\n"
               "[AH]:\tNelder Mead\n"
               "[AH]:\tRandom\n"
               "[AH]:\tBrute Force\n");
        return -1;
    }
    return 0;
}

/***
 * This function is called each time a operation requested by the client fails
 * It will return a FAIL message to the client
 ***/
void operation_failed(int sock)
{
    HRegistMessage m(HMESG_FAIL, 0);
    send_message(&m, sock);
}

/***
 *
 * Start the server by initializing sockets and everything else that is needed
 *
 ***/
int server_startup()
{
    /* used address */
    struct sockaddr_in sin;
    char *codeserver_type;
    int err, optval;

    /* try to get the port info from the environment */
    if (getenv("HARMONY_S_PORT") != NULL)
    {
        listen_port = atoi(getenv("HARMONY_S_PORT"));
    }
    else
    {
        /* if the env var is not defined, use the default */
        listen_port = atoi(cfg_get("harmony_port"));
    }

    /* create a listening socket */
    if ( (listen_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        h_exit("Error initializing socket!");
    }

    /* set socket options. We try to make the port reusable and have it
       close as fast as possible without waiting in unnecessary wait states
       on close. */
    optval = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR,
                   &optval, sizeof(optval)))
    {
        h_exit("Error setting socket options!");
    }

    /* Initialize the socket address. */
    sin.sin_family = AF_INET;
#ifdef LINUX
    sin.sin_addr = listen_addr;
#else
    unsigned int listen_addr = INADDR_ANY;
    sin.sin_addr = *(struct in_addr *)&listen_addr;
#endif  /*LINUX*/
    sin.sin_port = htons(listen_port);

    /* Bind the socket to the desired port. */
    if (bind(listen_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        h_exit("Error binding socket! Please check if there are previous\n"
               "instances of hserver still running.");
    }

    /* Now we have to initialize the Tcl interpreter */
    tcl_inter = Tcl_CreateInterp();

    if ( (err = Tcl_Init(tcl_inter)) != TCL_OK)
    {
        h_exit("Tcl Init failed!");
    }

    if ( (err = Tcl_EvalFile(tcl_inter, harmonyTclFile)) != TCL_OK)
    {
        printf("[AH]: Tcl Error %d; %s [%s]\n", err,
               tcl_inter->result, harmonyTclFile);
        h_exit("Tcl Interpreter Error ");
    }

    /*
     * Set the file descriptor set
     */
    FD_ZERO(&listen_set);
    FD_SET(listen_socket, &listen_set);
    highest_socket = listen_socket;

    /* Establish connection to the code server, if requested. */
    codeserver_type = cfg_getlower("codegen");
    if (codeserver_type == NULL || strcmp(codeserver_type, "none") == 0)
    {
        /* No code server requested. */
    }
    else if (strcmp(codeserver_type, "standard") == 0)
    {
        /* TCP-based code server requested. */
        assert(0 && "TCP-based code server not yet tested/implemented.");
    }
    else if (strcmp(codeserver_type, "standalone") == 0)
    {
        /* File/SSH based code server requested. */
        if (codegen_init() < 0)
        {
            return -1;
        }
    }

    /* Initialize the HTTP user interface. */
    return http_init();
}

/***
 *
 * Client registration adds the client information into the relevant maps
 *
 ***/
void client_registration(HRegistMessage *m, int client_socket)
{
    if (debug_mode)
        printf("[AH]: Client registration! (%d)\n", client_socket);

    /* store information about the client into the local database */
    clientSignals[client_socket] = m->get_param();
    int id = m->get_id();

    if (id)
    {
        /* the client is relocating its communicating agent */
        /* the server dissacociates the old socket with the client, and
         * uses the new one instead */
        printf("[AH]: Client id %d relocated\n", id);

        int cs = clientSocket[id];
        clientSocket[id] = client_socket;

        char *appName = clientName[cs];
        clientInfo[appName] = client_socket;
        clientName.erase(cs);
        clientName[client_socket] = appName;

        /* This code sequence is problematic.  It might disrupt the loop
         * inside main_loop() that iterates over harmony_fds.
         * Find a way to move it into main_loop().
         */
        FD_CLR(cs, &listen_set);
        if (cs == *socketIterator)
            ++socketIterator;
        harmony_fds.remove(cs);
        if (highest_socket == cs)
            --highest_socket;
    }
    else
    {
        /* generate id for the new client */
        id = next_id++;
        clientSocket[id] = client_socket;
    }

    clientId[client_socket] = id;

    if (debug_mode)
        printf("[AH]: Sending registration (id %d) confirmation\n", id);

    /* send the confirmation message with the client_id */
    /* ACK */
    HRegistMessage *mesg = new HRegistMessage(HMESG_CLIENT_REG, client_socket);
    mesg->set_id(id);
    send_message(mesg, client_socket);
}

/***
 *
 * Retrieves and parses the application description from the client
 *
 ***/
void get_appl_description(HDescrMessage *mesg, int client_socket)
{
    int err;
    int buflen;
    char *tclbuf, *endp;

    buflen = mesg->get_descrlen();
    tclbuf = (char *)malloc( sizeof(char) * (buflen + 13) );
    memcpy(tclbuf, mesg->get_descr(), buflen);
    tclbuf[buflen] = '\0';

    endp = strrchr(tclbuf, '}');
    if (endp++ == NULL)
        endp = tclbuf + buflen;

    sprintf(endp, " %d", clientId[client_socket]);

    if (debug_mode)
        printf("[AH]: Message size: %d\n", mesg->get_message_size());

    if ( (err = Tcl_Eval(tcl_inter, tclbuf)) != TCL_OK)
    {
        printf("[AH]: Error %d interpreting the Tcl description: %s\n",
               err, tcl_inter->result);
        printf("[AH]: App description: [%s]\n", tclbuf);
        free(tclbuf);
        operation_failed(client_socket);
    }
    else
    {
        /* send confirmation to client */
        HRegistMessage m(HMESG_CONFIRM, 0);
        send_message(&m, client_socket);

        /* add the information about the client in the clientInfo map
         * first just get the name of the application, so that we can
         * link it with the socket number.
         */
        const char *appName = tcl_inter->result;
        unsigned int cliNameLen = strlen(tcl_inter->result) + 13;

        if (debug_mode)
            printf("[AH]: Application name: %s\n", appName);

        code_timesteps[appName] = 1;
        last_code_req_timesteps[appName] = 0;
        last_code_req_responses[appName] = 0;

        char *cliName = (char *)malloc( cliNameLen );
        snprintf(cliName, cliNameLen, "%s_%d",
                 appName, clientId[client_socket]);

        if (debug_mode)
            printf("[AH]: Client name: %s\n", cliName);

        /* add info to the map */
        clientInfo[cliName] = client_socket;
        clientName[client_socket] = cliName;
    }
}

/***
 *
 * Registers the variables from the client into the database
 *
 ***/
void variable_registration(HDescrMessage *mesg, int client_socket)
{
    char *cliName = clientName[client_socket];
    char *retval = NULL, *bufptr;
    char tclvar[256];
    snprintf(tclvar, sizeof(tclvar), "%s_bundle_%s(value)",
             cliName, mesg->get_descr());

    if (debug_mode)
        printf("[AH]: Tcl string: %s\n", tclvar);

    if ((retval = Tcl_GetVar(tcl_inter, tclvar, 0)) == NULL)
    {
        operation_failed(client_socket);
        return;
    }

    if (debug_mode)
        printf("[AH]: Tcl value: %s\n", retval);

    /* Test to see if the result string can be converted to an integer */
    strtol(retval, &bufptr, 10);

    HUpdateMessage m(HMESG_VAR_REQ, 0);
    if (strlen(bufptr) == 0)
    {
        /* is int */
        VarDef v(mesg->get_descr(), VAR_INT);
        v.setValue(atoi(retval));
        m.set_var(v);
    }
    else
    {
        VarDef v(mesg->get_descr(), VAR_STR);
        v.setValue(retval);
        m.set_var(v);
    }

    send_message(&m, client_socket);
}

/***
 *
 * Return the value of the variable from the database
 *
 ***/
void variable_request(HUpdateMessage *mesg, int client_socket)
{
    char *cliName = clientName[client_socket];
    char *retval;
    char tclbuf[256];
    char curr_config[80], next_config[80];
    int err;

    /* construct the key using get_test_configuration (look at the database
     * function) if the conf is in the database, either change the message
     * type or construct a brand new message object. (set its type to
     * already_evaluated)... otherwise, do the usual
     */

    snprintf(tclbuf, sizeof(tclbuf), "get_test_configuration %s", cliName);
    if ((err = Tcl_Eval(tcl_inter, tclbuf)) != TCL_OK)
    {
        fprintf(stderr, "Error %d retrieving current configuration: %s\n",
                err, tcl_inter->result);
        operation_failed(client_socket);
        return;
    }
    else
    {
        char *endp = cliName;
        while (*endp && *endp != '_') ++endp;
        snprintf(curr_config, sizeof(curr_config), "%.*s_%s",
                 endp - cliName, cliName, tcl_inter->result);
        printf("Checking the map for %s\n", curr_config);
    }

    /* DATABASE CHECK */

    /* check the database whether "current_config" is already available */
    string str_config = curr_config;
    it = global_data.find(str_config);

    /* this is the case where the performance is not in the database.
     * if the iterator is at the end of the map, we never found the conf
     * in the database.  so simply use the old code (from the point after
     * setting the values of the variables).
     */

    if (it == global_data.end())
    {
        /* The database search did not find the configuration */
        printf("The following configuration has not seen before\n");

        /* step 1: updating the values for the tunable parameters.
         * get the values from the Tcl backend using Tcl_GetVar and set the
         * value of the variables in the message to whatever value received
         * from the Tcl backend.
         */
        for (int i = 0; i < mesg->get_nr_vars(); ++i)
        {
            /* get the name of the parameter. e.g. x or y ... */

            /* constructing the Tcl variable name: the way variables are
             * represented in the backend is as follows:
             * <appname>_<client socket>_bundle_<name of the variable>
             * for c++ example:
             *    appname is SimplexT
             *    name of the variables is x and y.
             *    if you are running 4 clients: client socket goes from 1 to 4.
             *    An example of the variable name in the backend will be:
             *    SimplexT_1_bundle_x
             * Now in the Tcl backend, this variable has attributes:
             *  SimplexT_1_bundle_x(value)
             */
            snprintf(tclbuf, sizeof(tclbuf), "%s_bundle_%s(value)", cliName,
                     mesg->get_var(i)->getName());

            if (debug_mode)
                printf("[AH]: Tcl value string: %s\n", tclbuf);

            /* invoking the Tcl interpreter to parse this Tcl command. */
            retval = Tcl_GetVar(tcl_inter, tclbuf, 0);

            if (debug_mode)
                printf("[AH]: Tcl value: %s\n", retval);

            if (retval == NULL)
            {
                printf("[AH]: Error retrieving Tcl variable %s: %s\n",
                       tclbuf, tcl_inter->result);
                operation_failed(client_socket);
                return;
            }

            /* get the VarDef object at position i in the message
             * and check its type.
             */
            switch (mesg->get_var(i)->getType()) 
            {
            case VAR_INT:
                /* is int */
                /* update the value */
                mesg->get_var(i)->setValue(atoi(retval));
                break;
            case VAR_STR:
                mesg->get_var(i)->setValue(retval);
                break;
            }
        }

        snprintf(tclbuf, sizeof(tclbuf), "%s_simplex_time", cliName);

        if (debug_mode)
            printf("[AH]: Timestamp Tcl string: %s\n", tclbuf);

        retval = Tcl_GetVar(tcl_inter, tclbuf, 0);

        if (debug_mode)
            printf("[AH]: Timestamp app: %s\n", retval);

        if (retval == NULL)
        {
            printf("[AH]: Error retrieving Tcl variable %s: %s\n",
                   tclbuf, tcl_inter->result);
            operation_failed(client_socket);
            return;
        }

        if (debug_mode)
            printf("[AH]: Timestamp value from the server: %s\n", retval);

        mesg->set_timestamp(strtol(retval, NULL, 10));

        send_message(mesg, client_socket);
        printf("Server is sending the message of type: %d\n",
               mesg->get_type());

        delete mesg;
    }
    else
    {
        /* the configuration->performance mapping exists in the database */
        printf("The following configuration has seen before\n"
               "Server gets the unique configuration from the backend\n");

        snprintf(tclbuf, sizeof(tclbuf), "get_next_configuration %s", cliName);
        if ((err = Tcl_Eval(tcl_inter, tclbuf)) != TCL_OK)
        {
            fprintf(stderr, "Error %d retrieving next configuration: %s\n",
                    err, tcl_inter->result);
            operation_failed(client_socket);
            return;
        }
        else
        {
            printf("The backend says the value is: %s\n", tcl_inter->result);
            char *endptr = cliName;
            while (*endptr && *endptr != '_') ++endptr;
            snprintf(next_config, sizeof(next_config), "%.*s_%s",
                     endptr - cliName, cliName, tcl_inter->result);
            printf("Checking the map: %s\n", next_config);
        }

        for (int i = 0; i < mesg->get_nr_vars(); ++i)
        {
            /* Maintain the list of new configs associated with cliName */
            appName_confs.insert(pair<string, string>(cliName, next_config));

            snprintf(tclbuf, sizeof(tclbuf), "%s_bundle_%s(value)",
                     cliName, mesg->get_var(i)->getName());

            if (debug_mode)
                printf("Tcl variable string: %s\n", tclbuf);

            /* invoke the Tcl interpreter to parse this command */
            retval = Tcl_GetVar(tcl_inter, tclbuf, 0);

            if (debug_mode)
                printf("Tcl value: %s\n", retval);

            if (retval == NULL)
            {
                printf("Error getting the value of variable!\n");
                operation_failed(client_socket);
                return;
            }

            /* get the VarDef object at position i in the message and
             * check its type
             */
            switch (mesg->get_var(i)->getType())
            {
            case VAR_INT:
                /* is int */
                /* update the value */
                mesg->get_var(i)->setValue(atoi(retval));
                break;
            case VAR_STR:
                mesg->get_var(i)->setValue(retval);
                break;
            }
        }

        it2 = appName_confs.find(next_config);
        if (it2 != it)
        {
            /* The search concluded and did not find the configuration */
            snprintf(tclbuf, sizeof(tclbuf), "%s_simplex_time", cliName);

            if (debug_mode)
                printf("Timestamp Tcl string: %s\n", tclbuf);

            retval = Tcl_GetVar(tcl_inter, tclbuf, 0);

            if (debug_mode)
                printf("Timestamp app: %s\n", retval);

            if (retval == NULL)
            {
                printf("Error getting value of variable %s: %s\n",
                       tclbuf, tcl_inter->result);
                operation_failed(client_socket);
                return;
            }

            if (debug_mode)
                printf("Timestamp value from the server: %s\n", retval);

            mesg->set_timestamp(strtol(retval, NULL, 10));
            send_message(mesg, client_socket);

            delete mesg;
        }
        else
        {
            /* Extracting the performance from the database
             * this performance has to come from the map
             */
            string updated_performance;
            int updt_perf;

            /* getting the perf from the map */
            updated_performance = (*it).second;

            /* This converts perf(string) to perf(int) */
            updt_perf = atoi(updated_performance.c_str());
            printf("Performance from the database: %d \n", updt_perf);

            snprintf(tclbuf, sizeof(tclbuf), "%s_simplex_time", cliName);

            if (debug_mode)
                printf("Timestamp Tcl string: %s\n", tclbuf);

            retval = Tcl_GetVar(tcl_inter, tclbuf, 0);

            if (debug_mode)
                printf("Timestamp app: %s\n", retval);

            if (retval == NULL)
            {
                printf("Error getting value of variable %s: %s\n",
                       tclbuf, tcl_inter->result);
                operation_failed(client_socket);
                return;
            }

            if (debug_mode)
                printf("Timestamp value from the server: %s\n", retval);

            mesg->set_timestamp(strtol(retval, NULL, 10));
            send_message(mesg, client_socket);
            delete mesg;
        }
    }
}

/*****
 *
 * Return the value of the variable from the Tcl backend. Use this
 * carefully. You have to make sure the variable is defined in the
 * backend.
 *
 ***/
void var_tcl_variable_request(HUpdateMessage *mesg, int client_socket)
{
    char *cliName = clientName[client_socket];
    char tclvar[256];
    char *retval;

    if (debug_mode)
        printf("[AH]: received a Tcl variable request\n");

    for (int i = 0; i < mesg->get_nr_vars(); ++i)
    {
        snprintf(tclvar, sizeof(tclvar), "%s_%s", cliName,
                 mesg->get_var(i)->getName());

	if (debug_mode)
            printf("[AH]: Tcl string: %s\n", tclvar);

        retval = Tcl_GetVar(tcl_inter, tclvar, 0);

        if (debug_mode)
            printf("[AH]: Tcl backend variable: %s\n", retval);

        if (retval == NULL)
        {
            printf("[AH]: Error getting value of Tcl backend variable!\n");
            operation_failed(client_socket);
            return;
        }

        switch (mesg->get_var(i)->getType())
        {
        case VAR_INT:
            mesg->get_var(i)->setValue(atoi(retval));
            break;
        case VAR_STR:
            mesg->get_var(i)->setValue(retval);
            break;
        }
    }
    send_message(mesg, client_socket);
    delete mesg;
}

/* check if a file exists in the file system */
int file_exists(const char *fileName)
{
    struct stat buffer;
    int i = stat( fileName, &buffer );
    if (i == 0)
    {
        return 1;
    }
    return 0;
}

/*
 * Send a query to the code-server to see if code generation for current
 * search iteration is done.
 *
 * 1. The first method utilizes the code-server. The connection to the
 *    code-server is maintained in the Tcl backend.
 * 2. The second method uses the file-system and checks for
 *    code.complete.<timestep> file to determine if the new code is ready.
 *    This method is useful when the machine that is running Active Harmony
 *    does not allow tcp connections to remote machines.
 */

/*
 * Check the code_complete.<timestep> file to see if the code generation is
 * complete. This is used with the standalone version of the code generator.
 */
void check_code_completion(HUpdateMessage *mesg, int client_socket)
{
    char *cliName = clientName[client_socket];
    char appName[256], filename[256];
    int retval = 0;

    char *endp = cliName;
    while (*endp && *endp != '_') ++endp;
    snprintf(appName, sizeof(appName), "%.*s", endp - cliName, cliName);

    /* retrieve the timestep */
    int req_timestep = *(int *)mesg->get_var(0)->getPointer();
    int code_timestep = code_timesteps[appName];
    int last_code_req_timestep = last_code_req_timesteps[appName];
    int last_code_req_response = last_code_req_responses[appName];

    if (debug_mode)
        printf("[AH]: Code Request Timestep: %d last_code_req_timestep %d "
               "code_timestep: %d\n", req_timestep, last_code_req_timestep,
               code_timestep);

    if (req_timestep > last_code_req_timestep)
    {
        /* based on file system right now. */
        snprintf(filename, sizeof(filename), "%s/code_complete.%s.%d",
                flag_dir, appName, code_timestep);

	if (debug_mode)
            printf("[AH]: Looking for %s\n", filename);

        if (file_exists(filename))
        {
            if (debug_mode)
                printf("[AH]: code generation is complete for timestep: %d\n",
                       code_timestep);

            code_timesteps[cliName] = code_timestep + 1;
            retval = 1;
            last_code_req_response = retval;
            last_code_req_responses[cliName] = retval;
            std::remove(filename);
            snprintf(filename, sizeof(filename), "%s/code_complete.%s.%d",
                    flag_dir, appName, code_timestep + 1);
            printf("[AH]: removing: %s\n", filename);
            std::remove(filename);
        }
        else
        {
            last_code_req_responses[cliName] = 0;
        }
        last_code_req_timesteps[cliName] = req_timestep;
    }
    else
    {
        retval = last_code_req_responses[cliName];
    }

    mesg->get_var(0)->setValue(retval);
    send_message(mesg, client_socket);
    delete mesg;
}

/*
 * Check the global database to see if we have already evaluated a 
 * configuration that we handed to a given client.
 */
void database(HUpdateMessage *mesg, int client_socket)
{
    /*
     * Make sure all pro instances have reported their performance value to
     * the server and that the "next_iteration" flag is set to 1 before
     * calling this function.
     */
    char *cliName = clientName[client_socket];
    char curr_config[80], buf[80];
    int err;

    /* Tcl proc get_test_configuration appName returns the current
     * configuration being tested
     */
    snprintf(buf, sizeof(buf), "get_test_configuration %s", cliName);
    if ((err = Tcl_Eval(tcl_inter, buf)) != TCL_OK)
    {
        fprintf(stderr, "Error %d retrieving current configuration: %s\n",
                err, tcl_inter->result);
        operation_failed(client_socket);
        return;
    }
    else
    {
        char *endp = cliName;
        while (*endp && *endp!='_') ++endp;
        snprintf(curr_config, sizeof(curr_config), "%.*s_%s",
                 endp - cliName, cliName, tcl_inter->result);
    }

    string str_conf = curr_config;
    it = global_data.find(str_conf);
    if (it == global_data.end())
    {
        snprintf(buf, sizeof(buf), "%d", INTMAXVAL);
    }
    else
    {
        strncpy(buf, (*it).second.c_str(), sizeof(buf));
    }

    switch (mesg->get_var(0)->getType())
    {
    case VAR_INT:
        mesg->get_var(0)->setValue(atoi(buf));
        break;
    case VAR_STR:
        mesg->get_var(0)->setValue(buf);
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
void variable_set(HUpdateMessage *mesg, int client_socket)
{
    char *cliName = clientName[client_socket];
    char tclvar[256], buf[80];
    char *value;
    char *retval;

    for (int i = 0; i < mesg->get_nr_vars(); ++i)
    {
        switch (mesg->get_var(i)->getType())
        {
        case VAR_INT:
            snprintf(buf, sizeof(buf), "%d",
                     *(int *)mesg->get_var(i)->getPointer());
            value = buf;
            break;
        case VAR_STR:
            value = (char *)mesg->get_var(i)->getPointer();
            break;
        }

        snprintf(tclvar, sizeof(tclvar), "%s_bundle_%s",
                 cliName, mesg->get_var(i)->getName());

        retval = Tcl_SetVar2(tcl_inter, tclvar, "value", value,
                             TCL_GLOBAL_ONLY);
        if (retval == NULL) {
            fprintf(stderr, "Error setting variable %s(value): %s\n",
                    tclvar, tcl_inter->result);
            operation_failed(client_socket);
            return;
        }

#ifdef SCHEDULER
#ifdef notdef
        char *c;
        c = strchr(ss, '_');
        assert(c);
        *c = '\0';
        char *cliName = ss;
        c += 8;
        char *vname = c;
#endif  /*notdef*/

        char StartEvent[] = "START";
        char EndEvent[] = "END";
        char SyncEvent[] = "SYNC";
        char VarEvent[] = "VAR";
        char *event;

        if (strcmp(mesg->get_var(i)->getName(), "AdaState") == 0)
        {
            if (!strcmp(s, "APP_STARTED"))
            {
                /* register application in scheduler */
                event = StartEvent;
            }
            else
            {
                /* call scheduler to take over */
                event = SyncEvent;
            }
        }
        else
        {
            event = VarEvent;
        }

        application_trigger = cliName;
        update_sent = FALSE;
        if (Tcl_VarEval(tcl_inter, "SchedulerEventHandler ",
                        cliName, " ", event, NULL) != TCL_OK)
        {
            fprintf(stderr, "SchedulerEventHandler failed: %s\n",
                    tcl_inter->result);
            operation_failed(client_socket);
        }

#elif SCHEDULER2
        char *c;
        c = strchr(ss, '_');
        assert(c);
        *c = '\0';
        cliName = ss;
        c += 8;
        char *vname = c;

        if (strcmp(vname, "AdaState") == 0)
        {
            if (!socket_to_app(sock))
            {
                /* register application in scheduler */
                app_start(cliName, client_socket);
            }
            else
            {
                /* call scheduler to take over */
                scheduler.app_sync(client_socket);
            }
        }
        else
        {
            scheduler.app_var(client_socket);
        }

        free(sname);
#endif
    }

    if (!update_sent)
    {
        HRegistMessage m(HMESG_CONFIRM, 0);
        send_message(&m, client_socket);
    }
    update_sent = FALSE;
}

/****
     Name:         revoke_resource
     Description:  calls scheduler to revoke allocated resources,
     remove Tcl names related to the disrupted application, and
     start a new scheduling round
     Called from:  cilent_unregister
****/

void revoke_resources(char *appName)
{
    if (Tcl_VarEval(tcl_inter, "SchedulerEventHandler ",
                    appName, " END", NULL) != TCL_OK)
    {
        fprintf(stderr, "revoke_resources: ERROR in unsetting related"
                " Tcl vars: %s\n", tcl_inter->result);
    }
    free(clientName[clientInfo[appName]]);
}

void filter(map<string,string> &m, vector<string> &result,
            vector<string> &values, string &condition)
{
    map<string,string>::iterator iter;
    for (iter = m.begin(); iter != m.end(); ++iter)
    {
        string key = iter->first; 
        size_t found = key.find(condition);
        if (found != string::npos) 
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
void client_unregister(HRegistMessage *m, int client_socket)
{
    int retval;
    char *cliName = clientName[client_socket];
    char appName[128], filename[128];

    /* configurations and corresponding performance values */
    vector<string> configurations;
    vector<string> corresponding_values;

    /* clear all the information related to the client */
    if (debug_mode)
        printf("[AH]: ENTERED CLIENT UNREGISTER!!!!!\n");
    
    /* derive the global appName */
    char *endp = cliName;
    while (*endp && *endp != '_') ++endp;
    snprintf(appName, sizeof(appName), "%.*s", endp - cliName, cliName);

    /* unique_points filename */
    snprintf(filename, sizeof(filename), "unique_points.%s.dat", appName);

    /* filter the keys by global appName */
    string filter_text = appName;

    /* get the unique configurations for this application */
    filter(global_data, configurations, corresponding_values, filter_text);

    FILE *pFile = fopen(filename, "a");
    if (pFile == NULL)
    {
        fprintf(stderr, "Could not open %s for writing: %s\n",
                filename, strerror(errno));
    }
    else
    {
        fprintf(pFile, "-----------------\n");
        for (unsigned index = 0; index < configurations.size(); ++index)
        {
            fprintf(pFile, (configurations[index]).c_str());
            fprintf(pFile, " ==> ");
            fprintf(pFile, (corresponding_values[index]).c_str());
            fprintf(pFile, " \n");
        }
        fclose(pFile);
    }
    
    /* run the Tcl function to clear after the client */
    retval = Tcl_VarEval(tcl_inter, "harmony_cleanup ",
                         clientName[client_socket], NULL);
    if (retval != TCL_OK)
    {
        fprintf(stderr, "Error cleaning up after client: %s\n",
                tcl_inter->result);
    }

    int stashed_id = clientId[client_socket];
    clientSocket.erase(clientId[client_socket]);
    clientId.erase(client_socket);

    if (m == NULL)
    {
        /* revoke resources */
        revoke_resources(clientName[client_socket]);
    }
    else
    {
        HRegistMessage mesg(HMESG_CONFIRM, 0);
        
        /* send back the received message */
        send_message(&mesg, client_socket);
    }

    if (close(client_socket) < 0)
    {
        printf("Error closing harmony client %d socket %d\n",
               stashed_id, client_socket);
    }
}

void performance_update_int(HUpdateMessage *m, int client_socket)
{
    char *cliName = clientName[client_socket];
    char buf[256], curr_config[80], perf_string[80];
    int performance = INTMAXVAL;
    int err;

    /* get the current conf for this instance from the Tcl backend. */
    performance = *(int *)m->get_var(0)->getPointer();

    if (performance != INTMAXVAL)
    {
        snprintf(buf, sizeof(buf), "get_test_configuration %s", cliName);

        if ((err = Tcl_Eval(tcl_inter, buf)) != TCL_OK)
        {
            fprintf(stderr, "Error %d retrieving current configuration: %s\n",
                    err, tcl_inter->result);
            operation_failed(client_socket);
            return;
        }
        else
        {
            char *endp = cliName;
            while (*endp && *endp != '_') ++endp;
            snprintf(curr_config, sizeof(curr_config), "%.*s_%s",
                     endp - cliName, cliName, tcl_inter->result);
        }

        snprintf(perf_string, sizeof(perf_string), "%d", performance);
        global_data.insert(pair<string, string>(string(curr_config),
                                                string(perf_string)));

        snprintf(buf, sizeof(buf), "get_http_test_coord %s", cliName);
        if ((err = Tcl_Eval(tcl_inter, buf)) != TCL_OK) {
            fprintf(stderr, "get_http_test_coord() error %d: %s\n",
                    err, tcl_inter->result);
            operation_failed(client_socket);
            return;
        }
        snprintf(buf, sizeof(buf), "%u000,%s,%s",
                 (unsigned int)time(NULL), tcl_inter->result, perf_string);
        coord_history.push_back(buf); 
   }

    /* let the search backend know what we have received the performance
     * number from this client.
     */
    snprintf(buf, sizeof(buf), "updateObsGoodness %s %d %ld",
             cliName, performance, m->get_timestamp());

    if (debug_mode)
        printf("[AH]: goodness Tcl string: %s\n", buf);

    if ((err = Tcl_Eval(tcl_inter, buf)) != TCL_OK)
    {
        fprintf(stderr, "Error %d setting performance function: %s\n",
                err, tcl_inter->result);
        operation_failed(client_socket);
        return;
    }
    send_message(m, client_socket);
}

/* performance update function */
void performance_update_double(HUpdateMessage *m, int client_socket)
{
    char *cliName = clientName[client_socket];
    char buf[256], curr_config[80], perf_dbl[80];
    int err;
    double performance = *(double *)(m->get_var(0)->getPointer());

    snprintf(perf_dbl, sizeof(perf_dbl), "%.17lg", performance);

    /* get the current conf for this instance from the Tcl backend. */
    if (performance != FLTMAXVAL)
    {
        snprintf(buf, sizeof(buf), "get_test_configuration %s", cliName);

        if ((err = Tcl_Eval(tcl_inter, buf)) != TCL_OK)
        {
            fprintf(stderr, "Error %d retrieving current configuration: %s\n",
                    err, tcl_inter->result);
            operation_failed(client_socket);
            return;
        }
        else
        {
            char *endp = cliName;
            while (*endp && *endp != '_') ++endp;
            snprintf(curr_config, sizeof(curr_config), "%.*s_%s",
                     endp - cliName, cliName, tcl_inter->result);
        }

        global_data.insert(pair<string, string>(string(curr_config),
                                                string(perf_dbl)));

        snprintf(buf, sizeof(buf), "get_http_test_coord %s", cliName);
        if ((err = Tcl_Eval(tcl_inter, buf)) != TCL_OK)
        {
            fprintf(stderr, "get_http_test_coord() error %d: %s\n",
                    err, tcl_inter->result);
            operation_failed(client_socket);
            return;
        }
        snprintf(buf, sizeof(buf), "%u000,%s,%s",
                 (unsigned int)time(NULL), tcl_inter->result, perf_dbl);
        coord_history.push_back(buf);
    }

    snprintf(buf, sizeof(buf), "updateObsGoodness %s %.17lg %ld",
             cliName, performance, m->get_timestamp());

    if (debug_mode)
        printf("[AH]: goodness Tcl string: %s\n", buf);

    if ((err = Tcl_Eval(tcl_inter, buf)) != TCL_OK)
    {
        fprintf(stderr, "Error %d setting performance function: %s\n",
                err, tcl_inter->result);
        operation_failed(client_socket);
        return;
    }

    send_message(m, client_socket);
}


/* performance update function */
void performance_update(HUpdateMessage *m, int client_socket)
{
    switch (m->get_var(0)->getType())
    {
    case VAR_INT:
        performance_update_int(m,client_socket);
        break;
    case VAR_REAL:
        performance_update_double(m,client_socket);
        break;
    }
}

/* Server queries the database to know whether the performance is
 * already evaluated or not
 */
void performance_already_evaluated_int(HUpdateMessage *m, int client_socket)
{
    char *cliName = clientName[client_socket];
    char buf[256], curr_config[80], str_conf[80];
    int err;
    int performance = INTMAXVAL;
    char perf_string[80];

    /* get the current conf for this instance from the Tcl backend. */
    performance = *(int *)m->get_var(0)->getPointer();

    if (performance != INTMAXVAL)
    {
        snprintf(buf, sizeof(buf), "get_test_configuration %s", cliName);

        if ((err = Tcl_Eval(tcl_inter, buf)) != TCL_OK) 
        {
            fprintf(stderr, "Error %d retrieving current configuration: %s\n",
                    err, tcl_inter->result);
            operation_failed(client_socket);
            return;
        }
        else
        {
            char *endp = cliName;
            while (*endp && *endp != '_') ++endp;
            snprintf(curr_config, sizeof(curr_config), "%.*s_%s",
                     endp - cliName, cliName, tcl_inter->result);
        }

        sprintf(perf_string, "%d", performance);
        global_data.insert(pair<string, string>(string(curr_config),
                                                string(perf_string)));

        snprintf(buf, sizeof(buf), "get_http_test_coord %s", cliName);
        if ((err = Tcl_Eval(tcl_inter, buf)) != TCL_OK)
        {
            fprintf(stderr, "get_http_test_coord() error %d: (%s)\n",
                    err, tcl_inter->result);
            operation_failed(client_socket);
            return;
        }
        snprintf(buf, sizeof(buf), "%u000,%s,%s",
                 (unsigned int)time(NULL), tcl_inter->result, perf_string);
        coord_history.push_back(buf);
    }

    /* Server checks the database whether the conf has already
     * been evaluated in the past
     */
    it = global_data.find(str_conf);

    /* searching the database */
    for (it = global_data.begin(); it != global_data.end(); ++it)
    {
        if (!str_conf)
        {
            performance_update_int(m, client_socket);

            if (debug_mode)
                printf("goodness Tcl string: %s\n", buf);

            if ((err = Tcl_Eval(tcl_inter, buf)) != TCL_OK)
            {
                fprintf(stderr, "Error %d setting performance function: %s\n",
                        err, tcl_inter->result);
                operation_failed(client_socket);
                return;
            }
            send_message(m, client_socket);
        }
        else
        {
            printf("The following configuration has already seen\n");

            /* For now using the update function again */
            performance_update_int(m, client_socket);

            if (debug_mode)
                printf("goodness Tcl string: %s\n", buf);

            if ((err = Tcl_Eval(tcl_inter, buf)) != TCL_OK)
            {
                fprintf(stderr, "Error %d setting performance function: %s\n",
                        err, tcl_inter->result);
                operation_failed(client_socket);
                return;
            }
            send_message(m, client_socket);
        }

        send_message(m, client_socket);
    }
}


void performance_already_evaluated_double(HUpdateMessage *m, int client_socket)
{
    char *cliName = clientName[client_socket];
    char buf[256], curr_config[80], str_conf[80];
    int err;
    double performance = *(double *)m->get_var(0)->getPointer();
    char perf_dbl[80];
  
    /* get the current conf for this instance from the Tcl backend */
    snprintf(perf_dbl, sizeof(perf_dbl), "%.17lg", performance);

    if (performance != FLTMAXVAL)
    {
        snprintf(buf, sizeof(buf), "get_test_configuration %s", cliName);

        if ((err = Tcl_Eval(tcl_inter, buf)) != TCL_OK)
        {
            fprintf(stderr, "Error %d retrieving current configuration: %s\n",
                    err, tcl_inter->result);
            operation_failed(client_socket);
            return;
        }
        else
        {
            char *endp = cliName;
            while (*endp && *endp != '_') ++endp;
            snprintf(curr_config, sizeof(curr_config), "%.*s_%s",
                     endp - cliName, cliName, tcl_inter->result);
        }

        global_data.insert(pair<string, string>(string(str_conf),
                                                string(perf_dbl)));

        snprintf(buf, sizeof(buf), "get_http_test_coord %s", cliName);
        if ((err = Tcl_Eval(tcl_inter, buf)) != TCL_OK) {
            fprintf(stderr, "get_http_test_coord() error %d: %s\n",
                    err, tcl_inter->result);
            operation_failed(client_socket);
            return;
        }
        snprintf(buf, sizeof(buf), "%u000,%s,%s",
                 (unsigned int)time(NULL), tcl_inter->result, perf_dbl);
        coord_history.push_back(buf);
    }

    /* Server checks the database whether the conf has already been
     * evaluated in the past
     */
    it = global_data.find(str_conf);

    /* searching the database */
    for (it = global_data.begin(); it != global_data.end(); ++it)
    {
        if (!str_conf)
        {
            printf("The following configuration has not seen before\n"
                   "Updating the performance and sending the configuration"
                   " to the client\n");

            performance_update_int(m, client_socket);

            if (debug_mode)
                printf("goodness Tcl string: %s\n", buf);

            if ((err = Tcl_Eval(tcl_inter, buf)) != TCL_OK)
            {
                fprintf(stderr, "Error %d setting performance function: %s\n",
                        err, tcl_inter->result);
                operation_failed(client_socket);
                return;
            }
            send_message(m,client_socket);
        }
        else
        {
            printf("The following configuration has already seen\n");

            /* For now using the update function again */
            performance_update_int(m, client_socket);

            if (debug_mode)
                printf("goodness Tcl string: %s\n", buf);

            if ((err = Tcl_Eval(tcl_inter, buf)) != TCL_OK)
            {
                fprintf(stderr, "Error %d setting performance function: %s\n",
                        err, tcl_inter->result);
                operation_failed(client_socket);
                return;
            }
            send_message(m, client_socket);
        }
        send_message(m,client_socket);
    }
}

/* performance already evaluated function */
void performance_already_evaluated(HUpdateMessage *m, int client_socket)
{
    switch (m->get_var(0)->getType())
    {
    case VAR_INT:
        performance_already_evaluated_int(m, client_socket);
        break;
    case VAR_STR:
        performance_already_evaluated_double(m, client_socket);
        break;
    }
}

/* not used anymore... but has some use for debugging, so keeping it intact */
void performance_update_with_conf(HUpdateMessage *m, int client_socket)
{
    char *cliName = clientName[client_socket];
    char tclbuf[512];
    int err;
    char *conf = (char *)m->get_var(0)->getName();

    printf("Client Name: %s\n\n", cliName);
    printf("configuration: %s\n", conf);

    snprintf(tclbuf, sizeof(tclbuf), "updateObsGoodness %s %d %ld",
             cliName, *(int *)m->get_var(0)->getPointer(), m->get_timestamp());

    printf("perf_update_str: (%s)\n", tclbuf);

    if ((err = Tcl_Eval(tcl_inter, tclbuf)) != TCL_OK)
    {
        fprintf(stderr, "Error %d setting performance function: %s\n",
                err, tcl_inter->result);
        operation_failed(client_socket);
        return;
    }
    send_message(m, client_socket);
}

/****
 *
 * Return values from the global_config file
 *
 ****/
void send_cfg_message(HDescrMessage *mesg, int client_socket)
{
    char *cfg_val;
    HDescrMessage reply;

    cfg_val = cfg_get(mesg->get_descr());
    if (cfg_val == NULL) {
        reply.set_type(HMESG_FAIL);

    } else {
        reply.set_type(HMESG_CFG_REQ);
        reply.set_descr(cfg_val, strlen(cfg_val), 0);
    }
    send_message(&reply, client_socket);
}

int in_process_message = FALSE;
void process_message_from(int temp_socket, int *close_fd)
{
    HMessage *m;
    assert(!in_process_message);
    in_process_message = TRUE;

    /* get the message type */
    m = receive_message(temp_socket);
    if (m == NULL)
    {
        client_unregister((HRegistMessage*)m, temp_socket);
        if (close_fd != NULL)
        {
            *close_fd = 1;
        }

        in_process_message = FALSE;
        return;
    }

    /*
     *	Based on the message type sent by the clients, we do different things.
     */
    switch (m->get_type())
    {
    case HMESG_DAEMON_REG:
        break;
    case HMESG_CLIENT_REG:
        client_registration((HRegistMessage *)m, temp_socket);
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
    case HMESG_CFG_REQ:
        send_cfg_message((HDescrMessage *)m, temp_socket);
        break;
    default:
        printf("[AH]: Wrong message type!\n");
    }

    in_process_message = FALSE;
}

int handle_new_connection(int fd)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    int new_fd = accept(fd, (struct sockaddr *)&addr, &addrlen);
    if (new_fd < 0)
    {
        printf("Error accepting connection\n");
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
    if (readlen < 0 || (unsigned int)readlen < sizeof(header))
    {
        /* Error on recv, or insufficient data.  Close the connection. */
        if (close(fd) < 0)
            printf("Error on read, but error closing connection too.\n");

        if (close_fd != NULL)
            *close_fd = 1;

        printf("[AH]: Can't determine type for socket %d.  Closing.\n", fd);
    }
    else if (ntohl(header) == HARMONY_MAGIC)
    {
        /* This is a communication socket from a harmony client. */
        printf("[AH]: Socket %d is a harmony client.\n", fd);
        harmony_fds.push_back(fd);
    }
    else
    {
        /* Consider this an HTTP communication socket. */
        if (http_fds.size() < http_connection_limit)
        {
            printf("[AH]: Socket %d is a http client.\n", fd);
            http_fds.push_back(fd);
        }
        else
        {
            printf("[AH]: Socket %d is an http client, but we have too many "
                   "already.  Closing.\n", fd);
            http_send_error(fd, 503, NULL);
            
            if (close(fd) < 0)
                printf("Error closing HTTP connection.\n");

            if (close_fd != NULL)
                *close_fd = 1;
        }
    }

    /* Do *not* remove fd from the unknown_list.  We might invalidate any
     * iterators that are currently traversing this list.
     */
}

/* Establish connection with the code server. */
int codegen_init()
{
    int fd, retval, err;
    char tmp_filename[] = "/tmp/harmony.tmp.XXXXXX";
    char buf[1024], *conf_host, *conf_dir;
    time_t session;
    FILE *fds;

    flag_dir = cfg_get("codegen_flag_dir");
    if (!flag_dir)
    {
        printf("[AH]: Configuration error: codegen_flag_dir unspecified.\n");
        return -1;
    }

    conf_host = cfg_get("codegen_conf_host");
    if (!conf_host)
    {
        printf("[AH]: Configuration error: codegen_conf_host unspecified.\n");
        return -1;
    }

    snprintf(buf, sizeof(buf), "set codegen_conf_host %s", conf_host);
    if ( (err = Tcl_Eval(tcl_inter, buf)) != TCL_OK)
    {
        fprintf(stderr, "Error %d setting global codegen_conf_host: %s\n",
                err, tcl_inter->result);
        return -1;
    }

    conf_dir = cfg_get("codegen_conf_dir");
    if (!conf_dir)
    {
        printf("[AH]: Configuration error: codegen_conf_dir unspecified.\n");
        return -1;
    }

    snprintf(buf, sizeof(buf), "set codegen_conf_dir %s", conf_dir);
    if ( (err = Tcl_Eval(tcl_inter, buf)) != TCL_OK)
    {
        fprintf(stderr, "Error %d setting global codegen_conf_dir: %s\n",
                err, tcl_inter->result);
        return -1;
    }

    fd = mkstemp(tmp_filename);
    if (fd == -1)
    {
        printf("[AH]: Error creating temporary file: %s\n", strerror(errno));
        return -1;
    }

    fds = fdopen(fd, "w+");
    if (fds == NULL)
    {
        printf("[AH]: Error during fdopen(): %s\n", strerror(errno));
        unlink(tmp_filename);
        close(fd);
        return -1;
    }

    if (time(&session) == ((time_t)-1)) {
        printf("[AH]: Error during time(): %s\n", strerror(errno));
        return -1;
    }

    fprintf(fds, "harmony_session=%ld\n", session);
    for (unsigned i = 0; i < cfg_pairlen && cfg_pair[i].key != NULL; ++i)
    {
        fprintf(fds, "%s=%s\n", cfg_pair[i].key, cfg_pair[i].val);
    }
    fclose(fds);

    snprintf(buf, sizeof(buf), "scp %s %s:%s/harmony.init",
             tmp_filename, conf_host, conf_dir);

    retval = system(buf);
    unlink(tmp_filename);
    if (retval != 0)
    {
        printf("[AH]: Error on system(\"%s\")\n", buf);
        return -1;
    }

    snprintf(buf, sizeof(buf), "%s/codeserver.init.%ld",
             cfg_get("codegen_flag_dir"), session);
    printf("[AH]: Awaiting response from code server (%s).\n", buf);
    while (!file_exists(buf))
    {
        /* Spin until the code server responds correctly. */
        sleep(1);
    }
    unlink(buf);

    printf("[AH]: Connection to code server established.\n");
    return 0;
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
    int active_sockets;
    int new_fd, close_fd;
    fd_set listen_set_copy;

    /* Start listening for new connections */
    if (listen(listen_socket, SOMAXCONN))
    {
        h_exit("Listen to socket failed!");
    }

    while (1)
    {
        listen_set_copy = listen_set;
        errno = 0;
        active_sockets = select(highest_socket + 1, &listen_set_copy,
                                NULL, NULL, NULL);
        if (active_sockets == -1)
        {
            if (errno == EINTR)
                continue;

            printf("[AH]: Error during select(): %s\n", strerror(errno));
            break;
        }

	if (debug_mode)
            printf("[AH]: Select:: Return value: %d\n", active_sockets);

        /* Handle new connections */
        if (FD_ISSET(listen_socket, &listen_set_copy))
        {
            if (debug_mode)
                printf("[AH]: processing connection request\n");

            new_fd = handle_new_connection(listen_socket);
            if (new_fd > 0)
            {
                FD_SET(new_fd, &listen_set);
                if (highest_socket < new_fd)
                    highest_socket = new_fd;
            }
        }

        /* Handle unknown connections (Unneeded if we switch to UDP) */
        socketIterator = unknown_fds.begin();
        while (socketIterator != unknown_fds.end())
        {
            close_fd = 0;
            if (FD_ISSET(*socketIterator, &listen_set_copy))
            {
                if (debug_mode)
                    printf("[AH]: determining connection type\n");

                handle_unknown_connection(*socketIterator, &close_fd);
                if (close_fd)
                    FD_CLR(*socketIterator, &listen_set);

                socketIterator = unknown_fds.erase(socketIterator);
            }
            else
            {
                ++socketIterator;
            }
        }

        /* Handle harmony messages */
        socketIterator = harmony_fds.begin();
        while (socketIterator != harmony_fds.end())
        {
            close_fd = 0;
            if (FD_ISSET(*socketIterator, &listen_set_copy))
            {
		if (debug_mode)
                {
                    if (clientId.count(*socketIterator) == 0)
                    {
                        printf("[AH]: processing harmony message from"
                               " unregistered client on socket %d\n",
                               *socketIterator);
                    }
                    else
                    {
                        printf("[AH]: processing harmony message from"
                               " client %d\n", clientId[*socketIterator]);
                    }
                }
                process_message_from(*socketIterator, &close_fd);
            }

            if (close_fd)
            {
                FD_CLR(*socketIterator, &listen_set);
                socketIterator = harmony_fds.erase(socketIterator);
            }
            else
            {
                ++socketIterator;
            }
        }

        /* Handle http requests */
        socketIterator = http_fds.begin();
        while (socketIterator != http_fds.end())
        {
            close_fd = 0;
            if (FD_ISSET(*socketIterator, &listen_set_copy))
            {
		if (debug_mode)
                     printf("[AH]: processing http request on socket %d\n",
                           *socketIterator);
 
                handle_http_socket(*socketIterator, &close_fd);
            }

            if (close_fd)
            {
                FD_CLR(*socketIterator, &listen_set);
                socketIterator = http_fds.erase(socketIterator);
            }
            else
            {
                ++socketIterator;
            }
        }

#if 0
        /*
          This section of the code is only needed if we want to use the
          commandline interface of hserver as a Tcl interpreter.
	  This is useful for debugging sessions. The users can query the
	  Tcl backend with Tcl commands. Uncomment this if you are debugging
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

/***
 *
 * This is the main program.
 * It first checks the arguments.
 * It then initializes and goes for the main loop.
 *
 ***/
int main(int argc, char **argv)
{
    char *config_filename = NULL;

    /* Reads the global_config file and retrieves the values assigned to 
       different variables in this file.
     */
    if (argc > 1)
    {
        config_filename = argv[1];
    }

    if (cfg_init(config_filename) < 0 || check_parameters() < 0)
    {
        printf("[AH]: Configuration error.  Exiting.\n");
        return -1;
    }

    if (server_startup() < 0)
    {
        printf("[AH]: Could not start server.  Exiting.\n");
        return -1;
    }

    /* Begin the main harmony loop. */
    main_loop();

    return 0;
}
