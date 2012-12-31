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
//harmony
#include "hclient.h"

#include <mpi.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>

#define MAX 500
#define EXT_SUCCESS 1
#define EXT_FAILURE -1
#define TAG 1234
#define PERF_MAX 2147483647

#define N 1000000

typedef void (*code_t)(void *, void *, void *, void *);
char *symbol_name="binarrsearch_";

char *code_name_prefix="binarrsearch";

code_t code_so_best;
code_t code_so_eval;

//pointer to the best code
void * flib_best;

//pointer to the code to evaluate
void * flib_eval;
char *dlError;

char so_filename[256];
int array_size=500;

int a[] = {1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33};
int n = sizeof(a)/sizeof(int);

//Array definitions. 
double A[N];
double B[N];
double E[N];

//Error checking
double E_check[N];
double E_orig[N];

//harmony related variables for the array
int *Tx=NULL;
int *Ty=NULL;

int current_best_perf=PERF_MAX;
double time_start, time_end;

//sending the int perf.
int perf_array=10000;

//new code location variables
char path_prefix_def[256];
char path_prefix_code[256];
char path_prefix_run_dir[256];

void initialize_paths()
{
  sprintf(path_prefix_run_dir,"/hivehomes/rahulp/testprograms");
  sprintf(path_prefix_def,"%s/%s_default.so",path_prefix_run_dir, code_name_prefix);
  sprintf(path_prefix_code,"/hivehomes/rahulp/scratch/code/%s", code_name_prefix);
}

//constructing the full pathname for new code variant
void construct_so_filename()
{
  sprintf(so_filename,"%s_%d_%d_%d_%d_%d.so",path_prefix_code, *Tx, *Ty);
}


// update the evaluation pointer.
//This will be called once the new code becomes available
void update_so_eval()
{
  //Contruct the name of the file to be evaluated
  construct_so_filename();
  flib_eval=dlopen(so_filename, RTLD_LAZY);
  dlError = dlerror();
  
  if( dlError )
  {
    printf("cannot open .so file! %s \n", so_filename);
    exit(1);
  }

  //fetch the symbol
  code_so_eval=(code_t)dlsym(flib_eval, symbol_name);
  dlError = dlerror();

  if( dlError )
  {
    printf("cannot find binaryarraysearch !\n");
    exit(1);
  }
  printf("Evaluation candidate updated \n");
}

// Array initialization
int initialize_binary1()
{
  //int n, value;
  //int a[n];
  int value;
  int left = 0;
  int right = n-1;
 
  while(left <= right)
  {
      int mid = (left + right)/2;
      if(value<a[mid])
      {
        right = mid -1;
        E_check[right];
      }
      else if (value>a[mid])
      {
        left = mid+1;
        E_check[left];
      }
      else
      {
	return mid;
	E_check[mid];
      }
  }
  return -1;
}

// Check code correctness
int check_code_correctness()
{
  //int n, value;
  //int a[n];
  int value;
  int return_val=1;
  int left = 0;
  int right = n-1;
  
  while(left <= right)
  {
      int mid = (left + right)/2;
      if(value<a[mid])
      {
	if (E_check[right] != E_orig[right])
	{
	  printf("E_orig[%d]=%f, E_check[%d]=%f don't match \n", right, E_orig[right], E_check[right]);
	   return_val=0;
	   break;
	}

      }
      else if (value>a[mid])
      {
	if(E_check[left] != E_orig[left])
	{
	  printf("E_orig[%d]=%f, E_check[%d]=%f don't match \n", right, E_orig[right], E_check[right]);
		     return_val=0;
		     break;
	}
      }
      else
      {
	  if(E_check[mid] != E_orig[mid])
	 {
	   printf("E_orig[%d]=%f, E_check[%d]=%f don't match \n", right, E_orig[right], E_check[right]);
	     return_val=0;
       	     break;
         }
      }
    }

    return return_val;

}

