

/***
 *
 * include system headers
 *
 ***/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#ifndef __HUTIL_H__
#define __HUTIL_H__

#define TRUE 1
#define FALSE 0

/***
 *
 * Exit and write a message to stderr
 *
 ***/
void h_exit(char *errmesg);


#endif /* __HUTIL_H__ */
