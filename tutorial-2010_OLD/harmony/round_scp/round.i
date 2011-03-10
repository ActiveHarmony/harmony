%module round
%{
#include <math.h>
#include <stdio.h>
#include "scp_candidate.h"
%}

// ANSI C/C++ prototypes
double round(double x);
int scp_candidate(char* filename, char* destination);
