/*
 * Copyright 2003-2012 Jeffrey K. Hollingsworth
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
#include "utilities.h"
#include "hserver.h"
#include <sys/param.h>
using namespace std;

// globals
map<string,int> global_data;
map<string,int> transport_data;
map<string,int>::iterator it;
generator *gen_ptr;
int nchildren = 1;
int timestep = 1;
char working_directory[MAXPATHLEN];

static int debug_mode=1;
int server_pid=0;
int usr_interrupt=0;

// signal handler
void sigusr1(int signalnum){
  if(debug_mode) {
    printf("inside the signal handler for SIGUSR1. ");
    printf("This is process number: %d \n", getpid());
  }
  usr_interrupt=1;
}

/*
 * generators: These are the real work-horses. For each new configuration, we 
 *  fork a new process to generate code. These processes die after the code
 *  generation is complete. We block all the signals because we do not want to
 *  interrupt any code generation activity.
*/
pid_t generator_make(int i, vector<int> params, int prim)
{
    pid_t pid;
    if(debug_mode)
      printf("GEN MAKE I: %d \n", i);
    
    //prototype for the main
    void generator_main(int, vector<int>,int);
 
    // fork the process.
    if((pid = fork()) > 0) {
      gen_ptr[i].parameters=params;
      gen_ptr[i].avail=1;
      gen_ptr[i].pid=pid;

      /* assumption here is that we can safely mark this configuration as
       *  done (meaning code generation is complete).
       *  Error checking is not done currently.
       */
      vector<int> temp_params;
      cout << "vec to string " << vector_to_string(params) << "params.size() " << params.size() << endl;
      
      // first entry in the param vector indicates whether this configuration
      //  was seen as the primary or secondary configuration. So - get rid 
      //  of the first entry and extract just the code transformation parameter
      //  and make an entry to the global database indicating we are done with
      //  code generation for this configuration.
      for(int jj=1; jj < params.size(); jj++)
	{
	  temp_params.push_back(params.at(jj));
	}
     
      global_data.insert(pair<string, int>(vector_to_string(temp_params),1));

      if(debug_mode)
	printf("size of the global database :: %d \n", global_data.size());
      // parent returns.
      return(pid);
    }

    // child portion::
    //  block all the signals.
    sigset_t new_mask;
    sigset_t old_mask;

    /* initialize the new signal mask */
    sigfillset(&new_mask);

    /* block all signals */
    sigprocmask(SIG_SETMASK, &new_mask, &old_mask);

    // child continues.
    generator_main(i, params, prim);
}

// this gets the code generation parameters from the code manager and fires
//  scripts to use the underlying code generation tool to generate the code.
//  Scripts for different code and different code-generation utility need to
//   be provided by the user.
void generator_main(int i,vector<int> params, int prim) {
  string path ("/fs/spoon/tiwari/signal/tmp/");

  stringstream ss;
  ss.str("");

  // set which machine to use
  // first check to see if there is an underscore in the machine name.
  string generator_name = generator_vec.at(i);

  // if we are launching multiple code-generation processes on a given
  //  machine, strip the underscore from the machine identifier to 
  //  get the actual name for ssh command.
  size_t found;
  found=generator_name.find("_");

  if(found != -1)
    {	
      generator_name=generator_name.erase(found);
    }	       
  
  // for now we are relying on the system command to launch a remote
  //  process. For cleaner implementation, libssh or libssh2 can be
  //  used as well.
  ss << "ssh  " << generator_name << " exec ";

  // full path to the script : default is:
  //   tmp/<machine-name>_<instance_num>
  ss << working_directory << "/tmp/" <<  generator_vec.at(i);
  
  ss << "/chill_script.sh " << prim;
  ss << vector_to_string(params);
  ss << generator_vec.at(i);
  if(debug_mode)
    printf("%s \n", ss.str().c_str());
  // fire up the ssh
  int sys_return = system(ss.str().c_str());
  if(debug_mode)
    cout << ss.str() << " " << gen_ptr[i].pid << "  " 
	 << generator_name << " sys_return " << sys_return << "\n";
  
  // error check not done yet. the process simply quits after the
  //  code generation is done.
  exit(EXIT_SUCCESS);
}


