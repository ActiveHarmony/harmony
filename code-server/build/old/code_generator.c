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
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

#include "StringTokenizer.h"
using namespace std;

int nchildren = 1;
int timestep = 1;

map<string,int> global_data;
map<string,int>::iterator it;
typedef pair <int, int> Int_Pair;

// generator typedef
typedef struct {
    vector<int> parameters;
    int avail;
    //string hostname;
    int rand_amount;
    int pid;
} generator;

// declarations
typedef vector<vector<int> > vec_of_intvec;
void process_point_line(string line, vector<int>& one_point);
void get_parameters_from_file(vector<vec_of_intvec>& intvec, string filename);
void get_code_generators (vector<string>& vec, string filename);
void get_points_vector (string point, vec_of_intvec& code_parameters);
string vector_to_string(vector<int> v);
string vector_to_string_demo(vector<int> v);
void get_parameters_from_file(vec_of_intvec& intvec, string filename);
vector<string> generator_vec;
int file_exists (const char * fileName);
void populate_remove(vec_of_intvec all_params,vec_of_intvec& code_parameters);
void print_vec(vector<int>& a);

string log_file("code_generation.log");
void logger (string message);

generator *gen_ptr;


pid_t generator_make(int i, vector<int> params)
{

    pid_t pid;
    printf("GEN MAKE I: %d \n", i);

    void generator_main(int, vector<int>);
    //int temp = rand()%6;
    if((pid = fork()) > 0) {
        gen_ptr[i].parameters=params;
        gen_ptr[i].avail=1;
        //gen_ptr[i].rand_amount=temp;
        gen_ptr[i].pid=pid;
        return(pid); // parent returns
    }
    generator_main(i, params); // child continues
}
void generator_main(int i,vector<int> params) {
  string path ("/fs/spoon/tiwari/matvec_code_generator/hosts/");

    // this is where the code generation happens
    //  make a call to chill_script.sh
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

void generator_main1(int i,vector<int> params) {
  string path ("/fs/spoon/tiwari/redblack_code_generator/hosts/");

    // this is where the code generation happens
    //  make a call to chill_script.sh
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
     //ss << "ssh  " << generator_name << " exec ";
     ss << "exec ";
     //ss << "ssh  " << generator_vec.at(i) << " exec ";
     // full path to the script
     ss << path <<  generator_vec.at(i);

     ss << "/chill_script.sh ";
     ss << vector_to_string(params);
     //ss << vector_to_string(gen_ptr[i].parameters);
     ss << generator_vec.at(i) << " " << i;
     //cout << ss.str() << " " ;
    
     int sys_return = system(ss.str().c_str());
     cout << ss.str() << " " << gen_ptr[i].pid << "  " << generator_name << " sys_return " << sys_return << "\n";

     
     // error check not done yet.
    exit(0);
}

void generator_main_(int i,vector<int> params) {

    string path ("/fs/spoon/tiwari/redblack_code_generator/hosts/");
    // this is where the code generation happens
    //  make a call to chill_script.sh
     stringstream ss;
     ss.str("");
     // set which machine to use
     ss << "ssh  " << generator_vec.at(i) << " exec ";
     // full path to the script
     ss << path <<  generator_vec.at(i);

     ss << "/chill_script.sh ";
     ss << vector_to_string(params);
     //ss << vector_to_string(gen_ptr[i].parameters);
     ss << generator_vec.at(i);

     cout << ss.str() << "\n";

     int sys_return = system(ss.str().c_str());
     // error check not done yet.
    exit(0);
}

int main(int argc, char **argv)
{
    int ppid = getpid();
    int parent=0, child=0, result, pid;

    int status, errno;

    int allpids[2];
    stringstream ss, ss2, log_message;
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
        populate_remove(all_params, code_parameters);


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
/*
  Helpers
*/

void print_vec(vector<int>& a)
{
    for(int i=0; i<a.size(); ++i)
        cout << a[i] << " ";
    cout << endl;
    cout << "----------------"<<endl;
}//print

void
populate_remove(vec_of_intvec all_params, vec_of_intvec& code_parameters)
{

    for(int i=0; i < all_params.size(); i++)
    {
        vector<int> temp = all_params.at(i);
        stringstream ss;
        ss << " ";
        for (int i = 0; i < temp.size(); i++)
        {
            ss << temp.at(i) << " ";
        }
        //string str=vector_to_string(temp);

        //cout << "string from vector_to_string " << ss.str().c_str() << "\n";
        pair< map<string,int>::iterator, bool > pr;
        pr=global_data.insert(pair<string, int>(ss.str(), 1));
        ss.str("");
        if(pr.second)
        {
            code_parameters.push_back(temp);
        }
    }
}

void
logger (string message)
{
    string line;
    ofstream out_file;
    out_file.open(log_file.c_str(),ios::app);
    if(!out_file) 
    {
        cerr << "Error file could not be opened \n";
        exit(1);
    }
    
    out_file << message;
    out_file.flush();
    out_file.close();
    

}


void
get_code_generators (vector<string>& vec, string filename)
{
    int num_lines=0;
    string line;
    ifstream myfile;

    myfile.open(filename.c_str());
    if (myfile.is_open()) {
        //while (! myfile.eof() ) {
        while(getline (myfile,line)) {
            //getline (myfile,line);
            if(line.length() == 0) break;
            vec.push_back(line);
            num_lines++;
        }
        myfile.close();
        //cout << "NUM_LINES READ: " << num_lines;
    }
    else cout << "Unable to open file";
    //return num_lines;
}

void
get_parameters_from_file(vec_of_intvec& intvec, string filename)
{
    int num_lines=0;
    string line;
    ifstream myfile;

    myfile.open(filename.c_str());
    if (myfile.is_open()) {
        while (! myfile.eof()) {
            getline (myfile,line);
            //cout << "string " << line << "\n";
            if(line.length() == 0) break;
            vector<int> one_point;
            process_point_line(line, one_point);
            intvec.push_back(one_point);
            num_lines++;
        }
        myfile.close();
        //cout << "NUM_LINES READ: " << num_lines;
    }
}


/// THIS IS WHERE WE ONLY TAKE THE LAST 4 parameters for redblack
void
process_point_line(string line, vector<int>& one_point)
{
    StringTokenizer strtok_space (line, " ");
    //strtok_space.nextIntToken();
    //strtok_space.nextIntToken();
    while(strtok_space.hasMoreTokens())
    {
      // skip the first two
      
        int temp = strtok_space.nextIntToken();
        //cout << temp << ", ";
        one_point.push_back(temp);
    }

}


string
vector_to_string_demo(vector<int> v)
{
    char *vars[] = {"TI","TJ","TK", "UI","US"};

    stringstream ss;
    ss << " ";
    for (int i = 0; i < v.size(); i++)
    {
        ss << vars[i] << "=" << v.at(i) << " ";
    }
    return ss.str();
}

string
vector_to_string(vector<int> v)
{
    stringstream ss;
    ss << " ";
    for (int i = 0; i < v.size(); i++)
    {
        ss << v.at(i) << " ";
    }
    return ss.str();
}

int
file_exists (const char * fileName)
{
    struct stat buf;
    int i = stat ( fileName, &buf );
    int size = buf.st_size;
    /* File found */
    if ( i == 0 && size > 0)
    {
        return 1;
    }
    return 0;

}


// given a file name, this gives the total number of lines in the file.
int
get_line_count(string filename)
{
    int num_lines=0;
    string line;
    ifstream myfile;

    myfile.open(filename.c_str());
    if (myfile.is_open()) {
        while (! myfile.eof()) {
            getline (myfile,line);
            num_lines++;
        }
        myfile.close();
    }
    else cout << "Unable to open file";
    return num_lines;
}

void
get_points_vector (string point, vec_of_intvec& code_parameters)
{

    StringTokenizer strtok_colon (point, ":");


    vector<int> temp_vector;
    while (strtok_colon.hasMoreTokens())
    {
        StringTokenizer strtok_space (strtok_colon.nextToken(), " ");
        while(strtok_space.hasMoreTokens())
        {
            int temp = strtok_space.nextIntToken();
            //cout << temp << ", ";
            temp_vector.push_back(temp);
        }
        code_parameters.push_back(temp_vector);
        temp_vector.clear();

    }
}
