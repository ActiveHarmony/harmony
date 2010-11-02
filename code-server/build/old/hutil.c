
/***
 *
 * include user defined headers
 *
 ***/
#include "hutil.h"

/***
 *
 * Exit and send a mesage to stderr
 *
 ***/
void h_exit(char *errmesg){
  perror(errmesg);
  exit(1);
}

