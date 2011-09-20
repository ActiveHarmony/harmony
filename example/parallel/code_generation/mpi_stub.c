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
int h_id = -1;

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
char *symbol_name="gemm_";
// This should match the name of the application that you have indicated in
// .tcl file.

char code_name_prefix[50]="gemm";

code_t code_so_best;
code_t code_so_eval;
// pointer to the best code
void * flib_best;
// pointer to the code to evaluate
void * flib_eval;
char *dlError;


char so_filename[128];
int matrix_size=500;


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
int *TI=NULL;
int *TJ=NULL;
int *TK=NULL;
int *UI=NULL;
int *UJ=NULL;

int current_best_perf=INT_MAX;
double time_start, time_end;
// we are sending int perf.
int perf_multiplier=10000;

// new code location vars
// CHANGE THIS (if needed)
static char *code_prefix;
static char *default_so;

// CHANGE THIS (if needed)
int initialize_paths()
{
    code_prefix = harmony_get_config_variable("example_codegen_code_prefix");
    if (!code_prefix) {
        printf("[r%d:h%d] Error retrieving config key"
               " 'example_codegen_code_prefix' from server\n", rank, h_id);
        return -1;
    }

    default_so = harmony_get_config_variable("example_codegen_default_so");
    if (!default_so) {
        printf("[r%d:h%d] Error retrieving config key"
               " 'example_codegen_default_so' from server\n", rank, h_id);
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
    if (stat != 0) printf("[r%d:h%d] Error return from gettimeofday: %d\n",
                          rank, h_id, stat);
    return(Tp.tv_sec + Tp.tv_usec*1.0e-6);
}

/*
  Construct the full pathname for the new code variant.
 */
// CHANGE THIS
void construct_so_filename()
{
    sprintf(so_filename, "%s_%d_%d_%d_%d_%d.so", code_prefix,
            *TI, *TJ, *TK, *UI, *UJ);
}

// update the evaluation pointer: This is called once the new code becomes available.
void update_so_eval()
{
    // construct the name of the file to be evaluated.
    construct_so_filename();
    flib_eval=dlopen(so_filename, RTLD_LAZY);
    dlError = dlerror();
    if( dlError ) {
        printf("[r%d:h%d] cannot open .so file! %s \n",
               rank, h_id, so_filename);
        exit(1);
    }


    // fetch the symbol
    code_so_eval=(code_t)dlsym(flib_eval, symbol_name);
    dlError = dlerror();
    if( dlError )
    {
        printf("[r%d:h%d] cannot find matmult !\n", rank, h_id);
        exit(1);
    }
    printf("[r%d:h%d] Evaluation candidate updated \n", rank, h_id);
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
                printf("[r%d:h%d] C_orig[%d][%d]=%f, C[%d][%d]=%f don't match \n", rank, h_id, i,j,C_orig[i][j],i,j,C[i][j]);
                return_val=0;
                break;
            }
        }
    }
    return return_val;
}

// CHANGE THIS
int eval_code(int best)
{
    initialize_matrices();
    if(best)
    {
        time_start=timer();
        // CHANGE THIS
        code_so_best(&matrix_size, A, B, C);
        time_end=timer();
        printf("[r%d:h%d] That took: %f \n", rank, h_id, time_end-time_start);
    } else
    {
        time_start=timer();
        // CHANGE THIS
        code_so_eval(&matrix_size, A, B, C);
        time_end=timer();
        printf("[r%d:h%d] That took: %f \n", rank, h_id, time_end-time_start);
        check_code_correctness();
    }
    return ((int)((time_end-time_start)*perf_multiplier));
}

// CHANGE THIS
// evaluates the original version of mm
int eval_original()
{

    int i,j;

    for (i=0; i<N; i++) {
        for (j=0; j<N; j++) {
            A[i][j] = i;
            B[i][j] = j;
            C_orig[i][j] = 0.0;
        }
    }

    sprintf(so_filename, "%s", default_so);
    printf("[r%d:h%d] Opening the default file: %s \n",
           rank, h_id, so_filename);
    flib_eval=dlopen(so_filename, RTLD_LAZY);
    dlError = dlerror();
    if( dlError ) {
        printf("[r%d:h%d] Cannot open .so file! %s \n",
               rank, h_id, so_filename);
        exit(1);
    }

    // fetch the symbol
    code_so_eval=(code_t)dlsym(flib_eval, symbol_name);
    code_so_best=(code_t)dlsym(flib_eval, symbol_name);
    dlError = dlerror();
    if( dlError )
    {
        printf("[r%d:h%d] cannot find matmult !\n", rank, h_id);
        exit(1);
    }
    time_start=timer();
    code_so_eval(&matrix_size, A, B, C_orig);
    time_end=timer();
    printf("[r%d:h%d] That took: %f \n", rank, h_id, time_end-time_start);
    return ((int)((time_end-time_start)*perf_multiplier));
}

