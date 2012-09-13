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
#include "hclient.h"
#include "hsockutil.h"

#define MAX 101
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
    return ((x-9)*(x-9)+(y-5)*(y-5)+24);
}

int main(int argc, char **argv) {

  float f[MAX][MAX];
  int i,j;
  int loop=20;

  /* initialize the harmony server */
  harmony_startup(0);

  printf("Sending setup file!\n");

  harmony_application_setup_file("simplext.tcl");
  int *x=NULL;
  int *y=NULL;

  /* register the tunable varible x */
  printf("Adding the variables ... \n");
  x=(int *)harmony_add_variable("SimplexT","x",VAR_INT);
  y=(int *)harmony_add_variable("SimplexT","y",VAR_INT);
  harmony_performance_update(2147483647.0);
  harmony_psuedo_barrier();

  for (i=0;i<loop;i++) {
      int return_req=harmony_request_all();

      printf("The return value of harmony_request_all is :%d \n", return_req);
      /* display the result */
      printf("Confs : %d %d \n", *x,*y);

      int perf=(simple(*x,*y));
      printf("%d %d = %d \n",*x,*y,perf);
      /* update the performance result */
      printf("Sending the performance \n");
      if(return_req!=1)
	harmony_performance_update(perf);

      harmony_psuedo_barrier();

      /* update the variable x */
      //harmony_request_all();
      //double result = atof((char*)harmony_database_lookup());
      //printf("databse lookup returned: %.15g \n", result);
      //char* best_conf = harmony_get_best_configuration();
      //printf("best conf: %s \n", best_conf);
  }
  /* close the session */
  harmony_end();
}
