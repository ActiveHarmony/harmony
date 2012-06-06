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
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
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
#include <set>

#include "Tokenizer.h"
#include "code_generator.h"
#include "hconfig.h"

using namespace std;

/*
 * Configuration from Harmony Server.
 */
cfg_t *cfg = NULL;

typedef vector<vector<int> > vec_of_intvec;

// generator typedef
typedef struct {
    vector<int> parameters;
    int avail;
    int rand_amount;
    int pid;
} generator;

// declarations
double time_stamp()
{
  struct timeval t;
  double time;
  gettimeofday(&t, NULL);
  time = t.tv_sec + 1.0e-6*t.tv_usec;
  return time;
}

void generator_main(int, vector<int>);
int codeserver_init(string &);
void process_point_line(string line, vector<int>& one_point);
int parse_slave_list(vector<string> &vec, const char *hostlist);
void get_points_vector (string point, vec_of_intvec& code_parameters);
string vector_to_string(vector<int> v);
string vector_to_bash_array_local(vector<int> v);
string vector_to_bash_array_remote(vector<int> v);
string vector_to_string_demo(vector<int> v);
void get_parameters_from_file(vec_of_intvec &intvec, string filename);
int file_exists (const char * fileName);
void populate_remove(vec_of_intvec &all_params, vec_of_intvec &code_params);
void print_vec(vector<int>& a);
void logger (string message);
int touch_remote_file(string filename, string &host);

/*
 * Global Variable Declaration
 */
static int nchildren = 1;
static int timestep = 0;
static generator *gen_ptr = NULL;
static vector<string> slave_vec;
/* Configuration values passed in from the harmony server. */
static string appname, conf_host, conf_dir, code_host,
    code_dir, flag_host, flag_dir, slave_dir;
/* global_data tracks all configurations we have generated code for so far. */
static map<string,int> global_data;

/*
 * generators: These are the real work-horses. For each new configuration, we 
 *  fork a new process to generate code. These processes die after the code
 *  generation is complete. We block all the signals because we do not want to
 *  interrupt any code generation activity.
*/

pid_t generator_make(int i, vector<int> params)
{
    pid_t pid;

    if ( (pid = fork()) > 0) {
        gen_ptr[i].parameters=params;
        gen_ptr[i].avail=1;
        gen_ptr[i].pid=pid;
        return(pid); // parent returns
    }
    generator_main(i, params); // child continues
}

// this gets the code generation parameters from the code manager and fires
//  scripts to use the underlying code generation tool to generate the code.
//  Scripts for different code and different code-generation utility need to
//   be provided by the user.

void generator_main(int i, vector<int> params)
{
    // this is where the code generation happens
    //  make a call to chill_script.appname.sh
    // Note that this appname has to match the appname specified in the client
    //  tcl file.

    stringstream ss;
    ss.str("");

    // set which machine to use
    // first check to see if there is an underscore in the machine name.
    string generator_name = slave_vec.at(i);
    size_t found;
    found = generator_name.find("_");
    if (found != -1)
    {
        generator_name=generator_name.erase(found);
    }

    /* Different machines might be configured differently. So a check here
     * should be made to make sure that the hostname matches uniformly across
     * generator_hosts file and hostname gathered here.
     */

    // find out if this is a remote host
    bool flag = (generator_name == conf_host);

    if (!flag) {
        // remote
        ss << "ssh " << generator_name << " ";
    }
    ss << "exec " << slave_dir << "/" << slave_vec.at(i)
       << "_" << appname << "/chill_script." << appname << ".sh ";

    if (flag) {
        ss << vector_to_bash_array_local(params);
    } else {
        ss << vector_to_bash_array_remote(params);
    }
    ss << generator_name << " "
       << slave_dir << "/" << slave_vec.at(i) << "_" << appname << " "
       << code_host << " "
       << code_dir;

    cout << "[CS]: Executing: " << ss.str() << endl;
    int sys_return = system(ss.str().c_str());
    cout << "[CS]: Returned: " << sys_return << endl;

    // error check not done yet.
    exit(0);
}

