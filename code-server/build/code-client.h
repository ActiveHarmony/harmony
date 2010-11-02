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


#include "cutil.h"
#include "cmesgs.h"
#include "csockutil.h"
#include "Tokenizer.h"

/***
 * macro definitions
 ***/

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


//char* global_projected = NULL;

/***
 * function prototypes
 ***/
void codeserver_startup(char* hostname, int sport);

/*
 * When a program announces the server that it will end it calls this function
 */
void codeserver_end();

int code_generation_complete();
void generate_code(char *request);
char* string_to_char_star(string str);





