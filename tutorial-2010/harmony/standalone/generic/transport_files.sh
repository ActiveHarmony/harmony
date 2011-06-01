#!/bin/bash

appname=$1

cd $HOME/scratch/new_code_${appname}

tar -cvf all_code_${appname}.tar *

bzip2 all_code_${appname}.tar 

echo "done zipping"

# brood
scp all_code_${appname}.tar.bz2 rahulp@brood00:~/scratch/code/

########################## TRANSPORT ####################################


#hopper
rm -rf *.o *.c *.so
rm -rf all_code_${appname}.tar.bz2


# brood
ssh rahulp@brood00 tar -xjf /hivehomes/rahulp/scratch/code/all_code_${appname}.tar.bz2 -C /hivehomes/rahulp/scratch/code/
ssh rahulp@brood00 rm /hivehomes/rahulp/scratch/code/all_code_${appname}.tar.bz2