/*
 * One can minimize the tcp messages between the harmony server and the
 * client using this function. Here only rank 0 queries the server and
 * passes the response to the other clients.
 */
static int buffer[1];
MPI_Status recv_status;
int get_code_completion_status(int simplex_size)
{
    int return_val=0;
    // check if the code is ready for the new set of parameters
    // only rank 0 communicated directly with the harmony server
    if(rank == 0)
    {
        printf("[r%d:h%d] asking the server about the code status \n", rank, h_id);
        return_val = harmony_code_generation_complete();
        printf("[r%d:h%d] code status %d \n", rank, h_id, return_val);
        buffer[0] = return_val;
        // send this to other processors
        int proc_no=1;
        for(proc_no = 1; proc_no < simplex_size; proc_no++)
            MPI_Send(buffer, 1, MPI_INT, proc_no, TAG, MPI_COMM_WORLD);
        buffer[0]=0;
    } else
    {
        printf("[r%d:h%d] waiting to hear from the 0 \n", rank, h_id);
        MPI_Recv(buffer, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &recv_status);
        return_val=buffer[0];
        printf("[r%d:h%d] code status %d \n", rank, h_id, return_val);
        buffer[0]=0;
    }
    MPI_Barrier(MPI_COMM_WORLD); // let everyone catch up
    return return_val;
}

int check_convergence(int simplex_size)
{
    int return_val=0;
    // check if the code is ready for the new set of parameters
    // only rank 0 communicated directly with the harmony server
    if(rank == 0)
    {
        return_val = harmony_check_convergence();
        printf("[r%d:h%d] search_done %d \n", rank, h_id, return_val);
        buffer[0] = return_val;
        // send this to other processors
        int proc_no=1;
        for(proc_no = 1; proc_no < simplex_size; proc_no++)
            MPI_Send(buffer, 1, MPI_INT, proc_no, TAG, MPI_COMM_WORLD);
        buffer[0]=0;
    } else
    {
        printf("[r%d:h%d] waiting to hear from the 0 \n", rank, h_id);
        MPI_Recv(buffer, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &recv_status);
        return_val=buffer[0];
        printf("[r%d:h%d] search status %d \n", rank, h_id, return_val);
        buffer[0]=0;
    }
    MPI_Barrier(MPI_COMM_WORLD); // let everyone catch up
    return return_val;
}

// illustrates the use of penalization technique
int penalty_factor()
{
    int increment=1000;
    int return_val=0;
    if(*TI < *TJ)
        return_val+=increment;

    if((*UI)*(*UJ) > 16)
        return_val=return_val+3*increment;

    return return_val;
}


