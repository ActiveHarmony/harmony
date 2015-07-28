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

/*
 * Design:
 *     code-server : receives and sends messages to the harmony server
 *		   : populated the primary and secondary configurations
 *     generator-parent : forked by code-server
 *			: changes the done-flag to notify the code-server 
 * 			  that code-generation for primary configurations is done
 *     generators: forked by generator-parent to generate code using 
 *		   some external code generation tools. these die after
 *		   generating code for one configuration
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <wait.h>
#include <time.h>
#include <stdlib.h>
#include <vector>
//#include    <sys/ipc.h>

using namespace std;
#define SEM_ID    2503	 /* ID for the semaphore.*/

// primary buffer
#define PRIMARY_BUFF 4096
// secondary buffer
#define SECONDARY_BUFF 16384

union semun {
 int val;
 struct semid_ds *buf;
 unsigned short int *array;
 };

struct primary_buf
{
  int num_bytes;
  int iteration;
  char buffer[PRIMARY_BUFF];
};

struct secondary_buf
{
  int num_bytes;
  int iteration;
  char buffer[SECONDARY_BUFF];
};

struct done_buf
{
  int done;
  int iteration;
};

void *primary_confs = (void *)0;
void *secondary_confs = (void *)0;
void *done_indicator = (void *)0;

int sem_id=-1;
int shm_id_primary=-1;
int shm_id_secondary=-1;
int shm_id_done=-1;

/*
 * function: sem_lock. locks the semaphore, for exclusive access to a resource.
 * input:    semaphore set ID.
 * output:   none.
 */
void sem_lock(int sem_set_id)
{
    /* structure for semaphore operations.   */
    struct sembuf sem_op;

    /* wait on the semaphore, unless it's value is non-negative. */
    sem_op.sem_num = 0;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
    semop(sem_set_id, &sem_op, 1);
}

/*
 * function: sem_unlock. un-locks the semaphore.
 * input:    semaphore set ID.
 * output:   none.
 */
void sem_unlock(int sem_set_id)
{
    /* structure for semaphore operations.   */
    struct sembuf sem_op;

    /* signal the semaphore - increase its value by one. */
    sem_op.sem_num = 0;
    sem_op.sem_op = 1;   /* <-- Comment 3 */
    sem_op.sem_flg = 0;
    semop(sem_set_id, &sem_op, 1);
}

/* setup the semaphore 
 *  currently using just one semaphore to control all three shared
 *  memory segments. Need to perform some testing for deadlocks.
*/
int setup_semaphore()
{
  union semun sem_val; 
  /* create a semaphore set with ID 250, with one semaphore   */
  /* in it, with access only to the owner.                    */
  sem_id = semget(SEM_ID, 1, IPC_CREAT | 0600);
  if (sem_id == -1) {
    perror("main: semget");
    exit(1);
  }

  /* intialize the first (and single) semaphore in our set to '1'. */
  sem_val.val = 1;
  int rc = semctl(sem_id, 0, SETVAL, sem_val);
  if (rc == -1) {
    perror("main: semctl");
    exit(1);
  }

  return rc;
}

/*
 * the next three setup_buffers functions all do the same thing:
 *    setup shared memory segments: one each for primary confs,
 *      secondary confs and done indicator.
*/

struct primary_buf * setup_primary_buffer() {
  shm_id_primary=shmget(IPC_PRIVATE, sizeof(struct primary_buf), 0666 | IPC_CREAT | IPC_EXCL);
  if(shm_id_primary == -1) {
    perror("primary: shmget failed\n");
    exit(-1);
  }

  primary_confs=shmat(shm_id_primary, (void*)0,0);
  if(primary_confs==(void*)-1){
    perror("primary: shmat failed\n");
    exit(-1);
  }	
    
  // Good idea to initialize shared memory
  memset(primary_confs, 0, sizeof(struct primary_buf));

  return (struct primary_buf *) primary_confs;

}

struct secondary_buf * setup_secondary_buffer() {
  shm_id_secondary=shmget(IPC_PRIVATE, sizeof(struct secondary_buf), 0666 | IPC_CREAT | IPC_EXCL);
  if(shm_id_secondary == -1) {
    perror("secondary: shmget failed\n");
    exit(-1);
  }

  secondary_confs=shmat(shm_id_secondary, (void*)0,0);
  if(secondary_confs==(void*)-1){
    perror("secondary: shmat failed\n");
    exit(-1);
  }	
    
  // Good idea to initialize shared memory
  memset(secondary_confs, 0, sizeof(struct secondary_buf));

  return (struct secondary_buf *) secondary_confs;
}


