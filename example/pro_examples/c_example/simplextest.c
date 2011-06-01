/*
 * Copyright 2003-2011 Jeffrey K. Hollingsworth
 *
 * This file is part of Active Harmony.
 *
 * Active Harmony is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Active Harmony is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Active Harmony.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <math.h>
#include <stdio.h>
#include "hclientc.h"


#define MAX 101

void psuedo_barrier()
{
    int next_iteration=0;
    // psuedo barrier using: DO NOT USE THIS FOR ONLINE TUNING
    // next_iteration variable is set if harmony server is done calculating
    // new set of parameters
    next_iteration=*((int*)c_harmony_request_tcl_variable("next_iteration", VAR_INT));
    printf("next_iteration : %d \n", next_iteration);
    while(next_iteration != 1)
    {
        sleep(1);printf("next_iteration : %d \n", next_iteration);
        next_iteration=*(int*)c_harmony_request_tcl_variable("next_iteration", VAR_INT);
    }
}

/*
double rosenbrock_2d(double x1_, double x2_, int div, int start) {
  double x1 = start+(x1_/div);
  double x2 = start+(x2_/div);
  double f = 100*(x2-x1*x1)*(x2-x1*x1) + (1-x1)*(1-x1);
  printf("%f %f ==> %f \n", x1_, x2_, f);
  return f;
}

double sombrero_2d(double x, double y)
{
  double x_n = -6+(x/10);
  double y_n = -6+(y/10); 
  double r = ((x_n-2)*(x_n-2))+((y_n-2)*(y_n-2))+1;
  double f = (sin(1.0)-(sin(r)/r));
  printf("%f %f ==> %f \n", x, y, f);
  return f;
}
*/
 
int simple(double x, double y) 
{
    return (x-9)*(x-9)+(y-5)*(y-5)+24 ;
}

int main(int argc, char **argv) {

  float f[MAX][MAX];
  int i,j;
  int loop=20;

  /*
for (j=0;j<MAX;j++) 
  for (i=0;i<MAX;i++) {
	f[i][j]=(i-9)*(i-9)+(j-5)*(j-5)+24 ;
  }
  */

  /* initialize the harmony server */
  c_harmony_startup(0);

  printf("Sending setup file!\n");

  c_harmony_application_setup_file("simplext.tcl");

  int *x=NULL;
  int *y=NULL;
  /* register the tunable varible x */
  printf("Adding the variables ... \n");
  x=(int *)c_harmony_add_variable("SimplexT","x",VAR_INT);
  y=(int *)c_harmony_add_variable("SimplexT","y",VAR_INT);

  c_harmony_performance_update(2147483647);
  psuedo_barrier();
  
  for (i=0;i<loop;i++) {
      
      c_harmony_request_all();
      /* display the result */
      int perf=(simple(*x,*y));
      printf("%d %d = %d\n",*x,*y,perf);
      /* update the performance result */
      printf("Sending the performance \n");
      c_harmony_performance_update(perf);

      psuedo_barrier();

      /* update the variable x */
      c_harmony_request_all();

      int result = c_harmony_database_lookup(VAR_INT);
      printf("databse lookup returned: %d \n", result);
  }
  /* close the session */
  c_harmony_end();
}
 
