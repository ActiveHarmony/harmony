#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#define SHM_SIZE 30
using namespace std;
extern int etext, edata, end;
int  main( ) {
     int    shmid;
     char   c, *shm, *s;
     if ((shmid=shmget(IPC_PRIVATE,SHM_SIZE,IPC_CREAT|0660))< 0) {
       perror("shmget fail");
        return 1;
     }
     if ((shm = (char *)shmat(shmid, 0, 0)) == (char *) -1) {
         perror("shmat : parent");
        return 2;
     }
    cout << "Addresses in parent"  << endl;
    cout << "shared mem: " << hex << int(shm) << " etext: "
           << &etext << " edata: "  << &edata
           << " end: " << &end << endl << endl;
     s = shm;                             // s now references shared mem
     for (c='A'; c <= 'Z'; ++c)           // put some info there
        *s++ = c;
        *s='\0';                            // terminate the sequence
        cout << "In parent before fork, memory is: " << shm << endl;
        switch (fork( )) {
        case -1:
          perror("fork");
          return 3;
        default:
          //wait(0);                          // let the child finish
          cout << "In parent after fork, memory is : " << shm << endl;
          cout << "\nParent removing shared memory" << endl;
          shmdt(shm);
          shmctl(shmid, SHM_LOCK, (struct shmid_ds *) 0);
          break;
        case 0:
          cout << "In child after fork, memory is  : " << shm << endl;
          for ( ; *shm; ++shm)              // modify shared memory
            *shm += 32;
          shmdt(shm);
          break;
        }
       return 0;
}
