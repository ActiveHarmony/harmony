/*
 * Copyright 2003-2016 Jeffrey K. Hollingsworth
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
#include <sys/param.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include "Tokenizer.h"
using namespace std;

stringstream& ss_insert_front( stringstream& ss, const string& s );
// declarations
typedef vector<vector<int> > vec_of_intvec;
// generator typedef
typedef struct {
  std::vector<int> parameters;
    int avail;
    //string hostname;
    int primary;
    int pid;
} generator;

/*
typedef struct {
  string hostname;
  string local_working_dir;
  vector<string> files_vec(20,10);
  int num_instances;
  int remote;
} generator_host;
*/

typedef pair <int, int> Int_Pair;

//function prototypes
int copy_file(char* from_name, char* to_name);
void process_point_line(string line, vector<int>& one_point);
void get_parameters_from_file(vector<vec_of_intvec>& intvec, string filename);
void get_code_generators (vector<string>& vec, string filename);
void get_points_vector (string point, vec_of_intvec& code_parameters);
string vector_to_string(vector<int> v);
string vector_to_string_demo(vector<int> v);
void get_parameters_from_file(vec_of_intvec& intvec, string filename);
vector<string> generator_vec;
int file_exists (const char * fileName);
//void populate_remove(vec_of_intvec all_params,vec_of_intvec& code_parameters);
void print_vec(vector<int>& a);
void sighup(int signumber);
void sigint(int signumber);
void sigquit(int signumber);
void parse_configurations(vec_of_intvec& intvec, string confs);
bool IsDirectory(char path[]);
bool RemoveDirectory(string path);



void print_vec(vector<int>& a)
{
    for(int i=0; i<a.size(); ++i)
        cout << a[i] << " ";
    cout << endl;
    cout << "----------------"<<endl;
}//print



string append_in_front(string str, string front){
   stringstream ss;
   ss << front;
   ss << str;
   return ss.str();
}



void
populate_remove(vec_of_intvec all_params, vec_of_intvec& code_parameters,
                map<string,int>& global_data,
                map<string, int>& transport_data, int prim)

{

  map<string,int> temp_map;

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

    /*
     * first check whether code has already been generated for this
     *  configuration.
     */
    map <string, int>:: iterator iter_1;
    map <string, int>:: iterator iter_2;

    cout << "trying to look for : " << ss.str() << endl;

    // code already produced?
    iter_1=global_data.find(ss.str());
    // file already transported?
    iter_2=transport_data.find(ss.str());

    if(iter_1==global_data.end()) {
      // code has not been generated for this configuration yet
      // check if this is a duplicate
      cout << "string from vector_to_string " << ss.str().c_str() << "\n";
      pair< map<string,int>::iterator, int > temp_flag;
      temp_flag=temp_map.insert(pair<string, int>(ss.str(), 1));
      // if this is a part of the primary, then add this to the
      //  transport data structure
      if(prim)
        transport_data.insert(pair<string, int>(ss.str(), 1));
      //ss.str("");
      if(temp_flag.second)
      {
        temp.insert(temp.begin(),1);
        code_parameters.push_back(temp);
      }
      ss.str("");
    } else {
      if(iter_2==transport_data.end()) {
        // we have generated code, but the code has not been transported yet.
        if(debug_mode)
          printf(" C O D E   A L R E A D Y   G E N E R A T E D ! ! \n");
        temp.insert(temp.begin(),0);
        code_parameters.push_back(temp);
        ss.str("");
      }
    }

  }
}

stringstream& ss_insert_front( stringstream& ss, const string& s )
{
  streamsize pos = ss.tellp();
  ss.str( s + ss.str() );
  ss.seekp( pos + s.length() );
  return ss;
  }

