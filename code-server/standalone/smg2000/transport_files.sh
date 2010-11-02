#!/bin/bash

cd $HOME/scratch/new_code

tar -cvf all_code.tar *

bzip2 all_code.tar 

scp all_code.tar.bz2 tiwari@brood00:~/scratch/code/
rm *.so
rm all_code.tar.bz2

ssh tiwari@brood00 tar -xjf /hivehomes/tiwari/scratch/code/all_code.tar.bz2 -C /hivehomes/tiwari/scratch/code/

ssh tiwari@brood00 rm /hivehomes/tiwari/scratch/code/all_code.tar.bz2
