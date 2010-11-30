
#include <math.h>
#include <stdio.h>
#include "hclient.h"
#include "hsockutil.h"

#define MAX 20

int main(int argc, char **argv) {

  float y[MAX];
  float temp[MAX];
  int i;
  int loop=100;

  for (i=0; i < MAX; i++) {
	y[i]=(i-9)*(i-9)*(i-9)-75*(i-9)+24 ;
	temp[i]=y[i];
	printf("%f ",y[i]);
  }

  printf("\n");

  printf("Starting Harmony...%d\n", argc);
  
  /* initialize the harmony server */
  harmony_startup(0);
  
  printf("Sending setup file!\n");
  
  harmony_application_setup_file("simplext.tcl");
  
  int *x=NULL;
  
  /* register the tunable varible x */
  x=(int *)harmony_add_variable("SimplexT","x",VAR_INT);
  printf("x: %d \n", *x);


  for (i=0;i<loop;i++) {

    
    /* display the result */
    printf("original value: temp[%d]=%f\n", *x, temp[*x]);
    printf(" new value: y[%d]=%f\n",*x,y[*x]);
    
    printf("x: %d \n", *x);
    /* update the performance result */
    harmony_performance_update(y[*x]);
    
    /* update the variable x */
    harmony_request_all();
    
  }
  
  /* close the session */
  harmony_end();
}
