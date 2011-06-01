#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/stat.h>

using namespace std;

// brood00
string hostname("brood00");
string user_home("/hivehomes/rahulp/");
string confs_dir(user_home+"scratch/"+"confs/");
string new_code_dir(user_home+"scratch/"+"hosts/");
string code_generator_base(user_home+"activeharmony/"+"tutorial-2010/"+"harmony/"+"standalone/");
string appname;
string num_code_gen_loc(code_generator_base+"num_code_generators");


//remote side : where do we need to transport the code
// brood
string code_destination_host("brood00");
string code_destination("rahulp@brood00:~/scratch/code");
string code_flag_destination("/scratch0/code_flags/");

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
