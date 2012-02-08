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

/*
 * This example shows how to search code-variants for MM. If the user likes
 * to change the tuning from MM to some other code, he/she needs to make
 * certain changes. These changes have been marked by CHANGE THIS.
*/

#include <string.h>
#include <unistd.h>

#include <mpi.h>
#include <sys/time.h>

// dlopen
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// harmony
#include "hclient.h"
int rank = -1;
hdesc_t *hdesc = NULL;

#define SEARCH_MAX 400
#define EXT_SUCCESS 1
#define EXT_FAILURE -1
#define TAG 1234
#define N 500

// code related variables
/*
 * typedef for gemm: If you would like to tune a different code, please
 * change the typedef here.
 *  CHANGE THIS
*/
typedef void (*code_t)(void *, void *, void *, void *);
const char symbol_name[] = "gemm_";

// pointer to the library and code to evaluate
void *flib_eval;
code_t code_so;
int matrix_size = N;

/* matrix definitions for MM. If you would like to tune a different code,
 * please make appropriate changes here.
 */
// CHANGE THIS
double A[N][N];
double B[N][N];
double C[N][N];
// Error Checking
double C_orig[N][N];

// harmony related variables for mm.
// CHANGE THIS
int TI, TJ, TK, UI, UJ;

double time_start, time_end;

// new code location vars
// CHANGE THIS (if needed)
static char *code_prefix;
static char *default_so;

// CHANGE THIS (if needed)
int initialize_paths()
{
    code_prefix = harmony_query(hdesc, "example_codegen_code_prefix");
    if (code_prefix == NULL) {
        printf("[r%d] Error retrieving config key"
               " 'example_codegen_code_prefix' from server\n", rank);
        return -1;
    }

    default_so = harmony_query(hdesc, "example_codegen_default_so");
    if (default_so == NULL) {
        printf("[r%d] Error retrieving config key"
               " 'example_codegen_default_so' from server\n", rank);
        return -1;
    }

    return 0;
}

double timer()
{
    struct timezone Tzp;
    struct timeval Tp;
    int stat;
    stat = gettimeofday (&Tp, &Tzp);
    if (stat != 0) printf("[r%d] Error return from gettimeofday: %d\n",
                          rank, stat);
    return(Tp.tv_sec + Tp.tv_usec*1.0e-6);
}

/* Check if the parameter space search has converged.
 * Only rank 0 communicates directly with the Harmony server.
 */
int check_convergence(hdesc_t *hdesc)
{
    int status;

    if (rank == 0) {
        printf("[r%d] Checking Harmony search status.\n", rank);
        status = harmony_converged(hdesc);
    }
    else {
        printf("[r%d] Waiting to hear search status from rank 0.\n", rank);
    }

    /* Broadcast the result of harmony_converged(). */
    MPI_Bcast(&status, 1, MPI_INT, 0, MPI_COMM_WORLD);
    return status;
}

/*
  Construct the full pathname for the new code variant.
 */
// CHANGE THIS
char *construct_so_filename(char *buf)
{
    sprintf(buf, "%s_%d_%d_%d_%d_%d.so", code_prefix,
            TI, TJ, TK, UI, UJ);
    return buf;
}

/* Update the code we wish to test.
 * Loads function <symbol_name> from shared object <filename>,
 * and stores that address in code_ptr.
 */
int update_so(const char *filename)
{
    char *err_str;

    flib_eval = dlopen(filename, RTLD_LAZY);
    err_str = dlerror();
    if (err_str) {
        fprintf(stderr, "[r%d] Error opening %s: %s\n",
                rank, filename, err_str);
        return -1;
    }

    code_so = (code_t)dlsym(flib_eval, symbol_name);
    err_str = dlerror();
    if (err_str) {
        fprintf(stderr, "[r%d] Error finding symbol %s in %s: %s\n",
                rank, symbol_name, filename, err_str);
        return -1;
    }

    printf("[r%d] Evaluation candidate updated.\n", rank);
    return 0;
}

