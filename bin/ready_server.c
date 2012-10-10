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
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
//#include <sys/wait.h>
#include <sys/stat.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;

string full_path("/hivehomes/tiwari/harmony_pro_release/bin/");

void logger (string message);
int file_exists (const char * fileName);

string log_file("/hivehomes/tiwari/harmony_pro_release/bin/last_port.log");

int main(int argc, char **argv)
{
    ifstream myfile;
    stringstream ss, log_message,ss2;
    int timestep=1;
    int experiment_num=1;
    //int ppid = getpid();

    // first wait until the port_start signal is received
    ss.str("");
    ss << full_path << "port_start." << experiment_num <<".dat";
    cout << ss.str() << "\n";
    while(!file_exists(ss.str().c_str())){
        //printf("sanityyyy \n");
        sleep(1);
    }
    printf("got the port_start file \n");

    ss.str("");
    ss << full_path << "port_start." << experiment_num <<".dat";

    //string port_start_file("/global/homes/t/tiwari/harmony/bin/port_start.dat");
    myfile.open(ss.str().c_str());
    string line;
    experiment_num++;
    // read in the port start number
    if (myfile.is_open()) {
        //while (! myfile.eof()) {
        getline (myfile,line);
        cout << "Here is the start port number " << line << endl;
        //}
        myfile.close();
    } else {
        cout << "Unable to open port_start.dat file \n";
        exit(0);
    }

    int port_start=atoi(line.c_str());
    cout << "atoi returns " << port_start << endl;

    std::remove(ss.str().c_str());
    // command to kill all the previous hservers
    string kill_command("source /hivehomes/tiwari/harmony_pro_release/bin/kill.sh hserver");
    int sys_return;

    while(true) {
        ss.str("");
        ss2.str("");
        ss << full_path << "server_num." << timestep << ".dat";
        ss2 << full_path << "port_start.1.dat";
        
        cout << ss.str() << "\n";
        cout << ss2.str() << "\n";

        while(!file_exists(ss.str().c_str()) &&
              !file_exists(ss2.str().c_str())
            )
        {
            //printf("sanityyyy \n");
            sleep(1);
        }

        if(file_exists(ss2.str().c_str()))
        {
            std::remove(ss2.str().c_str());
            break;
        }

        std::remove(ss.str().c_str());
        printf("got a new server request \n");

        // kill the existing ones?
        sys_return=system(kill_command.c_str());

        ss.str("");
        ss << full_path << "start_one.sh "
            << port_start;

        cout << ss.str() << "\n";

        sys_return=system(ss.str().c_str());

        ss.str("");
        ss << "" << port_start << "," << experiment_num << "\n";
        logger(ss.str());

        timestep++;
        //port_record.push_back(port_start);
        port_start++;
    }
    return 1;
}

int
file_exists (const char * fileName)
{
    struct stat buf;
    int i = stat ( fileName, &buf );
    //int size = buf.st_size;
    /* File found */
    if ( i == 0 )
    {
        return 1;
    }
    return 0;

}

void
logger (string message)
{
    string line;
    ofstream out_file;
    out_file.open(log_file.c_str(),ios::trunc);
    if(!out_file)
    {
        cerr << "Error file could not be opened \n";
        exit(1);
    }

    out_file << message;
    out_file.flush();
    out_file.close();


}
