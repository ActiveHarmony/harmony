/*
 * Copyright 2003-2011 Jeffrey K. Hollingsworth
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
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/stat.h>

using namespace std;
// local code server info: where is the main code running?
// buzz.cs.umd.edu
/*
string hostname("buzz");
string user_home("/fs/buzz/tiwari/");
string confs_dir(user_home+"scratch/"+ "confs/");
string new_code_dir(user_home+"scratch/"+ "hosts/");
string code_generator_base(user_home+"standalone/");
string appname ("irs");
//string appname ("smg2000");
string log_file("generation.log");
string num_code_gen_loc(confs_dir+"num_code_generators");
*/

// spoon.cs.umd.edu
string hostname("spoon");
string user_home("/fs/spoon/tiwari/");
string confs_dir(user_home+"scratch/"+ "confs/");
string new_code_dir(user_home+"scratch/"+ "hosts/");
string code_generator_base(user_home+"standalone/");
string appname;
//string appname ("smg2000");
//string log_file("generation.log");
string num_code_gen_loc(confs_dir+"num_code_generators");


//remote side : where do we need to transport the code
// brood
string code_destination_host("brood00");
string code_destination("tiwari@brood00:~/scratch/code");
string code_flag_destination("/scratch0/code_flags/");

// hopper
//string code_destination_host("hopper.nersc.gov");
//string code_destination("tiwari@hopper.nersc.gov:/global/scratch/sd/tiwari/code");
//string code_flag_destination("/global/scratch/sd/tiwari/code_flags/");

// carver
//string code_destination_host("carver.nersc.gov");
//string code_destination("tiwari@carver.nersc.gov:/global/scratch/sd/tiwari/code");
//string code_flag_destination("/global/scratch/sd/tiwari/code_flags/");


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