// mm matrix initialization
// CHANGE THIS
void initialize_matrices()
{
    int i,j;
    for (i=0; i<N; i++) {
        for (j=0; j<N; j++) {
            A[i][j] = i;
            B[i][j] = j;
            C[i][j] = 0.0;
        }
    }
}

// CHANGE THIS
int check_code_correctness()
{
    int return_val=1;
    int i,j;
    for (i=0; i<N; i++) {
        for (j=0; j<N; j++) {
            if(C[i][j] != C_orig[i][j])
            {
                printf("[r%d] C_orig[%d][%d]=%f, C[%d][%d]=%f don't match \n", rank, i,j,C_orig[i][j],i,j,C[i][j]);
                return_val=0;
                break;
            }
        }
    }
    return return_val;
}

// illustrates the use of penalization technique
int penalty_factor()
{
    int increment = 1000;
    int return_val = 0;

    if (TI < TJ)
        return_val += increment;

    if (UI*UJ > 16)
        return_val = return_val + (3 * increment);

    return return_val;
}


// driver
int main(int argc, char *argv[])
{
    char so_filename[128];
    int harmony_connected = 0;
    int simplex_size;
    char *host_name;
    double time_initial, time_current, time;
    double raw_perf, perf;
    int search_iter = 1;
    int harmonized = 0;
    int new_performance;

    /* MPI Initialization */
    MPI_Init(&argc,&argv);
    time_initial = MPI_Wtime();
    /* get the rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    /* get the simplex size. for PRO algorithm, the simplex size is the
     * number of processors
     */
    MPI_Comm_size(MPI_COMM_WORLD, &simplex_size);

    /* allocate space for the host name */
    host_name = (char *)calloc(80,sizeof(char));
    gethostname(host_name,80);

    printf("[r%d] sanity check, %s \n", rank, host_name);

    /* Insert Harmony API calls. */
    /* initialize a harmony session using only one server */
    hdesc = harmony_init("gemm", HARMONY_IO_POLL);
    if (hdesc == NULL) {
        fprintf(stderr, "Failed to initialize a harmony session.\n");
        goto cleanup;
    }

    /* declare application's runtime tunable parameters. for example, these
     * could be blocking factor and unrolling factor for matrix
     * multiplication program.
     */
    printf("[r%d] Registering harmony variables ... \n", rank);

    /* CHANGE THIS to reflect the parameters for different code versions. */
    if (harmony_register_int(hdesc, "TI", &TI, 2, 500, 2) < 0 ||
        harmony_register_int(hdesc, "TJ", &TJ, 2, 500, 2) < 0 ||
        harmony_register_int(hdesc, "TK", &TK, 2, 500, 2) < 0 ||
        harmony_register_int(hdesc, "UI", &UI, 1,   8, 1) < 0 ||
        harmony_register_int(hdesc, "UJ", &UJ, 1,   8, 1) < 0) {

        fprintf(stderr, "Error registering harmony variables.\n");
        goto cleanup;
    }

    /* Connect to Harmony server and register ourselves as a client. */
    if (harmony_connect(hdesc, NULL, 0) < 0) {
        fprintf(stderr, "Could not connect to harmony server.\n");
        goto cleanup;
    }

    harmony_connected = 1;

    /* Initialize default paths by querying the Harmony server. */
    if (initialize_paths() < 0) {
        printf("[r%d] Error during initialization.\n", rank);
        goto cleanup;
    }

    /* Prepare the initial default performance.
       This application uses a default shared object to measure performance,
       but another option is to retrieve a random server-generated simplex
       via harmony_fetch(). */
    if (update_so(default_so) < 0) {
        fprintf(stderr, "[r%d] Error loading new code.\n", rank);
        goto cleanup;
    }

    initialize_matrices();

    time_start = timer();
    code_so(&matrix_size, A, B, C_orig);
    time_end = timer();

    perf = (time_end - time_start) + penalty_factor();
    if (harmony_report(hdesc, perf) < 0) {
        fprintf(stderr, "Error reporting default performance to server.\n");
        goto cleanup;
    }

    /* Let all processes catch up, and begin the main loop. */
    MPI_Barrier(MPI_COMM_WORLD);
    printf("[r%d] Entering main loop.\n", rank);

    while (!harmonized && search_iter < SEARCH_MAX)
    {
        printf("[r%d] iteration #%d\n", rank, search_iter);
        search_iter++;

        new_performance = harmony_fetch(hdesc);
        if (new_performance < 0) {
            fprintf(stderr, "Error fetching new values from harmony.\n");
            goto cleanup;
        }
        else if (new_performance > 0) {
            /* Harmony updated variable values,
               so we must load a new shared object. */
            construct_so_filename(so_filename);
            if (update_so(so_filename) < 0) {
                printf("[r%d] *** Warning: error loading new code."
                       " Proceeding with current configuration.\n", rank);
            }
        }

        initialize_matrices();

        /* Perform and measure a single time-step of the client application. */
        time_start = timer();
        code_so(&matrix_size, A, B, C);
        time_end = timer();

        raw_perf = time_end - time_start;
        printf("[r%d] That took: %lf seconds\n", rank, raw_perf);

        check_code_correctness();

        perf = raw_perf + penalty_factor();
        printf("[r%d] TI:%d, TJ:%d, TK:%d, UI:%d, UJ:%d = %lf\n",
               rank, TI, TJ, TK, UI, UJ, perf);

        /* update the performance result */
        if (harmony_report(hdesc, perf) < 0) {
            fprintf(stderr, "Error reporting performance to server.\n");
            goto cleanup;
        }

        harmonized = check_convergence(hdesc);
        if (harmonized < 0) {
            fprintf(stderr, "[r%d] Error checking harmony convergence"
                    " status.\n", rank);
            goto cleanup;
        }

        if (harmonized > 0) {
            /* Harmony server has converged, so make one final fetch to
               load the harmonized values, and disconnect from server */
            if (harmony_fetch(hdesc) < 0) {
                fprintf(stderr, "[r%d] Error fetching new values from"
                        " Harmony server.\n", rank);
                goto cleanup;
            }
            construct_so_filename(so_filename);
            if (update_so(so_filename) < 0) {
                fprintf(stderr, "[r%d] Error loading harmonized code.\n",
                        rank);
                goto cleanup;
            }

            if (harmony_disconnect(hdesc) < 0) {
                fprintf(stderr, "[r%d] Error disconnecting from"
                        " Harmony server.\n", rank);
                goto cleanup;
            }

            /* At this point, a typical application would continue its
               execution using the harmonized values without interference
               from the harmony server. */
        }

        MPI_Barrier(MPI_COMM_WORLD);
    }

    // final book-keeping messages
    time_current = MPI_Wtime();
    time = time_current - time_initial;
    printf("[r%d] %.3f: machine=%s [# simplex_size=%i]\n",
           rank, time, host_name, simplex_size);
    
    // end the Harmony session, if needed
    if (harmonized == 0) {
        if (harmony_disconnect(hdesc) < 0) {
            fprintf(stderr, "[r%d] Error disconnecting from"
                    " Harmony server.\n", rank);
            goto cleanup;
        }
    }
    else {
        printf("[r%d] Reached final harmonized values (TI:%d, TJ:%d, TK:%d,"
               " UI:%d, UJ:%d) at iteration %d of %d\n",
               rank, TI, TJ, TK, UI, UJ, search_iter, SEARCH_MAX);
    }
    MPI_Finalize();
    return 0;

  cleanup:
    if (harmony_connected == 1) {
        if (harmony_disconnect(hdesc) < 0) {
            fprintf(stderr, "[r%d] Error disconnecting from"
                    " Harmony server.\n", rank);
        }
    }
    MPI_Finalize();
    return -1;
}
