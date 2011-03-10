/*
 * include system headers
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>

/*
 * include user headers
 */
#include "pmesgs.h"
#include "putil.h"

using namespace std;

#ifndef __PSOCKUTIL_H__
#define __PSOCKUTIL_H__


/* the maximum buffer size for a message is 64K */
#define MAX_BUFFER_SIZE 65535

/***
 *
 * Here we define some useful functions to handle data communication
 *
 ***/

/**
 * send a message on the given socket
 **/
void send_message(HMessage *, int sock);

/**
 * read a message from the given socket
 **/
HMessage *receive_message(int sock);

#endif /* ifndef _PSOCKUTIL_H__ */