struct done_buf * setup_done_buffer() {
  shm_id_done=shmget(IPC_PRIVATE, sizeof(struct done_buf), 0666 | IPC_CREAT | IPC_EXCL);
  if(shm_id_done == -1) {
    perror("done: shmget failed\n");
    exit(-1);
  }

  done_indicator=shmat(shm_id_done, (void*)0,0);
  if(done_indicator==(void*)-1){
    perror("done: shmat failed\n");
    exit(-1);
  }	
    
  // Good idea to initialize shared memory
  memset(done_indicator, 0, sizeof(struct done_buf));

  return (struct done_buf *) done_indicator;
}

/*
 * Revokes all tied up resources.
*/
void revoke_resources(){
  union semun semarg;

  // Detach Memory address
  if(shmdt(primary_confs) == -1){
    perror("shmdt failed for primary confs\n");
  }

  if(shmctl(shm_id_primary, IPC_RMID,0) == -1) {
    perror("shmctl failed for primary confs\n");
  }
  
  if(shmdt(secondary_confs) == -1){
    perror("shmdt failed for secondary confs\n");
  }

  if(shmctl(shm_id_secondary, IPC_RMID,0) == -1) {
    perror("shmctl failed for secondary confs\n");
  }


  if(shmdt(done_indicator) == -1){
    perror("shmdt failed for done indicator \n");
  }

  if(shmctl(shm_id_done, IPC_RMID,0) == -1) {
    perror("shmctl failed for done confs\n");
  }

  if (semctl(sem_id, 0, IPC_RMID, semarg) < 0) {
    perror("free_resources: semid ");
  }
}

/*
 *  code-server fills this buffer with the new set of confs (leads with $)
 *  generator-parent replaces the '$' with '*' to indicate that it has 
 *    read this new set of confs
*/
void write_to_primary_buffer(struct primary_buf *primary, char* confs) {
 
  // attempt to get the lock first.
  sem_lock(sem_id);
  strcpy(primary->buffer,confs);
  sem_unlock(sem_id);
}

void write_to_secondary_buffer(struct secondary_buf *secondary, char* confs) {
 
  // attempt to get the lock first.
  sem_lock(sem_id);
  strcpy(secondary->buffer,confs);
  sem_unlock(sem_id);
}

void write_to_done_indicator(struct done_buf *done_i, int flag) {
  // attempt to get the lock first.
  sem_lock(sem_id);
  done_i->done=flag;
  sem_unlock(sem_id);
}

char* read_from_primary(struct primary_buf *primary,int peek) {
  sem_lock(sem_id);
  char* tmp;
  if(peek)
    tmp=&(primary->buffer[0]);
  else
    tmp=primary->buffer;
  sem_unlock(sem_id);
  return tmp;
}

char* read_from_secondary(struct secondary_buf *secondary) {
  sem_lock(sem_id);
  char* tmp=secondary->buffer;
  sem_unlock(sem_id);
  return tmp;

}

int read_from_done(struct done_buf *done_i) {
  sem_lock(sem_id);
  int tmp=done_i->done;
  sem_unlock(sem_id);
  return tmp;
}

/* definitions */
struct candidates {
  vector<int> candidates;
  int iteration;
};


struct new_candidates_flag {
  int flag;
  int iteration;
};

struct done_indicator {
  int flag;
  int iteration;
};


/* define a structure to be used in the given shared memory segment. */
struct country {
    char name[30];
    char capital_city[30];
    char currency[30];
    int population;
};

/*
 * function: random_delay. delay the executing process for a random number
 *           of nano-seconds.
 * input:    none.
 * output:   none.
 */
void
random_delay()
{
    static int initialized = 0;
    int random_num;
    struct timespec delay;            /* used for wasting time. */

    if (!initialized) {
	srand(time(NULL));
	initialized = 1;
    }

    random_num = rand() % 5;
    //printf("random_num %d \n", random_num);
    delay.tv_sec = random_num;
    delay.tv_nsec = 500*random_num;
    nanosleep(&delay, NULL);
}



/*
 * only the code-server main can call this.
 */

void add_new_primaries(int sem_set_id, int num, struct candidates primaries, vector<int> candidates) {
  // get the lock
  sem_lock(sem_set_id);
  primaries.candidates=candidates;
  primaries.iteration=num;
  //release
  sem_unlock(sem_set_id);
}

/*
 * only the code-server can call this.
*/

void add_new_secondaries(int sem_set_id, int num, struct candidates secondaries, vector<int> candidates) {
  // get the lock
  sem_lock(sem_set_id);
  secondaries.candidates=candidates;
  // release
  sem_unlock(sem_set_id);
}



/*
 * only the generator parent can call this.
*/



/*
 * function: add_country. adds a new country to the counties array in the
 *           shard memory segment. Handles locking using a semaphore.
 * input:    semaphore id, pointer to countries counter, pointer to
 *           counties array, data to fill into country.
 * output:   none.
 */
