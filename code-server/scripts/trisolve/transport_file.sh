#!/bin/bash

cd $HOME/code-server/scratch_space/tmp/transport

tar -cvf all_code.tar *

bzip2 -9 all_code.tar 

cp all_code.tar.bz2 ../zipped

rm loop_*.so

rm all_code.tar.bz2
