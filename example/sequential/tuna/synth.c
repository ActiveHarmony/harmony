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

/* This example illustrates the use of Active harmony to search a parameter
 * space for a parameterized compiler transformation framework. 
 * The underlying optimization algorithm is modified version of Nelder-Mead
 * Simplex algorithm. A more effective search algorithm (Parallel Rank
 * Ordering) is in the development phase. Please refer to our SC'05 paper 
 * (can be accessed through Active Harmony's Webpage)
 * to get preliminary idea on this algorithm.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

/* For illustration purposes, the performance here is defined by following
 * simple definition:
 *   perf = (p1-9)*(p1-9) + (p2-8)*(p2-8) + 
 *          (p3-7)*(p3-7) + (p4-6)*(p4-6) + 
 *          (p4-5)*(p4-5) + (p5-4)*(p5-4) +
 *          (p6-3)*(p6-3) + 200
 * All parameters are in [1-100] range
 * 
 */
int application(int p1, int p2, int p3, int p4, int p5, int p6) 
{
    int perf =
        (p1-150)*(p1-150) +
        (p2-300)*(p2-300) +
        (p3-450)*(p3-450) +
        (p4-600)*(p4-600) +
        (p5-750)*(p5-750) +
        (p6-900)*(p6-900) + 2000;
    printf("App got: %d %d %d %d %d %d = %d\n",
           p1, p2 ,p3 ,p4, p5, p6, perf);
    return perf;
}

int main(int argc, char **argv)
{
    long i;
    char method;
    double perf;

    method = 'w';
    for (i = 1; i < argc; ++i) {
        if (argv[i][0] == '-')
            method = argv[i][1];
        else break;
    }

    if ((argc - i) < 6) return -1;
    perf = application(atoi(argv[i+0]), atoi(argv[i+1]), atoi(argv[i+2]),
                       atoi(argv[i+3]), atoi(argv[i+4]), atoi(argv[i+5]));

    switch (method) {
    case 'w': usleep(perf); break;
    case 'u': for (i=0; i < perf*250; ++i) argc *= argc * argc; break;
    case 's': for (i=0; i < perf/2; ++i) close(open(argv[0], O_RDONLY)); break;
    case 'o': printf("%lf", perf/1000000); break;
    }
    return 0;
}
