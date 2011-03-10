#include <math.h>
#include <stdio.h>
#include "hclient.h"
#include "hsockutil.h"

#define MAX 200

int main(int argc, char **argv) {

  float f[MAX][MAX];
  int i,j;
  int loop=200;

  for (j=0;j<MAX;j++) 
      for (i=0;i<MAX;i++) {
          f[i][j]=(i-9)*(i-9)+(j-5)*(j-5)+24 ;
      }

  printf("Starting Harmony...\n");

  /* initialize the harmony server */
  /*
   * Connecting to one server.
   */
  int handle=harmony_startup();

  printf("Sending setup file!\n");

  harmony_application_setup_file("simplext.tcl");

  int *x=NULL;
  int *y=NULL;

  /* register the tunable varible x */
  x=(int *)harmony_add_variable("SimplexT","x",VAR_INT);
  y=(int *)harmony_add_variable("SimplexT","y",VAR_INT);


  /*
   * send in default performance. In this case, there is no notion of
   * default, so we simply send INT_MAX.
   */

  harmony_performance_update(INT_MAX);

  for (i=0;i<loop;i++) {
      
      
      /* display the result */
      printf("f[%d][%d]=%f\n",*x,*y,f[*x][*y]);
      
      /* update the performance result */
      harmony_performance_update(f[*x][*y]);
      
      //int harmony_ended=harmony_check_convergence();
      /* update the variable x */
      harmony_request_all(); 
      
  }
  
  /* close the session */
  harmony_end();
}
