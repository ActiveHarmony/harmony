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
 *   perf = (p1 - 15)^2 + (p2 - 30)^2 + (p3 - 45)^2 +
 *          (p4 - 60)^2 + (p5 - 75)^2 + (p6 - 90)^2
 *
 * So the theoretical minimum can be found at point:
 *      (15, 30, 45, 60, 75, 90)
 *
 * And a reasonable search range for all parameters is [1-100].
 * 
 */
int application(int p1, int p2, int p3, int p4, int p5, int p6) 
{
    int perf =
        (p1-15) * (p1-15) +
        (p2-30) * (p2-30) +
        (p3-45) * (p3-45) +
        (p4-60) * (p4-60) +
        (p5-75) * (p5-75) +
        (p6-90) * (p6-90);
    printf("App got: (%d %d %d %d %d %d) = %d",
           p1, p2 ,p3 ,p4, p5, p6, perf);
    return perf;
}

int main(int argc, char **argv)
{
    long i, stall;
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
    case 'w':
        /* Wall time */
        stall = (perf * 100) + 50000;
        printf(" ==> usleep(%ld)\n", stall);
        usleep(stall);
        break;

    case 'u':
        /* User time */
        stall = (perf * perf);
        printf(" ==> perform %ld flops\n", stall * 2);
        for (i = 0; i < stall; ++i)
            perf = perf * (1/perf);
        break;

    case 's':
        /* System time */
        stall = perf * 32;
        printf(" ==> perform %ld syscalls\n", stall * 2);
        for (i = 0; i < stall; ++i)
            close(open("/dev/null", O_RDONLY));
        break;

    case 'o':
        /* Output method */
        usleep(200000);
        printf("\n%lf\n", perf);
        break;
    }
    return 0;
}
