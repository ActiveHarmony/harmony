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
 * include other headers 
 ***/

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>


//#include "hutil.h"
//#include "hmesgs.h"
//#include "hsockutil.h"

/***
 * macro definition
 ***/
#ifndef __HCLIENTC_H__
#define __HCLIENTC_H__

#define SERVER_PORT 1977

/*
 * the size of the buffer used to transfer data between server and client
 */
#define BUFFER_SIZE 1024

#define MAX_VAR_NAME BUFFER_SIZE

/*
 * define truth values
 */
#define false 0
#define true 1
#define VAR_INT 0
#define VAR_STR 1


/***
 * function definitions
 ***/

/*
 * When a program registers with the harmony server it uses this function
 *
 * Parms:
 *  use_sigs (int) - the client will use signals if the value is 1
 *		     the client will use polling if the value is 0
 * 		     the client will use sort of a polling if value is 2
 *  relocated (int)-
 *  sport (int)    - the port of the server 
 *  shost (char *) - the host of the server

 */
#ifdef __cplusplus
extern "C" {
#endif
void c_harmony_startup(int use_sigs);
/*
 * When a program announces the server that it will end it calls this function
 */
void c_harmony_end();


int c_get_hclient_id();


/*
 * Inform the Harmony server of the bundles and requirements of the application
 */
void c_harmony_application_setup(char *description);

void c_harmony_application_setup_file(char *fname);

/*
 * Bind a local variable with a harmony bundle 
 */
void * c_harmony_add_variable(char *appName, char *bundleName, int type);


/*
 * Send to the server the value of a bound variable
 */
void c_harmony_set_variable(void *variable);

int c_code_generation_complete(int timestep);

/*
 * Update bound variables on server'side.
 */
void c_harmony_set_all();


/*
 * Get the value of a bound variable
 *
 * I decided to remove the function from the API since I want the user to
 * use the request_all function that plays the role of a barrier
 */
void * c_harmony_request_variable(char *variable);


/*
 * Get the current value of all the bound variables
 */
void c_harmony_request_all();


/*
 * Send the performance function result to the harmony server
 */
void c_harmony_performance_update(int value);
void c_harmony_performance_update_with_conf(int value, char* conf);

int c_harmony_request_probe(char *variable, int type);
void* c_harmony_request_tcl_variable(char *variable, int type);
int c_harmony_request_tcl_variable_2(char *variable, int type);
void c_harmony_set_probe_perf(char *variable, int value);

double c_harmony_database_lookup();
#ifdef __cplusplus
}
#endif



#endif /* __HCLIENT_H__ */











