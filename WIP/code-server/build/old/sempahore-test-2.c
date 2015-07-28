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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <fcntl.h>
using namespace std;
// ****************************************************
// GLOBAL definitions and data
// **************************************************** 
void free_resources();

// The following flag turns on debug compilation code
#define DEBUG 1

union semun {
        int val;                    /* value for SETVAL */
        struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
        unsigned short int *array;  /* array for GETALL, SETALL */
        struct seminfo *__buf;      /* buffer for IPC_INFO */
    };

// In real life, you would probably make both of the following numbers
// much larger
#define BUFF_SIZE 1024
#define NUM_BUFFS 2

// Shared Memory data structure

struct big_buff
{
    int numBytes;
    char buff[BUFF_SIZE];
};
typedef struct big_buff BIG_BUFF;

struct copy_buffs
{
    int get;
    int put;
    int numRead;
    int numWritten;
    int done;
    BIG_BUFF  buffers[NUM_BUFFS];
};
int shmid = -1;
void *shared_memory = (void *)0;

int semid = -1; /* Semaphore IPC ID */


// ****************************************************
// Setup the Semaphores
// **************************************************** 

int setup_semid()
{ 
    union semun semarg;
    int mykey = getuid();
    unsigned short  vals[] = {NUM_BUFFS/*free buffers*/,   0/*full buffers*/};
    semid = semget(mykey, 2, 0666 | IPC_CREAT  | IPC_EXCL);
    if (semid < 0)
    {
        perror("semget(semid)");
        return -1;
    } 
    printf("created semid %d\n", semid);

    semarg.array = vals;
    if (semctl(semid, 0, SETALL, semarg) < 0)
    {
        perror("semctl(semid)");
        free_resources();
        return -1;
    }
    return 0;
}

// ****************************************************
// Used to increment and decrement Free Buffer counters
// **************************************************** 

void accessFreeBuffs(int freeBuff_inc){

       struct sembuf sops;
#ifdef DEBUG
       // This debug code was added to watch the semaphore counters in action

       union semun semarg;
       unsigned short vals[2];
       
       semarg.array = vals;
       if (semctl(semid, 0, GETALL, semarg) < 0)
        {
             perror("accessFreeBuffs: semctl");
        }
        
       printf("accessFreeBuffs freeBuff_inc(%d) val[free] = %d  val[full]=%d\n", 
            freeBuff_inc, vals[0], vals[1]);
#endif
       sops.sem_flg = 0; //SEM_UNDO -- causes problems 
                         // when Reader terminates before writer;
    
       sops.sem_num = 0;
       sops.sem_op = freeBuff_inc;
       if (semop(semid, &sops, 1) < 0)
                perror("semop buffs");    
       return;
   }
   
// ****************************************************
// Used to decrement and increment Full Buffer counters
// **************************************************** 

void accessFullBuffs(int fullBuff_inc){
       struct sembuf sops;
#ifdef DEBUG
 // This debug code was added to watch the semaphore counters in action

       union semun semarg;
       unsigned short vals[2];
       
       semarg.array = vals;
       if (semctl(semid, 0, GETALL, semarg) < 0)
        {
             perror("accessFreeBuffs: semctl");
        }
        
       printf("accessFullBuffs fullBuff_inc(%d) val[free] = %d  val[full]=%d\n", 
            fullBuff_inc, vals[0], vals[1]);
#endif
       sops.sem_flg = 0; //SEM_UNDO ; Causes problems 
                         // when reader terminates before writer
    
       sops.sem_num = 1;
       sops.sem_op = fullBuff_inc;
       if (semop(semid, &sops, 1) < 0)
                perror("semop buffs");    
       return;
   }


// **************************************************
// Setup and initialize Shared Memory
// **************************************************** 

struct copy_buffs * setup_shared_mem()
{    
    int mykey = getuid();
    printf("My shared memory key is %d\n", mykey);

    shmid = shmget((key_t)mykey, sizeof(struct copy_buffs), 0666 | 
                    IPC_CREAT | IPC_EXCL);

    if (shmid == -1) {
        perror("shmget failed\n");
        exit(-1);
    }
    
    shared_memory = shmat(shmid, (void *)0, 0);
    if (shared_memory == (void *)-1) {
        perror("shmat failed\n");
        exit(-1);
    }
// Good idea to initialize shared memory
    memset(shared_memory, 0, sizeof(struct copy_buffs));

    return (struct copy_buffs *) shared_memory;
}
// ****************************************************
// Free up semaphore and shared memory resources. 
// **************************************************** 

void free_resources()
{
    union semun semarg;
// Detach Memory address
    if (shmdt(shared_memory) == -1) {
        perror("shmdt failed\n");
    }

    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        perror("shmctl(IPC_RMID) failed\n");
    }
 
    if (semctl(semid, 0, IPC_RMID, semarg) < 0)
    {
        perror("free_resources: semid ");
    }
}
// ****************************************************
// Reader task decrements the free buffer semaphore and reads into it.  
// Once the buffer is read, the full buffer semaphore is incremented.
// **************************************************** 

