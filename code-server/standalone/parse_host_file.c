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
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>

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
