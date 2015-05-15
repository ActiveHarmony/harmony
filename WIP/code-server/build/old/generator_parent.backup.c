/*
 * Copyright 2003-2015 Jeffrey K. Hollingsworth
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

map<string,int> global_data;
map<string,int>::iterator it;


generator *gen_ptr;
int nchildren = 1;
int timestep = 1;



static int debug_mode=0;

int server_pid=0;

pid_t generator_make(int i, vector<int> params)
{

    pid_t pid;
    printf("GEN MAKE I: %d \n", i);

    void generator_main(int, vector<int>);
 
    if((pid = fork()) > 0) {
        gen_ptr[i].parameters=params;
        gen_ptr[i].avail=1;
        gen_ptr[i].pid=pid;
        return(pid); // parent returns
    }
    generator_main(i, params); // child continues
}

void generator_main(int i,vector<int> params) {
  string path ("/fs/spoon/tiwari/matvec_code_generator/hosts/");

  // this is where the actual code generation happens
  //   make a call to chill_script.sh
  //   or appropriate code generation utility script
  stringstream ss;
  ss.str("");
  // set which machine to use
  // first check to see if there is an underscore in the machine name.
  string generator_name = generator_vec.at(i);
  size_t found;
  found=generator_name.find("_");
  //printf("did we find the underscore? %d \n", found);
  if(found != -1)
    {
      //printf("inside the if?\n");
      generator_name=generator_name.erase(found);
      //printf("%s \n", generator_name.c_str());
    }	
  ss << "ssh  " << generator_name << " exec ";
  //ss << "ssh  " << generator_vec.at(i) << " exec ";
  // full path to the script
  ss << path <<  generator_vec.at(i);
  
  ss << "/chill_script.sh ";
  ss << vector_to_string(params);
  //ss << vector_to_string(gen_ptr[i].parameters);
  ss << generator_vec.at(i);
  //cout << ss.str() << " " ;
  
  int sys_return = system(ss.str().c_str());
  cout << ss.str() << " " << gen_ptr[i].pid << "  " << generator_name << " sys_return " << sys_return << "\n";
  
  
  // error check not done yet.
  exit(0);
}
int usr_interrupt=0;

void initialize() {
  
}



void child_main(int server_pid_) {


  server_pid=server_pid_;
  // parent's pid
  int ppid = getpid();
  int result, pid;
  int status, errno;
  int allpids[2];
  stringstream ss, ss2, log_message;

  /*
  sigset_t mask, oldmask;


  signal(SIGHUP,sighup);
  signal(SIGINT,sigint);
  signal(SIGQUIT, sigquit);
  */
  /* Set up the mask of signals to temporarily block. */
  /*
  sigemptyset (&mask);
  sigaddset (&mask, SIGINT);
  */
  


  // first get the list of available machines
  string filename = "generator_hosts_processed";
  get_code_generators(generator_vec, filename);
  
  cout << "The list of available machines: ";
  //log_message << "The list of available machines: ";
  
  int nchildren = generator_vec.size();
  gen_ptr = (generator*)calloc(nchildren, sizeof(generator));
  
  for(int i=0; i < generator_vec.size(); i++)
    {
      cout << generator_vec.at(i) << " ";
      //log_message << generator_vec.at(i) << " ";
    }	
    //log_message << "\n";
  cout << "\n";
 
  vec_of_intvec code_parameters;
  vec_of_intvec all_params;
  // main loop starts here
  
  // update the log file
  //logger(log_message.str());
  //log_message.str("");
  
  vector<int> parameter_size;
  
  while(true) {
    
    // read the parameters from parameter.timestep.dat file
    ss.str("");
    //log_message.str("");
    // the file to watch
    ss << "confs/candidate_simplex." << timestep << ".dat";
    
    /* Wait for a signal to arrive. */
    //pause();
    /*
     sigprocmask (SIG_BLOCK, &mask, &oldmask);
     while (!usr_interrupt)
       sigsuspend (&oldmask);
     sigprocmask (SIG_UNBLOCK, &mask, NULL);

     printf("sending the server a signal in return \n");
     //kill(server_pid, SIGHUP);
     */

    //log_message << "waiting to hear from harmony server ... \n";
    cout << ss.str() << "\n";
    while(!file_exists(ss.str().c_str())){
      sleep(1);
    }
    
    filename = ss.str();
    cout << "filename: " << filename << "\n";
    
    get_parameters_from_file(all_params, filename);
    /*
      for(int jj = 0; jj < all_params.size(); jj++)
      {
      print_vec(all_params.at(jj));
      }
    */
    
    // populate the global database and remove the confs
    // that we have seen already
    populate_remove(all_params, code_parameters,global_data);
    
    
    //get_points_vector(argv[1], code_parameters);
    //cout << "Command Line arguments: " << argv[1] << "\n";
    
    
    
    cout << "Code Transformation Parameters: size: " << code_parameters.size() << " \n";
    //log_message << "Code Transformation Parameters received for iteration: " << timestep << "\n";
    /*
      for(int i=0; i < code_parameters.size(); i++) {
      cout << "[";
      vector<int> temp_vector = code_parameters.at(i);
      for (int j=0; j < temp_vector.size(); j++) {
      cout << temp_vector.at(j) << " ";
      }
      cout << "] ";
      }
      cout << "\n";
    */

    parameter_size.push_back(code_parameters.size());
    for (int i = 0; i < code_parameters.size(); i++)
      {
	printf("code_parameters.size: %d i::::: %d nchildren:::: %d ::::::::: \n \n", 
	       code_parameters.size(),i, nchildren);
	nchildren=generator_vec.size();
	if (i  < nchildren) {
	  printf("Should never get here once i is greater than 8 \n \n");
	  pid = generator_make(i, code_parameters.at(i));
	  //allpids[i]=pid;
	} else {
	  // if we have used all the resources at least once, we have to wait until someone
	  //   becomes free
	  int status;
	  int pid_done = waitpid(-1, &status,0);
	  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
	    cerr << "Process " << i << " (pid " << pid_done << ") failed" << endl;
	    // handle this error separately
	    exit(1);
	  } else {
	    int ii;
	    for (ii = 0; ii < nchildren; ++ii) {
	      if(pid_done == gen_ptr[ii].pid) {
		gen_ptr[ii].avail=1;
		break;
	      }
	    }
	    pid = generator_make(ii, code_parameters.at(i));
	    //allpids[i]=pid;
	  }
	}
      }	
    
    /* finally wait for all children to finish
       write out the indicator file (for now, this
       should suffice, need to change this later).
    */
    
    // first retrieve the pids for the ones that could be
    //  alive at this point of the code.
    
    for(int i = 0; i < nchildren; i++) {
      (waitpid(gen_ptr[i].pid, &status, 0));
    }
    
    printf("NCHIDLREN::::: %d \n", nchildren);
    /* look for the existence of transform.timestep.dat file
       this is a hack, will change it to something more robust
       and more importantly efficient later.
    */
    
    // delete the confs for which we have generated code already
    ss.str("");
    ss << "rm -rf confs/candidate_simplex." << timestep << ".dat";
    cout << ss.str() << "\n";
    system(ss.str().c_str());
    
    
    // copy the zipped files from spoon to carver
    ss.str("");
    ss << "/fs/spoon/tiwari/matvec_code_generator/copy_file.sh";
    int sys_return = system(ss.str().c_str());
    
    // now unzip the file
    ss.str("");
    ss << "ssh tiwari@brood00 /hivehomes/tiwari/offline_tuning/trisolve/unzip_new_code.sh";
    sys_return = system(ss.str().c_str());
    cout << " new code unzipped";
    
    
    // first indicate that this round is complete
    ss.str("");
    ss << "touch /fs/spoon/tiwari/matvec_code_generator/code_flags/code_complete." << timestep;
    system(ss.str().c_str());
    
    ss.str("");
    ss << "scp /fs/spoon/tiwari/matvec_code_generator/code_flags/code_complete." << timestep 
       << " tiwari@brood00:~/SC10/redblack/harmony/bin/code_flags/";
    system(ss.str().c_str());
    
    
    
    // clear the stream
    ss.str("");
    // increment the timestep
    timestep++;
    // the file to watch
    ss << "confs/candidate_simplex." << timestep << ".dat";
    ss2.str("");
    ss2 << "confs/candidate_simplex.1.dat";
    
    cout << ss.str() << "\n";
    while(!file_exists(ss.str().c_str()) && !file_exists(ss2.str().c_str())){
      sleep(1);
    }
    
    if(file_exists(ss2.str().c_str())){
      timestep=1;
      global_data.clear();
      for(int iii=0; iii < parameter_size.size(); iii++)
	{
	  printf("%d, ", parameter_size[iii]);
	  
	}	
      //ss.str("");
      //ss << "rm -rf confs/candidate_simplex.*"  << ".dat";
      // cout << ss.str() << "\n";
      // system(ss.str().c_str());
    }
    code_parameters.clear();
    all_params.clear();
    printf("all done \n");
    
  } // mainloop
}	
