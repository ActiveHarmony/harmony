
/***
 *
 * include user defined headers
 *
 ***/
#include "cutil.h"
#ifdef linux
#include <sys/prctl.h>
#include <signal.h>
#endif

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