void reader(struct copy_buffs *shared_stuff, char * filename)
{
    int fid;
    int nread, index;
    BIG_BUFF * buff_ptr;  
    char *buff;  
    int count=0;

    fid = open(filename, O_RDONLY);
    if (fid < 0)
        {
            printf("reader open err(%s): %s\n", filename, strerror(errno));
            printf("reader terminating\n");
            shared_stuff->done = 1;            
            accessFullBuffs(1); //  add 1 "full"  buffer

            return;
        }

    do
    {
        accessFreeBuffs(-1); // Need one free buffer 

        index = shared_stuff->get;
        buff_ptr = &shared_stuff->buffers[index];
        buff = buff_ptr->buff;
        shared_stuff->get = (shared_stuff->get +1) % NUM_BUFFS;
        nread = read(fid, buff_ptr, BUFF_SIZE);
        buff_ptr->numBytes = nread;
        if (nread > 0)
            {
#ifdef DEBUG
                printf("reader read another buffer %d\n", count++);
#endif
                shared_stuff->numRead += 1;
                accessFullBuffs(1); // add 1 full buffers
            }
        else if (nread <= 0)
            {    
                printf("reader terminated with a read of 0 bytes\n");
                shared_stuff->done = 1;            
                accessFullBuffs(1); //  add 1 full buffers
          
                if (nread < 0) perror("read error with nread < 0)" );
            }
            
    } while (nread > 0);

    close(fid);
   
// If I uncomment out the lines that use the SEM_UNDO option, then
// I need the following lines uncommented.  It seems like with SEM_UNDO
// set, that when the reader task terminates, the counters in the semaphore
// get messed up.  So the following lines can be used to keep the Reader from
// terminating until the Writer has freed up all of his buffers.
      
//    accessFreeBuffs(-1); 
//    accessFreeBuffs(-1); 
}

// ****************************************************
// The writer decrements the Full buffer semaphore and then writes it
// to the appropriate file.  Then the writer increments the full buffer semaphore.
// **************************************************** 

void writer(struct copy_buffs *shared_stuff, char * filename)
{
    int index, fid, nwritten;
    BIG_BUFF * buff_ptr;
    char *buff;    
    int count = 0;

    unlink(filename);
    fid = open(filename, O_WRONLY|O_CREAT| S_IRUSR |S_IWUSR );
    if (fid < 0)
        {
            printf("writer open err(%s): %s\n", filename, strerror(errno));
            return;
        }
    
    do
    {
        accessFullBuffs( -1); // Need one full buffers

        if (shared_stuff->done)
        {
            printf("writer sees done flag read(%d) written(%d)\n",
                shared_stuff->numRead, shared_stuff->numWritten);
            
            if (shared_stuff->numRead == shared_stuff->numWritten)
            {
                accessFreeBuffs(1); // free buffer 
                break;
            }
        }
        index = shared_stuff->put;
        buff_ptr = &shared_stuff->buffers[index];
        buff = buff_ptr->buff;
        shared_stuff->put = (shared_stuff->put +1) % NUM_BUFFS;
        nwritten = write(fid, buff, buff_ptr->numBytes);
#ifdef DEBUG
        printf("writer wrote another block %d\n", count++);
#endif
        if (nwritten > 0)
            {
                shared_stuff->numWritten += 1;
                accessFreeBuffs(1); // free one buffer 
            }
        else if (nwritten <= 0)
            {    
                perror("write error with nwritten < 0)" );
            }
            
    } while (nwritten > 0);

    close(fid);
}


// **************************************************** 
// Initializes shared memory, and the Semaphores.
// Then main starts the reader and writer tasks.
// Main waits for the termination of the reader and the 
// writer before freeing up all of the resources.  
// **************************************************** 

int main(int argc, char * argv[])
{
    struct copy_buffs *shared_stuff;
    int i, pid_reader, pid_writer, pid, status;

    if (argc < 3)
     {
        printf("usage: %s infile outfile\n");
        exit(0);
     }
        
    shared_stuff = setup_shared_mem();
    if (setup_semid())
     {
        free_resources();
     }

    pid_reader = fork();
    if (pid_reader == 0)
     {
        reader(shared_stuff, argv[1]);
        exit(0);
     }
    if (pid_reader < 0)
     {
        free_resources();
        perror("reader fork error");
        exit(-1);
     }
    printf("Created reader task %d\n", pid_reader);


    pid_writer = fork();
    if (pid_writer == 0)
     {
        writer(shared_stuff, argv[2]);
        exit(0);
     }
    if (pid_writer < 0)
     {
        free_resources();
        perror("reader fork error");
        exit(-1);
     }

    printf("Created writer task %d\n", pid_writer);


    for (i=0; i < 2; i++)
        {
	  pid = waitpid(-1, &status,0);
            if (pid == pid_reader)
                printf("reader terminated\n");
            else if (pid == pid_writer)
                printf("writer terminated\n");
            else
                printf(" Some pid terminated %d\n");            
        }
    printf("Everything done\n");
    

    free_resources();

}
 
