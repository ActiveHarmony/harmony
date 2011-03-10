#include <math.h>
#include <stdio.h>
#include "hclient.h"
#include "hsockutil.h"

#define MAX 20

int main(int argc, char **argv) {

  float y[MAX];
  int i;
  int loop=100;

  // performance function
  for (i=0;i<MAX;i++) {
	y[i]=(i-9)*(i-9)*(i-9)-75*(i-9)+24 ;
  }

  printf("Starting Harmony...\n");

  /* initialize the harmony server */
  /*
    This client is connecting to one server.
   */
  int handle=harmony_startup();

  printf("Sending setup file!\n");

  harmony_application_setup_file("simplext.tcl");

  int *x=NULL;
  
  /* register the tunable varible x */
  x=(int *)harmony_add_variable("SimplexT","x",VAR_INT);

  /*
   * sending default performance
   */
  harmony_performance_update(INT_MAX);

for (i=0;i<loop;i++) {
  /* update the performance result */
  harmony_performance_update(y[*x]);

  /* update the variable x */
  harmony_request_all();

  /* display the result */
  printf("y[%d]=%f\n",*x,y[*x]);
}

  /* close the session */
  harmony_end();
}
