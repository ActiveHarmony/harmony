/*
 * Copyright 2003-2013 Jeffrey K. Hollingsworth
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

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <mpi.h>

#include "hsession.h"
#include "hclient.h"
#include "defaults.h"

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

hdesc_t *hdesc = NULL;
int rank = -1;
/* Variables to hold the application's runtime tunable parameters.
 * Once bound to a Harmony tuning session, these variables will be
 * modified upon harmony_fetch() to a new testing configuration.
 */
long param_1;
long param_2;
long param_3;
long param_4;
long param_5;
long param_6;


long application(long p1, long p2, long p3, long p4, long p5, long p6)
{
    long perf =
        (p1-15) * (p1-15) +
        (p2-30) * (p2-30) +
        (p3-45) * (p3-45) +
        (p4-60) * (p4-60) +
        (p5-75) * (p5-75) +
        (p6-90) * (p6-90);
    return perf;
}

int main(int argc, char **argv)
{
    const char *name, *retstr;
    char *ptr;
    hsession_t sess;

    int i, retval, loop = 200;
    long perf = -1000;
	int node_count;
	char numbuf[12];
	name = "TauDB_example";
	MPI_Init(&argc, &argv);

    retval = 0;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            fprintf(stderr, "Usage: %s [session_name]"
                    " [KEY_1=VAL_1] .. [KEY_N=VAL_N]\n\n", argv[0]);
            return 0;
        }
    }


	/*Get rank and size of this MPI application*/
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &node_count);
	
	if (rank == 0) {
    	/* Initialize a Harmony tuning session. */
	    hsession_init(&sess);
		snprintf(numbuf, sizeof(numbuf), "%d", node_count);

    	/* Process the program arguments. */
	    i = 1;
	    
	    if (argc > 1 && !strchr(argv[1], '=')) {
	        name = argv[1];
	        ++i;
	    }

	    if (hsession_name(&sess, name) < 0 ||
			hsession_cfg(&sess, CFGKEY_SESSION_PLUGINS, "tauDB.so") < 0 ||
			hsession_cfg(&sess, CFGKEY_SESSION_STRATEGY, "pro.so") < 0 ||
			hsession_cfg(&sess, CFGKEY_CLIENT_COUNT, numbuf) < 0 ||
			hsession_cfg(&sess, CFGKEY_PREFETCH_COUNT, "auto") < 0) {
	        fprintf(stderr, "Error during session configuration.\n");
	        MPI_Abort(MPI_COMM_WORLD, -1);
	    }

	    while (i < argc) {
	        ptr = strchr(argv[i], '=');
	        if (!ptr) {
	            fprintf(stderr, "Invalid parameter '%s'\n", argv[i]);
	            MPI_Abort(MPI_COMM_WORLD, -1);
	        }

	        *(ptr++) = '\0';
	        if (hsession_cfg(&sess, argv[i], ptr) < 0) {
	            fprintf(stderr, "Failed to set config var %s\n", argv[i]);
	            MPI_Abort(MPI_COMM_WORLD, -1);
	        }
	        ++i;
	    }
	
	    if (hsession_int(&sess, "param_1", 1, 100, 1) < 0 ||
	        hsession_int(&sess, "param_2", 1, 100, 1) < 0 ||
	        hsession_int(&sess, "param_3", 1, 100, 1) < 0 ||
	        hsession_int(&sess, "param_4", 1, 100, 1) < 0 ||
	        hsession_int(&sess, "param_5", 1, 100, 1) < 0 ||
	        hsession_int(&sess, "param_6", 1, 100, 1) < 0)
	    {
	        fprintf(stderr, "Failed to define tuning session\n");
	        MPI_Abort(MPI_COMM_WORLD, -1);
	    }
	
		printf("Launching tuning session.\n");
	
	    retstr = hsession_launch(&sess, NULL, 0);
	    if (retstr) {
	        fprintf(stderr, "Could not launch tuning session: %s\n", retstr);
	        MPI_Abort(MPI_COMM_WORLD, -1);
	    }
	}

	MPI_Barrier(MPI_COMM_WORLD);

    printf("Starting Harmony...\n");

    /* Initialize a Harmony client. */
    hdesc = harmony_init();
    if (hdesc == NULL) {
        fprintf(stderr, "Failed to initialize a harmony session.\n");
        return -1;
    }

    /* Bind the session variables to local variables. */
    if (harmony_bind_int(hdesc, "param_1", &param_1) < 0 ||
        harmony_bind_int(hdesc, "param_2", &param_2) < 0 ||
        harmony_bind_int(hdesc, "param_3", &param_3) < 0 ||
        harmony_bind_int(hdesc, "param_4", &param_4) < 0 ||
        harmony_bind_int(hdesc, "param_5", &param_5) < 0 ||
        harmony_bind_int(hdesc, "param_6", &param_6) < 0)
    {
        fprintf(stderr, "Failed to register variable\n");
        goto cleanup;
    }

    /* Join this client to the tuning session we established above. */
    if (harmony_join(hdesc, NULL, 0, name) < 0) {
        fprintf(stderr, "Could not connect to harmony server: %s\n",
                harmony_error_string(hdesc));
        goto cleanup;
    }

	printf("Connected to harmony server.\n");
	//MPI_Barrier(MPI_COMM_WORLD);

    /* main loop */
    for (i = 0; !harmony_converged(hdesc) && i < loop; i++) {
        int hresult = harmony_fetch(hdesc);
        if (hresult < 0) {
            fprintf(stderr, "Failed to fetch values from server: %s\n",
                    harmony_error_string(hdesc));
            retval = -1;
            goto cleanup;
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

        if (hresult > 0) {
            /* Only print performance if new values were fetched. */
            printf("%ld, %ld, %ld, %ld, %ld, %ld = %ld\n",
                   param_1, param_2, param_3,
                   param_4, param_5, param_6, perf);
        }

        /* Report the performance we've just measured. */
        if (harmony_report(hdesc, perf) < 0) {
            fprintf(stderr, "Failed to report performance to server.\n");
            retval = -1;
            goto cleanup;
        }

		MPI_Barrier(MPI_COMM_WORLD);
    }

    /* Leave the session */
    if (harmony_leave(hdesc) < 0) {
        fprintf(stderr, "Failed to disconnect from harmony server.\n");
        retval = -1;
    }

	MPI_Finalize();

  cleanup:
    harmony_fini(hdesc);
    return retval;
}
