#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "hclient.h"
using namespace std;
#define DEBUG 1

ifdef __cplusplus
extern "C" {
#endif

    int f_harmony_startup(int sport = 0, char *shost = NULL, int use_sigs=0, int relocated = 0);

    void f_harmony_end(int socketIndex = 0);

    void f_harmony_application_setup(char *description, int socketIndex = 0);

    void f_harmony_application_setup_file(char *fname, int socketIndex = 0);

    void * f_harmony_add_variable(char *appName, char *bundleName, int type,
                                  int socketIndex = 0, int local=0);

    void f_harmony_set_variable(void *variable, int socketIndex = 0);

    void f_harmony_set_all(int socketIndex = 0);

    void * f_harmony_request_variable(char *variable, int socketIndex = 0);

    void f_harmony_request_all(int socketIndex = 0, int pull=0);

    // int metric
    void f_harmony_performance_update_int(int value, int socketIndex = 0);

    // double metric
    void f_harmony_performance_update_double(double value, int socketIndex=0);

    void * f_harmony_request_tcl_variable(char *variable, int socketIndex=0);

    char* f_harmony_get_best_configuration(int socketIndex=0);

    int f_harmony_check_convergence(int socketIndex=0);

    int f_code_generation_complete(int socketIndex=0);

    int f_harmony_code_generation_complete(int socketIndex=0);

    void* f_harmony_database_lookup(int socketIndex=0);

    void f_harmony_psuedo_barrier(int socketIndex=0);

}







/*
#ifdef __cplusplus
extern "C" {
#endif

    extern int f_check_point__(int *pn);

    extern void fharmony_startup__();

    extern void  fharmony_end__();

    extern void fharmony_application_setup__(char *description);

    extern void fharmony_application_setup_file__(char *fname);

    extern void fharmony_add_variable__(char *appName, char *bundleName, int *type, int &target);

    extern void fharmony_set_variable__(void *variable);

    extern void fharmony_set_all__();

    extern void fharmony_request_variable__(char *variable, int &target);

    extern void fharmony_request_all__(int pull = 0);

    extern void fharmony_performance_update__(int *value);

    extern void fharmony_request_tcl_variable__(char *variable, int type, int &target);
 
    extern double fharmony_database_lookup__(int &target);

    extern void code_generation_complete__(int *iteration, int &target);
    
#ifdef __cplusplus
}
#endif
int fcheck_point__(int *pn) {
    return 1;
}

void fharmony_startup__()
{
    harmony_startup(0);
}

void  fharmony_end__()
{
    harmony_end();
}

void fharmony_application_setup__(char *description)
{
    harmony_application_setup(description);
}

void fharmony_application_setup_file__(char *fname)
{
    harmony_application_setup_file(fname);
}

void fharmony_add_variable__(char *appName, char *bundleName, int *type, int &target)
{
    int *pointer = (int *) harmony_add_variable(appName, bundleName, *type);
    target = *pointer;
 
}

void fharmony_set_variable__(void *variable)
{
    harmony_set_variable(variable);
}

void fharmony_set_all__()
{
    harmony_set_all();
}

void code_generation_complete__(int *iteration, int &target)
{
    //int value = code_generation_complete(*iteration);
    int value = code_generation_complete();
    printf("FROM HCLIENTF : %d \n", *iteration);
    target=value;
}

void fharmony_request_variable__(char *variable, int &target)
{
    int * pointer = (int *) harmony_request_variable(variable);
    target = *pointer;
}

void fharmony_request_all__()
{
    harmony_request_all();
}

void fharmony_performance_update__(int *value)
{
    harmony_performance_update(*value);
}


void fharmony_request_tcl_variable_int__(char *variable, int type, int &target)
{
    int value = *(int*)harmony_request_tcl_variable(variable, type);
    target = value;
}

void fharmony_request_tcl_variable_char__(char *variable, int type, char &target)
{
    target = *(char*)harmony_request_tcl_variable(variable, type);
    
}

void fharmony_database_lookup__(double &target)
{
    int value =  harmony_database_lookup();
    target = value;
}

*/
