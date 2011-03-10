#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include "Tokenizer.h"

using namespace std;

/*
 * takes the app specific generator_host file and parses it.
 */
void get_code_generators (string filename);

int main(int argc, char **argv) {
  string generator_host_filename=argv[1];
  get_code_generators(generator_host_filename);
  return 1;
}

void
get_code_generators (string filename)
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
            }
            num_lines++;
        }
        myfile.close();
    }
    else cout << "Unable to open file: " << filename;
}
