#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/stat.h>

using namespace std;
string hostname("spoon");
string user_home("/fs/spoon/tiwari/");
string confs_dir(user_home+"scratch/"+ "confs/");
string new_code_dir(user_home+"scratch/"+ "hosts/");
string code_generator_base(user_home+"standalone/");
string appname ("smg2000");
string code_destination_host("brood00");
string code_destination("tiwari@brood00:~/scratch/code");
string code_flag_destination("/scratch0/code_flags/");
string log_file("generation.log");


// might not be portable
int scp_candidate(char* filename, char* destination)
{
  char cmd[256];
  sprintf(cmd, "scp %s %s", filename, destination);
  int sys_stat=system(cmd);
  return sys_stat;
}

int touch_remote_file(const char* filename, const char* destination)
{
  char cmd[256];
  sprintf(cmd, "ssh %s touch %s ", destination, filename);
  printf("%s \n", cmd);
  int sys_stat=system(cmd);
  return sys_stat;
}
