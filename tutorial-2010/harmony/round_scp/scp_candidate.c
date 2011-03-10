#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/stat.h>

int scp_candidate(char* filename, char* destination)
{

    char cmd[256];
    sprintf(cmd, "scp %s %s", filename, destination);
    int sys_stat=system(cmd);
    return sys_stat;
}

int main() 
{
    scp_candidate("test", "tiwari@spoon:~");
}