// driver
int  main(int argc, char *argv[])
{
    int    simplex_size, i;
    char   *host_name;
    double time_initial, time_current, time;
    char  app_desc[128];
    int code_complete=0;
    int perf;
    int j;
    int search_iter=1;
    int harmony_ended=0;
    int new_performance=1;
    char* best_conf_from_server;


    /* MPI Initialization */
    MPI_Init(&argc,&argv);
    time_initial  = MPI_Wtime();
    /* get the rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    /* get the simplex size. for PRO algorithm, the simplex size is the
     * number of processors
     */
    MPI_Comm_size(MPI_COMM_WORLD, &simplex_size);

    /* allocate space for the host name */
    host_name=(char *)calloc(80,sizeof(char));

    gethostname(host_name,80);

    printf("[r%d:h%d] sanity check, %s \n", rank, h_id, host_name);

    /* Insert Harmony API calls. */
    /* initialize the harmony server */
    /* Using only one server */
    h_id = harmony_startup();

    // initialize the paths
    if (initialize_paths() < 0) {
        printf("[r%d:h%d] Error during initialization.\n", rank, h_id);
        MPI_Finalize();
        return -1;
    }

    /* send in the application description to the server */
    printf("[r%d:h%d] sending the app description \n", rank, h_id);
    sprintf(app_desc, "offline.tcl");
    harmony_application_setup_file("offline.tcl");

    /* declare application's runtime tunable parameters. for example, these
     * could be blocking factor and unrolling factor for matrix
     * multiplication program.
     */

    printf("[r%d:h%d] Adding harmony variables ... \n", rank, h_id);
    /* register the tunable varibles */
    // CHANGE THIS to reflect the parameters for different code versions.
    TI=(int *)harmony_add_variable(code_name_prefix,"TI",VAR_INT);
    TJ=(int *)harmony_add_variable(code_name_prefix,"TJ",VAR_INT);
    TK=(int *)harmony_add_variable(code_name_prefix,"TK",VAR_INT);
    UI=(int *)harmony_add_variable(code_name_prefix,"UI",VAR_INT);
    UJ=(int *)harmony_add_variable(code_name_prefix,"UJ",VAR_INT);
    printf("[r%d:h%d] entering \n", rank, h_id);

    // evaluate the original version
    perf=eval_original();
    current_best_perf=perf;

    /* send in the default performance */
    printf("[r%d:h%d] sending the default perf \n", rank, h_id);
    harmony_performance_update(perf);
    MPI_Barrier(MPI_COMM_WORLD);
    
    // loop until the search does not converge or we run a predefined set
    // of iterations
    while(!harmony_ended && search_iter < SEARCH_MAX)
    {
        printf("[r%d:h%d] iteration # %d \n", rank, h_id, search_iter);
        search_iter++;
        new_performance=0;

        // see if the new code is ready
        //if(get_code_completion_status(simplex_size))
        if(harmony_code_generation_complete())
        {
            harmony_request_all();
            update_so_eval();
            perf=eval_code(0)+penalty_factor();
            new_performance=1;

	    if(perf < current_best_perf)
            {
                // update the best pointer
                best_conf_from_server=harmony_get_best_configuration();
                printf("[r%d:h%d] Best Conf from server: %s \n",
                       rank, h_id, best_conf_from_server);

		if(best_conf_from_server!=NULL && *best_conf_from_server!='\0')
                {
		  sprintf(so_filename, "%s_%s.so",
                          code_prefix, best_conf_from_server);

	        }

		free(best_conf_from_server);
                flib_best=dlopen(so_filename, RTLD_LAZY);
                dlError = dlerror();
                if( dlError ) {
                    printf("[r%d:h%d] cannot open .so file! %s \n",
                           rank, h_id, so_filename);
                    exit(1);
                }

                // fetch the symbol
                code_so_best=(code_t)dlsym(flib_best, symbol_name);
                dlError = dlerror();
                if( dlError )
                {
                    printf("[r%d:h%d] cannot find %s !\n",
                           rank, h_id, symbol_name);
                    exit(1);
                }
            }
        }
        else
        {
            printf("[r%d:h%d] code generation is not complete \n", rank, h_id);
            // code generation is not complete.
            // run the best variant
            perf=eval_code(1);
        }

        printf("[r%d:h%d] TI: %d,TJ: %d, TK: %d, UI: %d, UJ: %d, perf: %d \n", rank, h_id, rank, *TI, *TJ, *TK, *UI, *UJ, perf);

        // does the perf that we just determined reflect the changes in the
        // parameter values? If it does, then send the performance to the server.
        if(new_performance)
            harmony_performance_update(perf);

        // check if the search is done
        printf("[r%d:h%d] harmony_ended call \n", rank, h_id);
        harmony_ended=check_convergence(simplex_size);

        if(harmony_ended)
        {
            // get the final set of parameters
            harmony_request_all();
            update_so_eval();
            perf=eval_code(0);
            
	    printf("[r%d:h%d] TI: %d,TJ: %d, TK: %d, UI: %d, UJ: %d, perf: %d", rank, h_id, rank, *TI, *TJ, *TK, *UI, *UJ, perf);

	    // If we are using online tuning for real applications, we can
            // call the harmony_end() function here and continue execution
            // with the best performing candidate.
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    // end the session
    harmony_end();

    // final book-keeping messages
    time_current  = MPI_Wtime();
    time  = time_current - time_initial;
    printf("[r%d:h%d] %.3f: machine=%s [# simplex_size=%i]\n",
           rank, h_id, time, host_name, simplex_size);
    MPI_Finalize();
    return 0;
}
