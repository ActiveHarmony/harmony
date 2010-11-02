
/***
 *
 * include user defined headers
 *
 ***/
#include "cutil.h"
#include <signal.h>

/***
 *
 * Exit and send a mesage to stderr
 *
 ***/
void h_exit(char *errmesg){
  perror(errmesg);
#ifdef linux
	prctl(PR_SET_PDEATHSIG, SIGHUP);
#endif
  exit(1);
}

