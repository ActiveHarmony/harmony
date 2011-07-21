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
#include <strings.h>
#include <signal.h>
#include <sys/time.h>
#include <math.h>


#include "putil.h"
#include "pmesgs.h"
#include "psockutil.h"
#include "StringTokenizer.h"

/***
 * macro definitions
 ***/

#define SERVER_PORT 2077

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


//char* global_projected = NULL;

/***
 * function prototypes
 ***/
int projection_startup(char* hostname, int sport);

/*
 * When a program announces the server that it will end it calls this function
 */
void projection_end();

/*
 * Is given point admissible?
*/
int is_a_valid_point(char* point);

/*
 * simplex construction:: saves the points to a file
 */
void simplex_construction(char* request, char* filename);

/*
 * projection: project a point to the admissible region
 */
char* do_projection_one_point(char *request);

char* projection_sim_construction_2(char* request, int mesg_type);

char* string_to_char_star(string str);

char* do_projection_entire_simplex(char *request);

/*
double rosen_2d(double x1, double x2);
double weird_1_2d(double x1_, double x2_);
double rosen_4d(double x1, double x2,double x3, double x4);
double sombrero_2d(double x, double y);
double rastrigin_2d(double x1_, double x2_) ;
double rosen_6d(double x1_, double x2_,double x3_, double x4_,double x5_,double x6_);
double sombrero_6d(double x1_, double x2_,double x3_, double x4_,double x5_,double x6_);
double rastrigin_6d(double x1_, double x2_,double x3_, double x4_,double x5_,double x6_);
double quad_6d(double x1_, double x2_,double x3_, double x4_,double x5_,double x6_);
*/
