/***
 * include other headers
 ***/

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

/***
 * macro definition
 ***/
#ifndef __HCLIENTC_H__
#define __HCLIENTC_H__

#define SERVER_PORT 1977
/*
 * the size of the buffer used to transfer data between server and client
 */
#define BUFFER_SIZE 1024
#define MAX_VAR_NAME BUFFER_SIZE

/*
 * define truth values
 */
#define false 0
#define true 1
#define VAR_INT 0
#define VAR_STR 1


/***
 * function definitions
 ***/

#ifdef __cplusplus
extern "C" {
#endif


    int c_harmony_startup();
    int c_harmony_startup_host_info(int sport, char *shost);

//extern int c_harmony_startup__(int *sport, char *shost, int *use_sigs, int *relocated);
/*
 * When a program announces the server that it will end it calls this function
 */
    void c_harmony_end();
    void c_harmony_end_host_info(int socketIndex);
    
//void c_harmony_end__(int socketIndex = 0);
/*
 * Inform the Harmony server of the bundles and requirements of the application
 */
    void c_harmony_application_setup(char *description);
    void c_harmony_application_setup_host_info(char *description, int socketIndex);
    
//void c_harmony_application_setup__(char *description, int socketIndex = 0);
    
    void c_harmony_application_setup_file(char *fname);
    void c_harmony_application_setup_file_host_info(char *fname, int socketIndex);

    //void c_harmony_application_setup_file__(char *fname, int socketIndex = 0);
/*
 * Bind a local variable with a harmony bundle 
 */
    void * c_harmony_add_variable(char *appName, char *bundleName, int type);
    void * c_harmony_add_variable_host_info(char *appName, char *bundleName, int type, int socketIndex);
/*
 * Send to the server the value of a bound variable
 */
    void c_harmony_set_variable(void *variable);
    void c_harmony_set_variable_host_info(void *variable, int socketIndex);

//void c_harmony_set_variable__(void *variable, int socketIndex = 0);

/*
 * Update bound variables on server'side.
 */
    void c_harmony_set_all();
    void c_harmony_set_all_host_info(int socketIndex);

    //void c_harmony_set_all__(int socketIndex = 0);

/*
 * Get the value of a bound variable
 *
 * I decided to remove the function from the API since I want the user to
 * use the request_all function that plays the role of a barrier
 */
    void * c_harmony_request_variable(char *variable);
    void * c_harmony_request_variable_host_info(char *variable, int socketIndex);


    //void * c_harmony_request_variable__(char *variable, int socketIndex = 0);

/*
 * Get the current value of all the bound variables
 */
    void c_harmony_request_all();
    void c_harmony_request_all_host_info(int socketIndex);
    //void c_harmony_request_all__(int socketIndex = 0, int pull=0);

/*
 * Send the performance function result to the harmony server
 */
// int metric
    void c_harmony_performance_update_int(int value);
    void c_harmony_performance_update_int_host_info(int value, int socketIndex);
    //void c_harmony_performance_update_int__(int value, int socketIndex = 0);

// double metric
    void c_harmony_performance_update_double(double value);
    void c_harmony_performance_update_double_host_info(double value, int socketIndex);
    //void c_harmony_performance_update_double__(double value, int socketIndex=0);

    void* c_harmony_request_tcl_variable(char *variable);
    void* c_harmony_request_tcl_variable_host_info(char *variable, int socketIndex);

    //void* c_harmony_request_tcl_variable__(char *variable, int socketIndex=0);
    
    char* c_harmony_get_best_configuration();
    char* c_harmony_get_best_configuration_host_info(int socketIndex);

    //char* c_harmony_get_best_configuration__(int socketIndex=0);
    
    int c_harmony_check_convergence();
    int c_harmony_check_convergence_host_info(int socketIndex);
    //int c_harmony_check_convergence__(int socketIndex=0);
    
    int c_code_generation_complete();
    int c_code_generation_complete_host_info(int socketIndex);

    //int c_code_generation_complete__(int socketIndex=0);
    
    int c_harmony_code_generation_complete();
    int c_harmony_code_generation_complete_host_info(int socketIndex);
    //int c_harmony_code_generation_complete__(int socketIndex=0);
    
    void* c_harmony_database_lookup();
    void* c_harmony_database_lookup_host_info(int socketIndex);

    //void* c_harmony_database_lookup__(int socketIndex=0);
    
    void c_harmony_psuedo_barrier();
    void c_harmony_psuedo_barrier_host_info(int socketIndex);
    //void c_harmony_psuedo_barrier__(int socketIndex=0);
#ifdef __cplusplus
}
#endif

#endif /* __HCLIENTC_H__ */