//Evaluating the code
int eval_code(int best) 
{
  initialize_binary1();

  if(best)
  {
    time_start=timer();
      
    code_so_best(&array_size, A, B, E);
    time_end=timer();
    printf("That took: %f \n", time_end-time_start);
  }
  else
  {
    time_start=timer();
    code_so_eval(&array_size, A, B, E);
    time_end=timer();
    printf("That took: %f \n", time_end-time_start);
    check_code_correctness();
  }
  return ((int)((time_end-time_start)*perf_array));
}

//Evaluating the original code
int eval_original()
{
  //int n, value;
  //int a[n];
  int value;
  int left = 0;
  int right = n-1;
  
  while(left <= right)
  {
      int mid = (left + right)/2;
      if(value<a[mid])
      {
        right = mid -1;
        E_check[right];
      }
      else if (value>a[mid])
      {
        left = mid+1;
        E_check[left];
      }
      else
      {
        return mid;
        E_check[mid];
      }
  }

  sprintf(so_filename, "%s", path_prefix_def);
  printf("Opening the default file: %s \n", so_filename);
  flib_eval=dlopen(so_filename, RTLD_LAZY);
  dlError = dlerror();

  if( dlError )
  {
    printf("cannot open .so file! %s \n", so_filename);
    exit(1);
  }

  //Fetch the symbol
  code_so_eval=(code_t)dlsym(flib_eval, symbol_name);
  code_so_best=(code_t)dlsym(flib_eval, symbol_name);

  dlError = dlerror();

  if( dlError )
  {
    printf("cannot find binaryarraysearch !\n");
    exit(1);
  }

  time_start=timer();
  code_so_eval(&array_size, A, B, E_check);
  time_end=timer();
  printf("That took: %f \n", time_end-time_start);
  return ((int)((time_end-time_start)*perf_array));

}

//Minimizing the tcp messages between the harmony server and the client
static int buffer[1];
MPI_Status recv_status;
int get_code_completion_status(int simplex_size, int rank)
{
    int return_val=0;

    //check if the code is ready for the new set of parameters
    //only rank 0 communicated directly with the harmony server
    if(rank==0)
    {
      printf("asking the server about the code status \n");
      return_val = harmony_code_generation_complete();
      printf("%d: code status %d \n", rank, return_val);
      buffer[0] = return_val;

      //send this to other processors
      int proc_no = 1;
      for(proc_no = 1; proc_no < simplex_size; proc_no++)
      {
        MPI_Send(buffer, 1, MPI_INT, proc_no, TAG, MPI_COMM_WORLD);
      }
      buffer[0]=0;
    }
    else
    {
       printf("%d is waiting to hear from the 0 \n", rank);
       MPI_Recv(buffer, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &recv_status);
       return_val=buffer[0];
       printf("%d: code status %d \n", rank, return_val);
       buffer[0]=0;
    }

    //let everyone catch up
    MPI_Barrier(MPI_COMM_WORLD);
    return return_val;
}

int check_convergence(int simplex_size, int rank)
{
    int return_val=0;
    
    //check if the code is ready for the new set of parameters
    //only rank 0 communicated directly with the harmony server
    if(rank==0)
    {
      return_val = harmony_check_convergence();
      printf("%d: search done %d \n", rank, return_val);
      buffer[0] = return_val;

      //send this to other processors
      int proc_no = 1;
      for(proc_no = 1; proc_no < simplex_size; proc_no++)
      {
        MPI_Send(buffer, 1, MPI_INT, proc_no, TAG, MPI_COMM_WORLD);
      }
      buffer[0]=0;
    }
    else
    {
      printf("%d is waiting to hear from the 0 \n", rank);
      MPI_Recv(buffer, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &recv_status);
      return_val=buffer[0];
      printf("%d: search status %d \n", rank, return_val);
      buffer[0]=0;
    }

  //let everyone catch up
  MPI_Barrier(MPI_COMM_WORLD);
  return return_val;
}

// illustrates the use of penalization technique
int penalty_factor()
{
  int increment=1000;
  int return_val=0;
  if(*Tx < *Ty)
  {
    return_val+=increment;
  }
  return return_val;
}

