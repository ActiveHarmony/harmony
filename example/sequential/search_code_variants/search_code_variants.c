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

#include <math.h>
#include <stdio.h>
#include "hclient.h"

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
        (p1-45)*(p1-9) + (p2-65)*(p2-8) + 
        (p3-85)*(p3-7) - (p4-75)*(p4-6) +
        (p5-55)*(p5-4) -
        (p6-45)*(p6-3) - 200;
    
    return perf;
}

int main(int argc, char **argv)
{
    hdesc_t *hd;
    int i;
    int loop = 200;
    int perf = -1000;

    printf("Starting Harmony...\n");

    /* initialize a harmony session */
    hd = harmony_init("SearchT", HARMONY_IO_POLL);
    if (hd == NULL) {
        fprintf(stderr, "Failed to initialize a harmony session.\n");
        return -1;
    }

    /* declare application's runtime tunable parameters. for example, these
     * could be blocking factor and unrolling factor for matrix
     * multiplication program.
     */
    int param_1;
    int param_2;
    int param_3;
    int param_4;
    int param_5;
    int param_6;

    /* register the tunable varibles */
    if (harmony_register_int(hd, "param_1", &param_1, 1, 100, 1) < 0 ||
        harmony_register_int(hd, "param_2", &param_2, 1, 100, 1) < 0 ||
        harmony_register_int(hd, "param_3", &param_3, 1, 100, 1) < 0 ||
        harmony_register_int(hd, "param_4", &param_4, 1, 100, 1) < 0 ||
        harmony_register_int(hd, "param_5", &param_5, 1, 100, 1) < 0 ||
        harmony_register_int(hd, "param_6", &param_6, 1, 100, 1) < 0) {

        fprintf(stderr, "Failed to register variable\n");
        return -1;
    }

    /* Connect to Harmony server and register ourselves as a client. */
    if (harmony_connect(hd, NULL, 0) < 0) {
        fprintf(stderr, "Could not connect to harmony server.\n");
        return -1;
    }

    /* main loop */
    for (i = 0; i < loop; i++) {
        int hresult = harmony_fetch(hd);
        if (hresult < 0) {
            fprintf(stderr, "Failed to fetch values from server.\n");
            return -1;
        }
        else if (hresult == 0) {
            /* New values were not available at this time.
             * Bundles remain unchanged by Harmony system.
             */
        }
        else if (hresult > 0) {
            /* The Harmony system modified the variable values.
             * Do any systemic updates to deal with such a change.
             */
        }

        /* Run one full iteration of the application (or code variant).
         *
         * Here our application is rather simple. Definition of performance can
         * be user-defined. Depending on application, it can be MFlops/sec,
         * time to complete the entire run of the application, cache hits vs.
         * misses and so on.
         *
         * For searching the parameter space in a Transformation framework,
         * just run different parameterized code variants here. A simple
         * mapping between the parameters and the code-variants is needed to
         * call the appropriate code variant.
         */

        perf = application(param_1, param_2, param_3,
                           param_4, param_5, param_6);
        printf("%d, %d, %d, %d, %d, %d = %d\n",
               param_1, param_2, param_3,
               param_4, param_5, param_6, perf);

        /* update the performance result */
        if (harmony_report(hd, perf) < 0) {
            fprintf(stderr, "Failed to report performance to server.\n");
            return -1;
        }
    }

    /* close the session */
    if (harmony_disconnect(hd) < 0) {
        fprintf(stderr, "Failed to disconnect from harmony server.\n");
        return -1;
    }
    return 0;
}
