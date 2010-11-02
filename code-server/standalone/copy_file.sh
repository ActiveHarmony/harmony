#!/bin/bash

cd /fs/spoon/tiwari/standalone/hosts/new_code

tar -cvf all_code.tar *

bzip2 all_code.tar 

scp all_code.tar.bz2 tiwari@brood00:/scratch0/code/
rm *.so
rm all_code.tar.bz2

ssh tiwari@brood00 unzip /scratch0/code/all_code.tar.bz2

