#!/bin/bash

_curr_dir=$PWD
export HARMONY_S_HOST=brood00 
export HARMONY_S_PORT=$1
ssh brood00 "cd /hivehomes/tiwari/harmony_pro_release/bin/; ./hserver > hserver_${i} & "
cd ${_curr_dir}