int binary2()
{
  //int n, value;
  //int a[n];
  int value;
  int p = n/2;
  while(n>0)
  {
    n=n/2;
    if(value<a[p])
    {
      p -= n;
    }
    else if (value>a[p])
    {
      p += n;
    }
    else
    {
        return p;
    }
  }
  return -1;
}

long timediff(struct timeval before, struct timeval after)
{
  long sec = after.tv_sec - before.tv_sec;
  long microsec = after.tv_usec - before.tv_usec;
  return 1000000*sec + microsec;
}

int main(int argc, char **argv)
{
  //int a[] = {1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33};
  //int n = sizeof(a)/sizeof(int);
  //int where;
  //struct timeval before;
  //struct timeval after;
  //int k;
  //int loop=1000000;

  int rank, simplex_size;
  char *host_name;
  double time_initial, time_current, time;
  char app_desc[256];
  //int code_complete=0;
  int perf; 
  int search_iter=1;
  int harmony_ended=0;
  int new_performance=1;
  char *best_conf_from_server;

  //MPI Initialization
  MPI_Init(&argc, &argv);
  time_initial = MPI_Wtime();

  //Get the rank
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  //Get the simplex size
  MPI_Comm_size(MPI_COMM_WORLD, &simplex_size);

  //Allocating the space for the host name
  host_name=(char *)calloc(80,sizeof(char));

  gethostname(host_name,80);

  printf("sanity check, %s \n", host_name);

  //Inserting the Harmony API calls
  //Initializing the harmony server
  //Using one server
  harmony_startup();

  //Sending the application description to the server
  printf("Sending the application description \n");
  sprintf(app_desc, "binarrsearch.tcl");
  harmony_application_setup_file("binarrsearch.tcl");

  printf("Adding harmony variables..... \n");
  
  //registering the tunable variables
  int *Tx=NULL;
  int *Ty=NULL;

  Tx=(int *)harmony_add_variable(code_name_prefix,"Tx",VAR_INT);
  Ty=(int *)harmony_add_variable(code_name_prefix,"Ty",VAR_INT);

  printf("entering \n");

  //Initialize the paths
  initialize_paths();

  //Evaluating the original version
  perf=eval_original();
  current_best_perf=perf;

  //sending the default performance
  printf("sending the default perf \n");
  harmony_performance_update(perf);

  MPI_Barrier(MPI_COMM_WORLD);

  // loop until the search does not converge or we run a predefined set of iterations
  while(!harmony_ended && search_iter < MAX)
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
	    if(best_conf_from_server!=NULL && best_conf_from_server!='\0')
            {
	      sprintf(so_filename, "%s_%s.so", path_prefix_code, best_conf_from_server);
            }
	    free(best_conf_from_server);
	    flib_best=dlopen(so_filename, RTLD_LAZY);
	    dlError = dlerror();
	    if( dlError ) 
	    {
	      printf("cannot open .so file! %s \n", so_filename);
	      exit(1);
	    }
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
      else
      {
	// code generation is not complete. run the best variant
	printf("code generation is not complete \n");
	perf=eval_code(1);
      }

      printf("rank :%d, Tx: %d, Ty: %d, perf: %d\n",rank, *Tx, *Ty, perf);

      //Sending the performance to the server
      if(new_performance)
      {
	harmony_performance_update(perf);
      }

      //check whether the search is done
      printf("harmony_ended call \n");
      harmony_ended=check_convergence(simplex_size, rank);

      if(harmony_ended)
      {
	//Get the final set of parameters
	harmony_request_all();
	update_so_eval();
	perf=eval_code(0);
	printf("rank :%d, Tx: %d,Ty: %d, perf: %d\n",rank, *Tx, *Ty, perf);
	// If we are using online tuning for real applications, we can
	// call the harmony_end() function here and continue execution
	// with the best performing candidate.
      }
      MPI_Barrier(MPI_COMM_WORLD);
  }

  // end the session
  harmony_end();

  time_current = MPI_Wtime();
  
  time = time_current - time_initial;

  printf("%.3f rank=%i : machine=%s [# simplex_size=%i]\n",time, rank, host_name, simplex_size);
  
  MPI_Finalize();

  return 0;
}