string log_file;
stringstream log_message;
int main(int argc, char **argv)
{
    stringstream ss;
    int pid, status;

    if (argc != 2) {
        cerr << "Usage: ./code_generator <codegen_path>\n";
        cerr << " Where <codegen_path> should match the path specified"
                " in the harmony server's configuration file.\n";
        exit(-1);
    }

    if (file_exists(argv[1]) != 2) {
        cerr << argv[1] << " is not a valid directory.  Exiting.\n";
        exit(-1);
    }
    conf_dir = argv[1];

    vec_of_intvec code_parameters;
    vec_of_intvec all_params;

    // main loop starts here
    // update the log file

    string init_filename = conf_dir + "/harmony.init";
    string next_filename;
    while (true) {
        /* Construct the next timestep filename. */
        ss.str("");
        ss << conf_dir << "/candidate_simplex." << appname
           << "." << timestep << ".dat";
        next_filename = ss.str();

        cout << "[CS]: Waiting to hear from harmony server..." << endl;
        log_message << "[CS]: Waiting to hear from harmony server...\n";
        while (!file_exists(init_filename.c_str()) &&
               !file_exists(next_filename.c_str())) {
            sleep (1);
        }

        if (file_exists(init_filename.c_str())) {
            cout << "[CS]: Harmony initialization file found." << endl;
            if (codeserver_init(init_filename) < 0) {
                cerr << "[CS]: Removing invalid configuration file.\n";

            } else {
                // record some data? How many variants produced in total?
                global_data.clear();
                timestep = 0;
                cout << "[CS]: Beginning new code server session." << endl;
            }
            std::remove(init_filename.c_str());
            continue;
        }

        cout << "[CS]: Filename: " << next_filename << endl;
        get_parameters_from_file(all_params, next_filename);

        // populate the global database and remove the confs
        // that we have seen already
        populate_remove(all_params, code_parameters);

        cout << "[CS]: Code Transformation Parameters: size: "
             << code_parameters.size() << "\n";
        log_message << "[CS]: Code Transformation Parameters for iteration: "
                    << timestep << "\n";
	log_message << "# of unique confs for this iteration: "
                    << code_parameters.size() << " \n";

	double time1__, time2__;
	time1__=time_stamp();
        for (int i = 0; i < code_parameters.size(); i++)
        {
            // check if we have used up all the available
            //  resources
            if (i < nchildren) {
                pid = generator_make(i, code_parameters.at(i));
                log_message << slave_vec.at(i) << " : "
                            << vector_to_string(code_parameters.at(i)) << "\n";
                logger(log_message.str());
                log_message.str("");

            } else {
                // if we have used all the resources at least once,
                // we have to wait until someone becomes free
                int pid_done = waitpid(-1, &status, 0);
                if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                    cerr << "[CS]: Process " << i
                         << " (pid " << pid_done << ") failed.\n";
                    // handle this error separately
                    exit(1);

                } else {
                    int ii;
                    for (ii = 0; ii < nchildren; ++ii) {
                        if (pid_done == gen_ptr[ii].pid) {
                            gen_ptr[ii].avail=1;
                            break;
                        }
                    }
                    pid = generator_make(ii, code_parameters.at(i));
                    log_message << slave_vec.at(ii) << " : "
                                << vector_to_string(code_parameters.at(i))
                                << "\n";
                    logger(log_message.str());
                    log_message.str("");
                }
            }
        }

        /* Finally, wait for all children to finish. (for now, this
           should suffice, need to change this later). */
        for (int i = 0; i < nchildren; i++) {
            waitpid(gen_ptr[i].pid, &status, 0);
        }
	time2__=time_stamp();
	double elapsed__=time2__-time1__;
	log_message << "[CS]: Total time for iteration "<< timestep
                    << " : " << elapsed__ << "\n------------------\n";
	logger(log_message.str());
	log_message.str("");

	/* Remove the conf file we just processed. */
	std::remove(next_filename.c_str());

        /* If the next code-timestep file exists at this point,
         * it must be stale.  Remove it.
         */
        ss.str("");
        ss << conf_dir << "/candidate_simplex." << appname << "." 
           << (timestep + 1) << ".dat";
        next_filename = ss.str();
        if (file_exists(next_filename.c_str())) {
            cout << "[CS]: Removing stale candidate simplex file.\n";
            std::remove(next_filename.c_str());
        }

        /* Inform the harmony server of newly generated code. */
	ss.str("");
	ss << flag_dir << "/code_complete." << appname << "." << timestep;
	touch_remote_file(ss.str(), flag_host);

	code_parameters.clear();
        all_params.clear();
        cout << "[CS]: Iteration complete." << endl;

        // increment the timestep
        timestep++;
    } // mainloop

    return 1;
}

