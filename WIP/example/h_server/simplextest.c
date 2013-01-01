/*
 * Copyright 2003-2012 Jeffrey K. Hollingsworth
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
