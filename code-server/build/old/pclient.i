/* File : nearest_neighbor.i */
%module nearest_neighbor
%{
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
#include "StringTokenizer.h"
#include "pclient.h"

%}

void projection_startup();
void projection_end();
int is_a_valid_point(char* point);
char* simplex_construction(char* request);
string do_projection(char *request);
char* projection_sim_construction_2(char* request, int mesg_type);
char* string_to_char_star(string str);