void initialize() {
 
  // copy over the necessary code-generation-utility specific files to the newly created
  //  directories. For CHiLL, this set generally involves a chill_script.sh (which drives
  //  the code generation) and SUIF intermediate files.
  //  We REQUIRE users to provide 'file.list' file, where each line is the name of a 
  //   unique file residing in the working directory.

  // get the names of the files from the file.list file
  
  vector<string> file_list;
  get_file_list(file_list, "file.list");

  // current working directory for the code generation manager
  getcwd(working_directory, MAXPATHLEN);
  if(debug_mode)
    printf("working directory -> %s\n", working_directory);
  
  // check for tmp directory: all code generated will be 
  struct stat dir_tmp;
  char tmp_dir_path[MAXPATHLEN];
  char from_file[MAXPATHLEN];
  char to_file[MAXPATHLEN];

  sprintf(tmp_dir_path,"%s/tmp",working_directory);
  //if(debug_mode)
  //  printf("Creating the tmp directory: %s \n", tmp_dir_path);
  
  // Remove old directory here?
  if(stat(tmp_dir_path, &dir_tmp) == 0) {
    if(debug_mode)
      printf("Removing the directory \n");
    sprintf(from_file, "rm -rf %s", tmp_dir_path);
    if(debug_mode)
      printf("rm command : %s \n", from_file);
    system(from_file);
  }

  int result=mkdir(tmp_dir_path,0755);
  if(debug_mode)
    printf("Creating the tmp directory: %d \n", result);
  if(result!=0) {
    perror("cannot create tmp directory in the current working directory\n");
    exit(EXIT_FAILURE);
  }
  

  /*
   * To keep things simple, we go ahead and make an individual directory 
   *  for each machine that will be participating in the code generation.
   */
  for(int i=0; i<generator_vec.size(); i++){
    sprintf(tmp_dir_path,"%s/tmp/%s",working_directory,(generator_vec.at(i)).c_str());
    if(debug_mode)
      printf("creating directory: %s \n", tmp_dir_path);
    
    result=mkdir(tmp_dir_path,0755);

    if(result!=0) {
      perror("cannot create machine-specific directory in the current working directory\n");
      exit(EXIT_FAILURE);
    }

    // copy required files to the newly created directories
    // for example, for CHiLL, these will be the SUIF intermediate files.
    //  and any other files that the compilation might need.
    for(int j=0; j<file_list.size(); j++) {
      sprintf(from_file, "%s/%s", working_directory,
	      (file_list.at(j)).c_str());
      sprintf(to_file, "%s/tmp/%s/%s", working_directory, 
	      (generator_vec.at(i)).c_str(), (file_list.at(j)).c_str());
      if(debug_mode)
	printf("Copying %s to %s \n", from_file, to_file);
      copy_file(from_file, to_file);
      // change the permisssions
      chmod(to_file, S_IRWXU);
    }
  }
  // three additional directories: new_code, zipped, and transport
  sprintf(tmp_dir_path,"%s/tmp/new_code",working_directory);
  result=mkdir(tmp_dir_path,0755);

  if(result!=0) {
    perror("cannot create machine-specific directory in the current working directory\n");
    exit(EXIT_FAILURE);
  }
  sprintf(tmp_dir_path,"%s/tmp/zipped",working_directory);
  result=mkdir(tmp_dir_path,0755);
  
  if(result!=0) {
    perror("cannot create machine-specific directory in the current working directory\n");
    exit(EXIT_FAILURE);
  }
   sprintf(tmp_dir_path,"%s/tmp/transport",working_directory);
  result=mkdir(tmp_dir_path,0755);

  if(result!=0) {
    perror("cannot create machine-specific directory in the current working directory\n");
    exit(EXIT_FAILURE);
  }

}

