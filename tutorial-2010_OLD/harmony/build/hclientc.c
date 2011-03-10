#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "hclientc.h"
#include "hclient.h"
using namespace std;
#define DEBUG 0


int c_harmony_startup()
{
    return harmony_startup();
}

int c_harmony_startup_host_info(int sport, char *shost)
{
    return harmony_startup(sport, shost);
}

/*
  int c_harmony_startup__(int *sport, char *shost, int *use_sigs, int *relocated)
  {
  printf("Got the following parameters: %d %s %d %d \n", *sport, *shost, *use_sigs, *relocated);
  return harmony_startup(*sport, shost, *use_sigs, *relocated);
  }
*/
void c_harmony_end()
{
    harmony_end();
}

void c_harmony_end_host_info(int socketIndex)
{
    harmony_end(socketIndex);
}

/*
void c_harmony_end__(int socketIndex)
{
    harmony_end(socketIndex);
}
*/

void c_harmony_application_setup_host_info(char *description, int socketIndex)
{
    harmony_application_setup(description, socketIndex);
}

void c_harmony_application_setup(char *description)
{
    harmony_application_setup(description);
}

void c_harmony_application_setup_file(char *fname)
{
    harmony_application_setup_file(fname);
}

void c_harmony_application_setup_file_host_info(char *fname, int socketIndex)
{
    harmony_application_setup_file(fname, socketIndex);
}

/*
void c_harmony_application_setup_file__(char *fname, int socketIndex)
{
harmony_application_setup_file(fname, socketIndex);
}
*/
void* c_harmony_add_variable(char *appName, char *bundleName, int type)
{
    return harmony_add_variable(appName, bundleName, type);
}

void* c_harmony_add_variable_host_info(char *appName, char *bundleName, int type,
                              int socketIndex, int local)
{
    return harmony_add_variable(appName, bundleName, type,
                                socketIndex, local);
}

/*
void* c_harmony_add_variable__(char *appName, char *bundleName, int type,
                              int socketIndex, int local)
{
    return harmony_add_variable(appName, bundleName, type,
                                socketIndex, local);
}
*/

void c_harmony_set_variable(void *variable)
{
    harmony_set_variable(variable);
}
void c_harmony_set_variable_host_info(void *variable, int socketIndex)
{
    harmony_set_variable(variable, socketIndex);
}

void c_harmony_set_all()
{
    harmony_set_all();
}

void c_harmony_set_all_host_info(int socketIndex)
{
    harmony_set_all(socketIndex);
}

void* c_harmony_request_variable(char *variable)
{
    return harmony_request_variable(variable);
}

void* c_harmony_request_variable_host_info(char *variable, int socketIndex)
{
    return harmony_request_variable(variable, socketIndex);
}

/*
void* c_harmony_request_variable__(char *variable, int socketIndex)
{
    return harmony_request_variable(variable, socketIndex);
}
*/
void c_harmony_request_all()
{
    harmony_request_all();
}

void c_harmony_request_all_host_info(int socketIndex)
{
    harmony_request_all(socketIndex);
}

/*
void c_harmony_request_all__(int socketIndex, int pull)
{
    harmony_request_all(socketIndex, pull);
}
*/
// int performance metric
void c_harmony_performance_update_int(int value)
{
    harmony_performance_update(value);
}

void c_harmony_performance_update_int_host_info(int value, int socketIndex)
{
    harmony_performance_update(value, socketIndex);
}
/*
void c_harmony_performance_update_int__(int value, int socketIndex)
{
    harmony_performance_update(value, socketIndex);
}
*/
// double performance metric
void c_harmony_performance_update_double(double value)
{
    harmony_performance_update(value);
}
void c_harmony_performance_update_double_host_info(double value, int socketIndex)
{
    harmony_performance_update(value, socketIndex);
}

/*
void* c_harmony_request_tcl_variable(char *variable, int socketIndex)
{
    return harmony_request_tcl_variable(variable, socketIndex);
}
*/

char* c_harmony_get_best_configuration()
{
    return harmony_get_best_configuration();
}

char* c_harmony_get_best_configuration_host_info(int socketIndex)
{
    return harmony_get_best_configuration(socketIndex);
}

int c_harmony_check_convergence()
{
    return harmony_check_convergence();
}

int c_harmony_check_convergence_host_info(int socketIndex)
{
    return harmony_check_convergence(socketIndex);
}
int c_code_generation_complete()
{
    return code_generation_complete();
}

int c_code_generation_complete_host_info(int socketIndex)
{
    return code_generation_complete(socketIndex);
}

int c_harmony_code_generation_complete()
{
    return harmony_code_generation_complete();
}

int c_harmony_code_generation_complete_host_info(int socketIndex)
{
    return harmony_code_generation_complete(socketIndex);
}

void* c_harmony_database_lookup()
{
    return harmony_database_lookup();
}

void* c_harmony_database_lookup_host_info(int socketIndex)
{
    return harmony_database_lookup(socketIndex);
}
void c_harmony_psuedo_barrier()
{
    harmony_psuedo_barrier();
}

void c_harmony_psuedo_barrier_host_info(int socketIndex)
{
    harmony_psuedo_barrier(socketIndex);
}

 
