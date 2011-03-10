
#ifndef __HSERVER_H__
#define __HSERVER_H__

/***
 *
 * include system headers
 *
 ***/
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <map>
#include <tcl.h>
#include <tk.h>
#include <cstdio>
#include <cstdlib>
/***
 *
 * include user defined headers
 *
 ***/
#include "hutil.h"
#include "hmesgs.h"
#include "hsockutil.h"
/***
 *
 * define macros
 *
 ***/
#define SERVER_PORT 1977
#define BUFF_SIZE 1024

// change this variable to reflect where to look for code generation
// completion flag.
char code_flags_path[256]= "/scratch0/code_flags/";

#endif /* __HSERVER_H__ */
