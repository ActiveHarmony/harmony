#!/bin/bash

cd /fs/spoon/tiwari/code_generator/hosts/newcode

tar -cvf all_code.tar *

bzip2 -9 all_code.tar 

scp all_code.tar.bz2 tiwari@brood00:~/pmlb/fusion_ex/code/zipped

rm streaming_*.so
rm all_code.tar.bz2
