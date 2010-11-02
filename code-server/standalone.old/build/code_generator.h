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
#include "Tokenizer.h"
//#include "utilities.h"

using namespace std;


//map<string,int> global_data;
//map<string,int>::iterator it;
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
void get_code_generators (vector<string>& vec, vector<string>& paths,string filename);
void get_points_vector (string point, vec_of_intvec& code_parameters);
string vector_to_string(vector<int> v);
string vector_to_string_demo(vector<int> v);
void get_parameters_from_file(vec_of_intvec& intvec, string filename);
vector<string> generator_vec;
vector<string> generator_vec_path;
int file_exists (const char * fileName);
void populate_remove(vec_of_intvec all_params,vec_of_intvec& code_parameters);
void print_vec(vector<int>& a);

string log_file("code_generation.log");
string appname("irs");
void logger (string message);

generator *gen_ptr;

string generator_hosts_filename("generator_hosts_processed");
