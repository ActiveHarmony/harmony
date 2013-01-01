/*
 * Copyright 2003-2012 Jeffrey K. Hollingsworth
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
 * This is the header file for the client side of harmony.
 * 
 * Author: Cristian Tapus
 * History:
 *   July 9, 2000 - first version
 *   July 15, 2000 - comments and update added by Cristian Tapus
 *   Nov  20, 2000 - some changes made by Dejan Perkovic
 *   Dec  20, 2000 - comments and updates added by Cristian Tapus
 *  
 *******************************/

#ifndef __HCLIENT_H__
#define __HCLIENT_H__

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


#include "hutil.h"
#include "hmesgs.h"
#include "hsockutil.h"

/***
 * macro definition
 ***/

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
void harmony_startup(int use_sigs, int relocated = 0, int sport = 0, 
		     char *shost = NULL);

/*
 * When a program announces the server that it will end it calls this function
 */
void harmony_end();


/*
 * Inform the Harmony server of the bundles and requirements of the application
 */
void harmony_application_setup(char *description);

void harmony_application_setup_file(char *fname);

/*
 * Bind a local variable with a harmony bundle 
 */
void * harmony_add_variable(char *appName, char *bundleName, int type, 
			    int local = 0);


/*
 * Send to the server the value of a bound variable
 */
void harmony_set_variable(void *variable);


/*
 * Update bound variables on server'side.
 */
void harmony_set_all();


/*
 * Get the value of a bound variable
 *
 * I decided to remove the function from the API since I want the user to
 * use the request_all function that plays the role of a barrier
 */
void harmony_request_variable(void *variable);


/*
 * Get the current value of all the bound variables
 */
void harmony_request_all(int pull = 0);


/*
 * Send the performance function result to the harmony server
 */
void harmony_performance_update(int value);

#endif /* __HCLIENT_H__ */










