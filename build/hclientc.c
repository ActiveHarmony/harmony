#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "hclientc.h"
#include "hclient.h"
using namespace std;
#define DEBUG 0


// use these if you are using Active Harmony from a program compiled with c.

void c_harmony_startup(int use_sigs) {
    harmony_startup(use_sigs);
}

/*
 * When a program announces the server that it will end it calls this function
 */
void c_harmony_end() 
{
    harmony_end();
}


int c_get_hclient_id()
{
    return get_hclient_id();
}



/*
 * Inform the Harmony server of the bundles and requirements of the application
 */
void c_harmony_application_setup(char *description)
{
    harmony_application_setup(description);
}


void c_harmony_application_setup_file(char *fname){
    harmony_application_setup_file(fname);
}

/*
 * Bind a local variable with a harmony bundle 
 */
void * c_harmony_add_variable(char *appName, char *bundleName, int type)
{
    harmony_add_variable(appName, bundleName, type);
}


/*
 * Send to the server the value of a bound variable
 */
void c_harmony_set_variable(void *variable){
    harmony_set_variable(variable);
}


int c_code_generation_complete(int timestep) {
    return code_generation_complete(timestep);
}


/*
 * Update bound variables on server'side.
 */
void c_harmony_set_all()
{
    harmony_set_all();
}



/*
 * Get the value of a bound variable
 *
 * I decided to remove the function from the API since I want the user to
 * use the request_all function that plays the role of a barrier
 */
void * c_harmony_request_variable(char *variable)
{
    harmony_request_variable(variable);
}



/*
 * Get the current value of all the bound variables
 */
void c_harmony_request_all()
{
    harmony_request_all();
}



/*
 * Send the performance function result to the harmony server
 */
void c_harmony_performance_update(int value)
{
    harmony_performance_update(value);
}

void c_harmony_performance_update_with_conf(int value, char* conf)
{
    harmony_performance_update_with_conf(value, conf);
}


int c_harmony_request_probe(char *variable, int type)
{
    return harmony_request_probe(variable, type);
}

void* c_harmony_request_tcl_variable(char *variable, int type)
{
    return harmony_request_tcl_variable(variable, type);
}

int c_harmony_request_tcl_variable_2(char *variable, int type)
{
    return harmony_request_tcl_variable_2(variable, type);
}

void c_harmony_set_probe_perf(char *variable, int value)
{
    harmony_set_probe_perf(variable, value);
}


double c_harmony_database_lookup()
{
    return harmony_database_lookup();
}

