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

/*
  A simple application for demonstration purposes.
  When you want to tune real applications, you replace a call to this
  application in your submission script.
 */


int application(int i) 
{
    return (i-9)*(i-9)*(i-9)-75*(i-9)+24 ;
}



int main(int argc, char **argv) 
{
    int param=atoi(argv[1]);
    printf("perf: %d \n", application(param));
    return 1;

}
