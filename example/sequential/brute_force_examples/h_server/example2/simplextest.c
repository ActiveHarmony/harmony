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

#define MAX 20

int main(int argc, char **argv) {

  float f[MAX][MAX];
  int i,j;
  int loop=40;

for (j=0;j<MAX;j++) 
  for (i=0;i<MAX;i++) {
	f[i][j]=(i-9)*(i-9)+(j-5)*(j-5)+24 ;
	//printf("%f ",y[i]);
  }

  //printf("\n");

  //printf("Starting Harmony...%d\n", argc);

  /* initialize the harmony server */
  harmony_startup(0);

  printf("Sending setup file!\n");

  harmony_application_setup_file("simplext.tcl");

  int *x=NULL;
  int *y=NULL;
  
  /* register the tunable varible x */
  x=(int *)harmony_add_variable("SimplexT","x",VAR_INT);
  y=(int *)harmony_add_variable("SimplexT","y",VAR_INT);


for (i=0;i<loop;i++) {


  /* display the result */
  printf("f[%d][%d]=%f\n",*x,*y,f[*x][*y]);

  /* update the performance result */
  harmony_performance_update(f[*x][*y]);

  /* update the variable x */
  harmony_request_all();

}

  /* close the session */
  harmony_end();
}
