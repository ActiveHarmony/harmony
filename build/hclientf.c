#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "hclient.h"
using namespace std;
#define DEBUG
/*
  extern void harmony_startup(int use_sigs, int relocated = 0, int sport = 0,
  char *shost = NULL);
  extern void harmony_end();

  extern void harmony_application_setup(char *description);

  extern void harmony_application_setup_file(char *fname);

  extern void * harmony_add_variable(char *appName, char *bundleName, int type,
  int local = 0);
  extern void harmony_set_variable(void *variable);

  extern void harmony_set_all();

  extern void harmony_request_variable(void *variable);

  extern void harmony_request_all(int pull = 0);

  extern void harmony_performance_update(int value);

  extern int harmony_request_probe(char *variable, int type);

  extern void harmony_set_probe_perf(char *variable, int value);
*/
#ifdef __cplusplus
extern "C" {
#endif

    extern int fcheck_point__(int *pn);

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
    int value = code_generation_complete(*iteration);
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