int codeserver_init(string &filename)
{
    FILE *fp;
    stringstream ss;

    if (cfg)
        cfg_free(cfg);
    cfg = cfg_init();

    fp = fopen(filename.c_str(), "r");
    if (!fp) {
        fprintf(stderr, "Could not open config file %s\n", filename.c_str());
        return -1;
    }
    cfg_load(cfg, fp);  /* Errors during this call will show up
                           when we check code_dir and session. */

    /* Configuration variable assignment needs to be an atomic operation.
     * So, check that all variables exist, then assign them all.
     */
    const char *session = cfg_get(cfg, "harmony_session");
    if (session == NULL) {
        cerr << "[CS]: Error: harmony_session config directive missing.\n";
        return -1;
    }

    if (cfg_get(cfg, "app_name_tuning") == NULL) {
        cerr << "[CS]: Error: app_name_tuning config directive missing.\n"
             << "Please fix the harmony server's global config file.\n";
        return -1;
    }

    if (cfg_get(cfg, "codegen_host") == NULL) {
        cerr << "[CS]: Error: codegen_host config directive missing.\n"
             << "Please fix the harmony server's global config file.\n";
        return -1;
    }

    if (cfg_get(cfg, "codegen_path") == NULL) {
        cerr << "[CS]: Error: codegen_path config directive missing.\n"
             << "Please fix the harmony server's global config file.\n";
        return -1;
    }

    if (cfg_get(cfg, "codegen_code_host") == NULL) {
        cerr << "[CS]: Error: codegen_code_host config directive missing.\n"
             << "Please fix the harmony server's global config file.\n";
        return -1;
    }

    if (cfg_get(cfg, "codegen_flag_host") == NULL) {
        cerr << "[CS]: Error: codegen_flag_host config directive missing.\n"
             << "Please fix the harmony server's global config file.\n";
        return -1;
    }

    if (cfg_get(cfg, "codegen_flag_dir") == NULL) {
        cerr << "[CS]: Error: codegen_flag_dir config directive missing.\n"
             << "Please fix the harmony server's global config file.\n";
        return -1;
    }

    if (cfg_get(cfg, "codegen_slave_dir") == NULL) {
        cerr << "[CS]: Error: codegen_slave_dir config directive missing.\n"
             << "Please fix the harmony server's global config file.\n";
        return -1;
    }

    if (parse_slave_list(slave_vec, cfg_get(cfg, "codegen_slave_list")) < 0) {
        cerr << "[CS]: Error: codegen_slave_list config directive invalid.\n"
             << "Please fix the harmony server's global config file.\n";
        return -1;
    }
    nchildren = slave_vec.size();

    /* All configuration items are valid.  Begin atomic assignement. */
    appname = cfg_get(cfg, "app_name_tuning");
    conf_host = cfg_get(cfg, "codegen_host");
    code_host = cfg_get(cfg, "codegen_code_host");
    code_dir = cfg_get(cfg, "codegen_code_dir");
    flag_host = cfg_get(cfg, "codegen_flag_host");
    flag_dir = cfg_get(cfg, "codegen_flag_dir");
    slave_dir = cfg_get(cfg, "codegen_slave_dir");

    /* Initialize the application log file. */
    ss.str("");
    ss << "generation." << appname << ".log";
    log_file = ss.str();
    cout << "[CS]: Generating code for: " << appname << endl;

    cout << "[CS]: The list of available machines:" << endl;
    log_message << "[CS]: -------------------------------------------\n";
    log_message << "[CS]: The list of available machines: ";

    for(int i = 0; i < nchildren; ++i)
    {
        cout << slave_vec.at(i) << " ";
        log_message << slave_vec.at(i) << " ";
    }
    cout << "\n";
    log_message << "\n";

    logger(log_message.str());
    log_message.str("");

    /* Initialize generator structure array. */
    if (gen_ptr != NULL) {
        free(gen_ptr);
    }
    gen_ptr = (generator*)calloc(nchildren, sizeof(generator));

    /* Run the setup_code_gen_hosts.sh script. */
    ss.str("");
    ss << "/bin/sh setup_code_gen_hosts.sh " << appname <<  " "
       << slave_dir << " " << conf_host;
    for (int i = 0; i < nchildren; ++i) {
        ss << " " << slave_vec[i];
    }
    if (system(ss.str().c_str()) != 0) {
        cout << "[CS]: Error on system(" << ss.str() << ")" << endl;
        return -1;
    }

     /* If the first candidate configuration file exists at this point,
      * it must be stale. Remove it.
      */
    ss.str("");                                                               
    ss << conf_dir << "/candidate_simplex." << appname << ".0.dat";           
    string first_filename = ss.str();                                         
    if (file_exists(first_filename.c_str())) {                                
        cout << "[CS]: Removing stale candidate simplex file.\n";
        std::remove(first_filename.c_str());
    }

    /* Respond to the harmony server. */
    ss.str("");
    ss << flag_dir << "/codeserver.init." << session;
    touch_remote_file(ss.str(), code_host);
    cout << "[CS]: Created harmony response flag (" << ss.str() << ")" << endl;

    return 0;
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
populate_remove(vec_of_intvec &all_params, vec_of_intvec &code_params)
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
        
        pair< map<string,int>::iterator, bool > pr;
        pr=global_data.insert(pair<string, int>(ss.str(), 1));
        ss.str("");
        if(pr.second)
        {
            code_params.push_back(temp);
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

int
parse_slave_list(vector<string>& vec, const char *hostlist)
{
    vector<string> tmp;
    const char *end, *head, *tail, *host_ptr;
    char *num_ptr;
    string host;
    unsigned num;
    stringstream ss;

    if (hostlist == NULL)
        return -1;

    end = hostlist + strlen(hostlist);
    head = hostlist;
    while (head < end) {
        host = "";
        num = -1;

        /* Find the entry boundary. */
        tail = (char *)memchr(head, ',', end - head);
        if (!tail) {
            tail = end;
        }

        /* Skip leading whitespace. */
        while (head < tail && (head == '\0' || isspace(*head))) {
            ++head;
        }
        host_ptr = head;

        /* Find host boundary whitespace. */
        while (head < tail && (head != '\0' && !isspace(*head))) {
            ++head;
        }
        host = string(host_ptr, head++);

        /* Find the unsigned integer after the host. */
        errno = 0;
        num = strtoul(head, &num_ptr, 0);
        if (errno != 0) {
            num = -1;
            head = tail;
        } else {
            head = num_ptr;
        }

        /* Skip trailing whitespace. */
        while (head < tail && (head == '\0' || isspace(*head))) {
            ++head;
        }

        /* Error check */
        if (host.empty() || num == -1 || head != tail) {
            cerr << "[CS]: Error parsing slave host list ("
                 << hostlist << ")\n";
            return -1;
        }

        for (int i = 1; i <= num; ++i) {
            ss.str("");
            ss << host << "_" << i;
            tmp.push_back(ss.str());
        }
        ++head;
    }

    /* Only modify vec if we know the input to be error free. */
    vec = tmp;
    return 0;
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


void
process_point_line(string line, vector<int>& one_point)
{
    Tokenizer strtok_space;
    strtok_space.set(line," ");
    string temp=strtok_space.next();
    while(temp!= "")
    {
        int temp_int = atoi(temp.c_str());
        //cout << temp << ", ";
        one_point.push_back(temp_int);
        temp=strtok_space.next();
    }

}

string
vector_to_string_demo(vector<int> v)
{
    const char *vars[] = {"TI","TJ","TK", "UI","US"};

    stringstream ss;
    ss << " ";
    for (int i = 0; i < v.size(); i++)
    {
        ss << vars[i] << "=" << v.at(i) << " ";
    }
    return ss.str();
}

string
vector_to_bash_array_remote(vector<int> v)
{
    stringstream ss;
    ss << "\\\"";
    for (int i = 0; i < v.size(); i++)
    {
        ss << v.at(i) << " ";
    }
    ss << "\\\" ";
    return ss.str();
}
string
vector_to_bash_array_local(vector<int> v)
{
    stringstream ss;
    ss << "\"";
    for (int i = 0; i < v.size(); i++)
    {
        ss << v.at(i) << " ";
    }
    ss << "\" ";
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
file_exists (const char *fileName)
{
    struct stat buf;
    if (fileName == NULL) {
        return 0;
    }

    int i = stat ( fileName, &buf );
    if (i != 0) {
        return 0;

    } else if (S_ISREG(buf.st_mode) && buf.st_size > 0) {
        return 1;

    } else if (S_ISDIR(buf.st_mode)) {
        return 2;
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

    Tokenizer strtok_colon;
    strtok_colon.set(point, ":");
    string colon_token = strtok_colon.next();

    vector<int> temp_vector;
    while (colon_token!="")
    {
        Tokenizer strtok_space;
        strtok_space.set(colon_token," ");
        string space_token=strtok_space.next();
        while(space_token!="")
        {
            int temp = atoi(space_token.c_str());
            //cout << temp << ", ";
            temp_vector.push_back(temp);
            space_token=strtok_space.next();
        }
        code_parameters.push_back(temp_vector);
        temp_vector.clear();
        colon_token=strtok_colon.next();
    }
}

int touch_remote_file(string filename, string &host)
{
    stringstream ss;
    ss << "ssh " << host << " touch " << filename;
    return system(ss.str().c_str());
}
