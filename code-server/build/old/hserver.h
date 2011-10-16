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
#ifndef __HSERVER_H__
#define __HSERVER_H__

/***
 *
 * include system headers
 *
 ***/
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
/***
 *
 * include user defined headers
 *
 ***/
#include "hutil.h"
#include "hmesgs.h"
#include "hsockutil.h"


using namespace std;


/*** IMPORTANT
 * These are the global variables that will most *likely*
 *  change for different experiments.
 *
 */




/***
 *
 * define macros
 *
 ***/

#define SERVER_PORT 1977
#define BUFF_SIZE 1024



typedef vector<vector<int> > IntArray;


#define SEM_ID    2503	 /* ID for the semaphore.*/

// primary buffer size
//   INCREASE THIS if your simplex does not fit in this buffer size
#define PRIMARY_BUFF 4096
// secondary buffer size -- INCREASE THIS MAX if your multiple-ply
//   search does not fit in this MAX BUFFER SIZE
#define SECONDARY_BUFF 16384

union semun {
 int val;
 struct semid_ds *buf;
 unsigned short int *array;
 };


// this is where we store the primary simplex.
//  the configurations stored here will always get preference when it
//  comes to code-generation
// some additional parameters:
//  num_bytes is *not* used right now.
//  iteration
//  indicator: '$' : this is a brand new set of configurations
//           : '*' : code manager has read the new set and is DONE
//                   generating the code.
struct primary_buf
{
  int num_bytes;
  char indicator[1];
  int iteration;
  char buffer[PRIMARY_BUFF];
};

// this is filled with locality based configurations.
//  when the code-generator manager runs out of primary configuration,
//    the manager selects configurations from this buffer to keep
//    code-generators busy
struct secondary_buf
{
  int num_bytes;
  char indicator[1];
  int iteration;
  char buffer[SECONDARY_BUFF];
};

// this is used by the generator-manager to indicate that the current
//   round (iteration) of code-generation is complete.
struct done_buf
{
  int done;
  int iteration;
};

// globals for shared memory segments
void *primary_confs = (void *)0;
void *secondary_confs = (void *)0;
void *done_indicator = (void *)0;
int sem_id=-1;
int shm_id_primary=-1;
int shm_id_secondary=-1;
int shm_id_done=-1;
//int usr_interrupt=0;
/***
 *
 * define prototypes
 *
 ***/
void setup_shared_buffers();
void check_parameters(int argc, char **argv);
void sem_lock(int sem_set_id);
void sem_unlock(int sem_set_id);
int setup_semaphore(void);
struct primary_buf *setup_primary_buffer(void);
struct secondary_buf *setup_secondary_buffer(void);
struct done_buf *setup_done_buffer(void);
void revoke_shared_mem_resources(void);
void write_to_primary_buffer(char *confs, int server_or_manager);
void write_to_secondary_buffer(char *confs, int server_or_manager);
void write_to_done_indicator(struct done_buf *done_i, int flag);
char *read_from_primary(struct primary_buf *primary, int peek);
char *read_from_secondary(struct secondary_buf *secondary);
int read_from_done(struct done_buf *done_i);
void random_delay(void);
void do_child_1(void);
void do_parent_1(void);
void operation_failed(int sock);
void server_startup(void);
void revoke_resources(char *appName);
void process_message_from(int temp_socket);
void main_loop(int generator_pid);
int setup_shared_memory(void);
int fork_and_launch(void);
void sighup_user(int signumber);
void sigint_user(int signumber);
void sigquit_user(int signumber);
void setup_all_signals(void);
void generate_code(HDescrMessage *mesg, int client_socket);

void generator_manager_main(int server_pid_);

#endif /* __HSERVER_H__ */
