/* File : code_client.i */
%module code_client
%{
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
#include <map>
#include <vector>


#include "cutil.h"
#include "cmesgs.h"
#include "csockutil.h"
//#include "Tokenizer.h"
#include "code-client.h"

%}

void codeserver_startup(char* hostname, int sport);
void codeserver_end();
int code_generation_complete();
void generate_code(char *request);
char* string_to_char_star(string str);

