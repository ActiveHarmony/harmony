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
 * to change the tuning from MM to some other code, she needs to make
 * certain changes. These changes have been marked by CHANGE THIS.
*/



#include <mpi.h>
#include <sys/time.h>
// dlopen
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// harmony
#include "hclient.h"

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
char *code_name_prefix="gemm";

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
char path_prefix_def[128];
char path_prefix_code[128];
char path_prefix_run_dir[128];

// CHANGE THIS (if needed)
void initialize_paths()
{
    sprintf(path_prefix_run_dir,"/hivehomes/tiwari/tutorial-2010/harmony/example/parallel/code_generation");
    sprintf(path_prefix_def,"%s/%s_default.so",path_prefix_run_dir, code_name_prefix);
    sprintf(path_prefix_code,"/hivehomes/tiwari/scratch/code/%s", code_name_prefix);
}

double timer()
{
    struct timezone Tzp;
    struct timeval Tp;
    int stat;
    stat = gettimeofday (&Tp, &Tzp);
    if (stat != 0) printf("Error return from gettimeofday: %d",stat);
    return(Tp.tv_sec + Tp.tv_usec*1.0e-6);
}

/*
  Construct the full pathname for the new code variant.
 */
// CHANGE THIS
void construct_so_filename()
{
     sprintf(so_filename,"%s_%d_%d_%d_%d_%d.so",path_prefix_code,*TI, *TJ,*TK,*UI,*UJ);
}


// update the evaluation pointer: This is called once the new code becomes available.
void update_so_eval()
{
    // construct the name of the file to be evaluated.
    construct_so_filename();
    flib_eval=dlopen(so_filename, RTLD_LAZY);
    dlError = dlerror();
    if( dlError ) {
        printf("cannot open .so file! %s \n", so_filename);
        exit(1);
    }
    // fetch the symbol
    code_so_eval=(code_t)dlsym(flib_eval, symbol_name);
    dlError = dlerror();
    if( dlError )
    {
        printf("cannot find matmult !\n");
        exit(1);
    }
    printf("Evaluation candidate updated \n");
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
                printf("C_orig[%d][%d]=%f, C[%d][%d]=%f don't match \n", i,j,C_orig[i][j],i,j,C[i][j]);
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
        printf("That took: %f \n", time_end-time_start);
    } else
    {
        time_start=timer();
        // CHANGE THIS
        code_so_eval(&matrix_size, A, B, C);
        time_end=timer();
        printf("That took: %f \n", time_end-time_start);
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

    sprintf(so_filename,"%s",path_prefix_def);
    printf("Opening the default file: %s \n", so_filename);
    flib_eval=dlopen(so_filename, RTLD_LAZY);
    dlError = dlerror();
    if( dlError ) {
        printf("cannot open .so file! %s \n", so_filename);
        exit(1);
    }

    // fetch the symbol
    code_so_eval=(code_t)dlsym(flib_eval, symbol_name);
    code_so_best=(code_t)dlsym(flib_eval, symbol_name);
    dlError = dlerror();
    if( dlError )
    {
        printf("cannot find matmult !\n");
        exit(1);
    }
    time_start=timer();
    code_so_eval(&matrix_size, A, B, C_orig);
    time_end=timer();
    printf("That took: %f \n", time_end-time_start);
    return ((int)((time_end-time_start)*perf_multiplier));
}

/*
 * One can minimize the tcp messages between the harmony server and the
 * client using this function. Here only rank 0 queries the server and
 * passes the response to the other clients.
 */
static int buffer[1];
MPI_Status recv_status;
int get_code_completion_status(int simplex_size, int rank)
{
    int return_val=0;
    // check if the code is ready for the new set of parameters
    // only rank 0 communicated directly with the harmony server
    if(rank == 0)
    {
        printf("asking the server about the code status \n");
        return_val = harmony_code_generation_complete();
        printf("%d: code status %d \n", rank, return_val);
        buffer[0] = return_val;
        // send this to other processors
        int proc_no=1;
        for(proc_no = 1; proc_no < simplex_size; proc_no++)
            MPI_Send(buffer, 1, MPI_INT, proc_no, TAG, MPI_COMM_WORLD);
        buffer[0]=0;
    } else
    {
        printf("%d is waiting to hear from the 0 \n", rank);
        MPI_Recv(buffer, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &recv_status);
        return_val=buffer[0];
        printf("%d: code status %d \n", rank, return_val);
        buffer[0]=0;
    }
    MPI_Barrier(MPI_COMM_WORLD); // let everyone catch up
    return return_val;
}