void
add_country(int sem_set_id, int* countries_num, struct country* countries,
	    char* country_name, char* capital_city, char* currency,
	    int population)
{
    sem_lock(sem_set_id);
    strcpy(countries[*countries_num].name, country_name);
    strcpy(countries[*countries_num].capital_city, capital_city);
    strcpy(countries[*countries_num].currency, currency);
    countries[*countries_num].population = population;
    (*countries_num)++;
    sem_unlock(sem_set_id);
}

/*
 * function: do_child. runs the child process's code, for populating
 *           the shared memory segment with data.
 * input:    semaphore id, pointer to countries counter, pointer to
 *           counties array.
 * output:   none.
 */
void
do_child(int sem_set_id, int* countries_num, struct country* counties)
{
    add_country(sem_set_id, countries_num, counties,
		"U.S.A", "Washington", "U.S. Dollar", 250000000);
    random_delay();
    add_country(sem_set_id, countries_num, counties,
		"Israel", "Jerusalem", "New Israeli Shekel", 6000000);
    random_delay();
    add_country(sem_set_id, countries_num, counties,
		"France", "Paris", "Frank", 60000000);
    random_delay();
    add_country(sem_set_id, countries_num, counties,
		"Great Britain", "London", "Pound", 55000000);
}


void
do_child_1()
{
  random_delay();
  for(int i=0; i <5; i++) {
    char * values_in_primary=read_from_primary((struct primary_buf *)primary_confs,0);
    //if(*values_in_primary=='$')
    // {
    printf("found %s at the beginning \n", values_in_primary);
	//      }
	//    else if(*values_in_primary=='*'){
	//      	printf("found * at the beginning \n");
      
	//    } else {
	//      printf("reader found %s in the beginning \n", *values_in_primary);
	//}	
    random_delay();
    printf("reader is waking up \n");
  }
  
}

void
do_parent_1()
{
  random_delay();
  char temp_[20]="$*$* 12 13 14 15";
  char * pointer =&temp_[0];
  for(int i=0; i <4; i++) {
    
    write_to_primary_buffer((struct primary_buf *)primary_confs, pointer);
    random_delay();
    printf("writer is waking up \n");
    pointer=&temp_[i];
  }


}

/*
 * function: do_parent. runs the parent process's code, for reading and
 *           printing the contents of the 'countries' array in the shared
 *           memory segment.
 * input:    semaphore id, pointer to countries counter, pointer to
 *           counties array.
 * output:   printout of countries array contents.
 */
void
do_parent(int sem_set_id, int* countries_num, struct country* countries)
{
    int i, num_loops;

    for (num_loops=0; num_loops < 5; num_loops++) {
        /* now, print out the countries data. */
        sem_lock(sem_set_id);
        printf("---------------------------------------------------\n");
        printf("Number Of Countries: %d\n", *countries_num);
        for (i=0; i < (*countries_num); i++) {
            printf("Country %d:\n", i+1);
            printf("  name: %s:\n", countries[i].name);
            printf("  capital city: %s:\n", countries[i].capital_city);
            printf("  currency: %s:\n", countries[i].currency);
            printf("  population: %d:\n", countries[i].population);
        }
        sem_unlock(sem_set_id);
	random_delay();
    }
}

int main(int argc, char* argv[])
{
    int sem_set_id;            /* ID of the semaphore set.           */
    union semun sem_val;       /* semaphore value, for semctl().     */
    int shm_id, shm_id_1, shm_id_2, shm_id_3; /* ID of the shared memory segment.*/
    char* shm_addr; char* shm_addr_1; char* shm_addr_2; char* shm_addr_3; 	       /* address of shared memory segment.  */
    int* countries_num;        /* number of countries in shared mem. */
    struct country* countries; /* countries array in shared mem.     */
    struct shmid_ds shm_desc;
    struct shmid_ds shm_desc_1;
    struct shmid_ds shm_desc_2;
    struct shmid_ds shm_desc_3;
    int rc;		       /* return value of system calls.      */
    pid_t pid;		       /* PID of child process.              */


    setup_semaphore();
    setup_primary_buffer();
    setup_secondary_buffer();
    setup_done_buffer();


    
    /* fork-off a child process that'll populate the memory segment. */
    pid = fork();
    switch (pid) {
	case -1:
	    perror("fork: ");
	    exit(1);
	    break;
	case 0:
	    do_child_1();
	    exit(0);
	    break;
	default:
	    do_parent_1();
	    break;
    }

    /* wait for child process's termination. */
    {
        int child_status;

        wait(&child_status);
    }

    revoke_resources();

    return 0;
}
