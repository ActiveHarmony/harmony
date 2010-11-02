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

int nchildren = 4;
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

    void generator_main(int, vector<int>);
    int temp = rand()%6;
    if((pid = fork()) > 0) {
        gen_ptr[i].parameters=params;
        gen_ptr[i].avail=1;
        gen_ptr[i].rand_amount=temp;
        gen_ptr[i].pid=pid;
        return(pid); // parent returns
    }
    generator_main(i, params); // child continues
}

void generator_main(int i,vector<int> params) {

    string path ("/hivehomes/tiwari/SC09-demo/code_repository/hosts/");
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
    stringstream ss, log_message;
    int nchildren = 4;
    gen_ptr = (generator*)calloc(nchildren, sizeof(generator));


    // first get the list of available machines
    string filename = "generator_hosts";
    get_code_generators(generator_vec, filename);

    cout << "The list of available machines: ";
    log_message << "The list of available machines: ";

    for(int i=0; i < generator_vec.size(); i++)
    {
        cout << generator_vec.at(i) << " ";
        log_message << generator_vec.at(i) << " ";
    }
    log_message << "\n";
    cout << "\n";

    vec_of_intvec code_parameters;
    vec_of_intvec all_params;
    // main loop starts here

    // update the log file
    logger(log_message.str());
    log_message.str("");

    while(true) {

        // read the parameters from parameter.timestep.dat file
        ss.str("");
        log_message.str("");
        // the file to watch
        ss << "confs/candidate_simplex." << timestep << ".dat";

        log_message << "waiting to hear from harmony server ... \n";
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
        log_message << "Code Transformation Parameters received for iteration: " << timestep << "\n";
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

        for (int i = 0; i < code_parameters.size(); i++)
        {
            //vector<int> temp_vector;

            //for(int j=0; j<10; j++){
            //    temp_vector.push_back(rand() % 2);
            // }
            // check if we have used up all the available
            //  resources
            if (i < nchildren) {
                pid = generator_make(i, code_parameters.at(i));
                allpids[i]=pid;
                log_message << generator_vec.at(i) << " : " << vector_to_string_demo(code_parameters.at(i)) << "\n";
                logger(log_message.str());
                log_message.str("");
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
                    allpids[i]=pid;
                    log_message << generator_vec.at(ii) << " : " << vector_to_string_demo(code_parameters.at(i)) << "\n";
                    logger(log_message.str());
                    log_message.str("");
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
        /* look for the existence of transform.timestep.dat file
           this is a hack, will change it to something more robust
           and more importantly efficient later.
        */

        // first indicate that this round is complete
        ss.str("");
        ss << "touch /hivehomes/tiwari/SC09-demo/harmony/bin/code_flags/code_complete." << timestep;
        system(ss.str().c_str());

        // clear the stream
        ss.str("");
        // increment the timestep
        timestep++;
        // the file to watch
        ss << "confs/candidate_simplex." << timestep << ".dat";
        cout << ss.str() << "\n";
        while(!file_exists(ss.str().c_str()));

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

void
process_point_line(string line, vector<int>& one_point)
{
    StringTokenizer strtok_space (line, " ");
    while(strtok_space.hasMoreTokens())
    {
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
    /* File found */
    if ( i == 0 )
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