/*
 * This is the code generation manager. This waits for SIGUSR1 signal from the code-server
 *  and starts the code generation. 
*/
void generator_manager_main(int server_pid_ ) {
  

  server_pid=server_pid_;
  // parent's pid
  int ppid = getpid();
  int result, pid;
  int status, errno;
  int allpids[2];
  stringstream ss, ss2, log_message;

  // signal stuff
  signal(SIGUSR1,sigusr1);
  sigset_t mask, oldmask;
  sigemptyset (&mask); 
  sigaddset (&mask, SIGUSR1);

  /* 
   *  first get the list of available machines.
   *  this is the user-provided list of the machines available for
   *  code generation. 
   *   
   *  ASSUMPTION: NOTE THIS CAREFULLY.
   *   We assume that the same file-system is mounted on all the machines
   *    that are used to generate code. This is not a very hard assumption
   *    and usually holds true.
   *
   *   Format of 'generator_hosts' file should be the following:
   *    <machine_name> <number_of_instances>
   *     where machine_name is the name of the machine (for example, spoon, driver etc)
   *     and number_of_instances is the number of code generation processes to launch
   *     on that machine.
   */

  string filename = "generator_hosts";
  get_code_generators(generator_vec, filename);
  initialize();
  if(debug_mode) {
    printf("The list of available machines: ");
  }
  
  int nchildren = generator_vec.size();
  gen_ptr = (generator*)calloc(nchildren, sizeof(generator));
  //gen_host_ptr=(generator_host*)calloc(nchildren, sizeof(generator_host));
  
  if(debug_mode) {
    for(int i=0; i < generator_vec.size(); i++)
      {
	cout << generator_vec.at(i) << " ";
      }	
    cout << "\n";
  }
 
  vec_of_intvec code_parameters;
  vec_of_intvec all_params;

  // main loop starts here
  vector<int> parameter_size;


  /* Wait for a signal SIGUSR1 to arrive. */

  sigprocmask (SIG_BLOCK, &mask, &oldmask);

  // signal handler for SIGUSR1 sets usr_interrupt to 1.
  while (!usr_interrupt)
    sigsuspend (&oldmask);
  sigprocmask (SIG_UNBLOCK, &mask, NULL);

  // revert
  usr_interrupt=0;
  
  while(true) {
 
    ss.str("");


    // read in the primary configurations from the shared memory
    char * values_in_primary=read_from_primary((struct primary_buf *)primary_confs,1);
    
    if(debug_mode)
      printf("values in primary: %s \n", values_in_primary);

    while(*values_in_primary!='1')
      {
	sleep(3);
	values_in_primary=read_from_primary((struct primary_buf *)primary_confs,1);
	printf("values in primary: %s \n", values_in_primary);
      }
    // we got a new set of primary configurations

    if(debug_mode)
      printf(" The primary buffer is new. \n");

    
    /* parse the value 
     * the string is assumed be in the following format
     *	    point_1:point_2: ... : point_n
     *		where point_k is in the following format:
     *		value_dim_1 value_dim_2 ... value_dim_n, assuming we have n
     *		dimensionsal search space
    */
    values_in_primary=read_from_primary((struct primary_buf *)primary_confs,0);
    parse_configurations(all_params,string(values_in_primary));
    
    // populate the global database and remove the confs
    //  that we have seen already.
    // Note that transport_data holds information about what code have been
    //  transported to the destination machine.
    populate_remove(all_params, code_parameters,global_data, transport_data, 1);
    

    cout << "Code Transformation Parameters: size: " << code_parameters.size() << " \n";

    parameter_size.push_back(code_parameters.size());

    // generate the code for the primary
    for (int i = 0; i < code_parameters.size(); i++)
      {
	nchildren=generator_vec.size();
	if (i  < nchildren) {
	  pid = generator_make(i, code_parameters.at(i),1);
	  kill(pid, SIGUSR1);
	} else {
	  // if we have used all the resources at least once, we have to wait until someone
	  //   becomes free
	  int status;
	  int pid_done = waitpid(-1, &status,0);
	  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
	    cerr << "Process " << i << " (pid " << pid_done << ") failed" << endl;
	    exit(EXIT_FAILURE);
	  } else {
	    int ii;
	    for (ii = 0; ii < nchildren; ++ii) {
	      if(pid_done == gen_ptr[ii].pid) {
		gen_ptr[ii].avail=1;
		break;
	      }
	    }
	    pid = generator_make(ii, code_parameters.at(i),1);
	  }
	}
      }
    /* 
     * wait for all children to finish
    */
    
    for(int i = 0; i < nchildren; i++) {
      (waitpid(gen_ptr[i].pid, &status, 0));
    }
    
    /* transport the code to the destination system/folder 
     * for now, we use a simple method to do this : a bash
     * script, which takes candidate simplex as an argument
     * along with the destination machine's host address and
     * the desitnation folder location.
     *  For later versions, this can be replaced with calls
     *   to the libssh2 API.
    */
    
    // determine what files need to be transported over to the target
    //  machine
    


    // copy the zipped files to the target machine
    //  unzip the files on the target machine
    ss.str("");
    ss << working_directory << "/transport_file.sh";
    int sys_return = system(ss.str().c_str());    
    
    // first indicate that this round is complete
    char empty[1];
    empty[0]='O';
    write_to_primary_buffer(empty,0);
    
    // secondary configurations
    code_parameters.clear();
    all_params.clear();

    if(debug_mode)
      printf("After clearing: code_parameters.size() is %d and global_data.size() is %d \n",
	     code_parameters.size(), transport_data.size());
    if(debug_mode) {
      map<string, int>::iterator pos;

      for(pos = transport_data.begin(); pos != transport_data.end(); ++pos)
	{
	  cout << "transport Key: " << pos->first << endl;
	  cout << "Value:" << pos->second << endl; 

	}
    }
    char * values_in_secondary=read_from_secondary((struct secondary_buf *)secondary_confs);
    parse_configurations(all_params,string(values_in_secondary));
    if(debug_mode)
      printf("values in secondary: %s \n", values_in_secondary);
    populate_remove(all_params, code_parameters,global_data, transport_data,0);

    
    /* usr_interrupt flag is turned on when we receive a SIGUSR1 signal indicating that
     *  there is a new set of primary configurations. Keep generating code for secondary
     *  configurations, until we receive the signal.
     */ 
    cout << "Code Transformation Parameters (Secondary): size: " << code_parameters.size() << " \n";

    parameter_size.push_back(code_parameters.size());

    // generate the code for the primary
    for (int i = 0; i < code_parameters.size(); i++)
      {
	if(usr_interrupt) 
	  {
	    usr_interrupt=0;
	    break;
	  }
	  
	nchildren=generator_vec.size();
	if (i  < nchildren) {
	  pid = generator_make(i, code_parameters.at(i),0);
	  kill(pid, SIGUSR1);
	} else {
	  // if we have used all the resources at least once, we have to wait until someone
	  //   becomes free
	  int status;
	  int pid_done = waitpid(-1, &status,0);
	  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
	    cerr << "Process " << i << " (pid " << pid_done << ") failed" << endl;
	    exit(EXIT_FAILURE);
	  } else {
	    int ii;
	    for (ii = 0; ii < nchildren; ++ii) {
	      if(pid_done == gen_ptr[ii].pid) {
		gen_ptr[ii].avail=1;
		break;
	      }
	    }
	    pid = generator_make(ii, code_parameters.at(i),0);
	  }
	}
      }
    /* 
     * wait for all children to finish. This is potentially dangerous, if
     *  there are configurations for which the code generator takes substantially
     *  long time to generate and compile the code. Further testing needed here.
    */
    
    for(int i = 0; i < nchildren; i++) {
      (waitpid(gen_ptr[i].pid, &status, 0));
    }
    
    // need to add this capability
    /*
    if(file_exists(ss2.str().c_str())){
      timestep=1;
      global_data.clear();
      for(int iii=0; iii < parameter_size.size(); iii++)
	{
	  printf("%d, ", parameter_size[iii]);
	  
	}	
    }
    */
    code_parameters.clear();
    all_params.clear();
    printf("all done \n");
    
  } // mainloop
}	