int check_convergence(int simplex_size, int rank)
{
    int return_val=0;
    // check if the code is ready for the new set of parameters
    // only rank 0 communicated directly with the harmony server
    if(rank == 0)
    {
        return_val = harmony_check_convergence();
        printf("%d: search_done %d \n", rank, return_val);
        buffer[0] = return_val;
        // send this to other processors
        int proc_no=1;
        for(proc_no = 1; proc_no < simplex_size; proc_no++)
            MPI_Send(buffer, 1, MPI_INT, proc_no, TAG, MPI_COMM_WORLD);
        buffer[0]=0;
    } else
    {
        printf("%d is waiting to hear from the 0 \n", rank);
        MPI_Recv(buffer, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &recv_status);
        return_val=buffer[0];
        printf("%d: search status %d \n", rank, return_val);
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
    int    rank, simplex_size, i;
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

    printf("sanity check, %s \n", host_name);

    /* Insert Harmony API calls. */
    /* initialize the harmony server */
    /* Using only one server */
    harmony_startup();

    /* send in the application description to the server */
    printf("sending the app description \n");
    sprintf(app_desc, "offline.tcl");
    harmony_application_setup_file("offline.tcl");
    /* declare application's runtime tunable parameters. for example, these
     * could be blocking factor and unrolling factor for matrix
     * multiplication program.
     */
    
    printf("Adding harmony variables ... \n");
    /* register the tunable varibles */
    // CHANGE THIS to reflect the parameters for different code versions.
    TI=(int *)harmony_add_variable(code_name_prefix,"TI",VAR_INT);
    TJ=(int *)harmony_add_variable(code_name_prefix,"TJ",VAR_INT);
    TK=(int *)harmony_add_variable(code_name_prefix,"TK",VAR_INT);
    UI=(int *)harmony_add_variable(code_name_prefix,"UI",VAR_INT);
    UJ=(int *)harmony_add_variable(code_name_prefix,"UJ",VAR_INT);
    printf("entering \n");

    // initialize the paths
    initialize_paths();
    
    // evaluate the original version
    perf=eval_original();
    current_best_perf=perf;
    //MPI_Barrier(MPI_COMM_WORLD);
    /* send in the default performance */
    printf("sending the default perf \n");
    harmony_performance_update(perf);
    MPI_Barrier(MPI_COMM_WORLD);
    //harmony_request_all();

    // loop until the search does not converge or we run a predefined set
    // of iterations
    while(!harmony_ended && search_iter < SEARCH_MAX)
    {
        printf("iteration # %d \n", search_iter);
        search_iter++;
        new_performance=0;

        // see if the new code is ready
        //if(get_code_completion_status(simplex_size, rank))
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
                printf("Best Conf from server: %s \n", best_conf_from_server);
                if(best_conf_from_server!=NULL && *best_conf_from_server!='\0')
                {
                    sprintf(so_filename, "%s_%s.so", path_prefix_code, best_conf_from_server);
                }
                free(best_conf_from_server);
                flib_best=dlopen(so_filename, RTLD_LAZY);
                dlError = dlerror();
                if( dlError ) {
                    printf("cannot open .so file! %s \n", so_filename);
                    exit(1);
                }

                // fetch the symbol
                code_so_best=(code_t)dlsym(flib_best, symbol_name);
                dlError = dlerror();
                if( dlError )
                {
                    printf("cannot find %s !\n", symbol_name);
                    exit(1);
                }
            }
        }
        else
        {
            printf("code generation is not complete \n");
            // code generation is not complete.
            // run the best variant
            perf=eval_code(1);
        }

        printf("rank :%d, TI: %d,TJ: %d, TK: %d, UI: %d, UJ: %d, perf: %d\n", 
               rank, *TI, *TJ, *TK, *UI, *UJ, perf);

        // does the perf that we just determined reflect the changes in the
        // parameter values? If it does, then send the performance to the server.
        if(new_performance)
            harmony_performance_update(perf);

        //MPI_Barrier(MPI_COMM_WORLD);

        // check if the search is done
        printf("harmony_ended call \n");
        harmony_ended=check_convergence(simplex_size, rank);

        if(harmony_ended)
        {
            // get the final set of parameters
            harmony_request_all();
            update_so_eval();
            perf=eval_code(0);
            printf("rank :%d, TI: %d,TJ: %d, TK: %d, UI: %d, UJ: %d, perf: %d\n", 
               rank, *TI, *TJ, *TK, *UI, *UJ, perf);
            // If we are using online tuning for real applications, we can
            // call the harmony_end() function here and continue execution
            // with the best performing candidate.
        }
        MPI_Barrier(MPI_COMM_WORLD);
    } //while(search_iter < SEARCH_MAX);

    // end the session
    harmony_end();

    // final book-keeping messages
    time_current  = MPI_Wtime();
    time  = time_current - time_initial;
    printf("%.3f rank=%i : machine=%s [# simplex_size=%i]\n",
           time, rank, host_name, simplex_size);
    MPI_Finalize();
    return 0;
}
