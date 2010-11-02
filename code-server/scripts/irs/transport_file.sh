#!/bin/bash

cd $HOME/code-server/scratch_space/tmp/transport

ls

tar -cvf all_code.tar *

bzip2 -9 all_code.tar 

cp all_code.tar.bz2 ../zipped

rm *.so

rm all_code.tar.bz2

## where do you want to send the files?

### THIS IS SUBJECT TO CHANGE FOR EXPERIMENTS ON DIFFERENT PLATFORMS
cd $HOME/code-server/scratch_space/tmp/zipped/

scp all_code.tar.bz2 tiwari@brood00:/hivehomes/tiwari/irs.1.0/decks/code/

ssh brood00 "tar -xjf /hivehomes/tiwari/irs.1.0/decks/code/all_code.tar.bz2 -C /hivehomes/tiwari/irs.1.0/decks/code/"
 
ssh brood00 "rm /hivehomes/tiwari/irs.1.0/decks/code/all_code.tar.bz2"