void
logger (string message, string log_file)
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
    string machine_name;
    string instances;
    int num_instances;

    myfile.open(filename.c_str());
    if (myfile.is_open()) {
        while(getline (myfile,line)) {
            if(line.length() == 0) break;
            Tokenizer strtok_space;
            strtok_space.set(line, " ");
            machine_name=strtok_space.next();
            instances=strtok_space.next();
            num_instances=atoi(instances.c_str());
            for(int ii=1; ii<=num_instances; ii++) {
              stringstream tmp;
              tmp.str("");
              tmp << machine_name << "_" << ii;
              cout << tmp.str() << endl;
              vec.push_back(tmp.str());
            }
            num_lines++;
        }
        myfile.close();
    }
    else cout << "Unable to open file";
    //return num_lines;
}

void
get_file_list (vector<string>& vec, string filename)
{
  int num_lines=0;
  string line;
  ifstream myfile;

  myfile.open(filename.c_str());
  if (myfile.is_open()) {
    while(getline (myfile,line)) {
      if(line.length() == 0) break;
      vec.push_back(line);
      num_lines++;
    }
    myfile.close();
    cout << getpid() << " NUM_LINES READ: " << num_lines;
  }
  else cout << "Unable to open file";
  //return num_lines;
}

void
get_code_generators_old (vector<string>& vec, string filename)
{
    int num_lines=0;
    string line;
    ifstream myfile;

    myfile.open(filename.c_str());
    if (myfile.is_open()) {
        //while (! myfile.eof() ) {
        while(getline (myfile,line)) {

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

void parse_configurations(vec_of_intvec& intvec, string confs) {
  // format : point_1 : point_2 : ... where each coordinate value within
  //   a given point is separated by a space.

  int num_lines=0;
  Tokenizer strtok_colon;
  strtok_colon.set(confs, ":");
  string line=strtok_colon.next();
  cout << "LINE: " << line << endl;
  while(line!="") {
    vector<int> one_point;
    process_point_line(line, one_point);
    intvec.push_back(one_point);
    num_lines++;
    line=strtok_colon.next();
    cout << "LINE: " << line << endl;
  }
  cout << getpid() << " NUM_LINES READ: " << num_lines;

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

int copy_file(char* from_name, char* to_name) {
  FILE *from, *to;
  char ch;

  /* open source file */
  if((from = fopen(from_name, "rb"))==NULL) {
    printf("Cannot open source file.\n");
    exit(1);
  }

  /* open destination file */
  if((to = fopen(to_name, "wb"))==NULL) {
    printf("Cannot open destination file.\n");
    exit(1);
  }

  /* copy the file */
  while(!feof(from)) {
    ch = fgetc(from);
    if(ferror(from)) {
      printf("Error reading source file.\n");
      exit(1);
    }
    if(!feof(from)) fputc(ch, to);
    if(ferror(to)) {
      printf("Error writing destination file.\n");
      exit(1);
    }
  }

  if(fclose(from)==EOF) {
    printf("Error closing source file.\n");
    exit(1);
  }

  if(fclose(to)==EOF) {
    printf("Error closing destination file.\n");
    exit(1);
  }

  return 0;
}


void
process_point_line(string line, vector<int>& one_point)
{
  Tokenizer strtok_space;
  strtok_space.set(line, " ");
  string temp_str = strtok_space.next();
    while(temp_str != "")
    {
      int temp = atoi(temp_str.c_str());
        one_point.push_back(temp);
        temp_str=strtok_space.next();
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

  Tokenizer strtok_colon;
  strtok_colon.set(point, ":");


  string colon_token=strtok_colon.next();
    vector<int> temp_vector;
    while (colon_token != "")
    {
      Tokenizer strtok_space;
      strtok_space.set(colon_token," ");
      string temp_str = strtok_space.next();
        while(temp_str != "")
        {
          int temp = atoi(temp_str.c_str());
          //cout << temp << ", ";
          temp_vector.push_back(temp);
          temp_str = strtok_space.next();
        }
        code_parameters.push_back(temp_vector);
        temp_vector.clear();
        colon_token=strtok_colon.next();

    }
}

