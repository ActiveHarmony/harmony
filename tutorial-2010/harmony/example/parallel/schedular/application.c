#include <math.h>
#include <stdio.h>

/*
  A simple application for demonstration purposes.
  When you want to tune real applications, you replace a call to this
  application in your submission script.
 */


int application(int i) 
{
    return (i-9)*(i-9)*(i-9)-75*(i-9)+24 ;
}



int main(int argc, char **argv) 
{
    int param=atoi(argv[1]);
    printf("perf: %d \n", application(param));
    return 1;

}
