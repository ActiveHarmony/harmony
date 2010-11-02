#!/bin/bash

# first generate the chill script
produce_code=$2
primary_or_secondary=$1
UI=$3
UJ=$4

WORK_DIR=$HOME/signal/tmp/${5}

cd $WORK_DIR
#source exports.sh
export OMEGA_P=/fs/spoon/tiwari/omega
export SUIFHOME=/fs/spoon/tiwari/suifhome
#export OPENMPI=/usr/local/stow/openmpi-1.3.3-gm

export PATH=/fs/spoon/tiwari/chill/bin:${SUIFHOME}/i386-linux/bin/:${PATH}
export LD_LIBRARY_PATH=${SUIFHOME}/i386-linux/solib:${LD_LIBRARY_PATH}

# using chill?
useChill=1

useICC=0
out_file=loop_with_scalar_${UI}_${UJ}.so

if [ $produce_code -eq 1 ]; then
    echo "Currently generating code for configuration: UI: $2, UJ: $3"

    echo "
    source: loop_with_scalar.sp2
    procedure: 0
    loop: 0

    original()
    print
    known(bs > 14) 
    known(bs < 16)
    unroll(1,2,${UI})
    unroll(1,3,${UJ})
    " > temp.${rank}.script


    chill temp.${rank}.script
    s2c loop_with_scalar.lxf > dgemv_optimized.c

    if [ $useICC -eq 1 ]; then
	icc -g -pg -fpic -c -o dgemv_optimized.o dgemv_optimized.c -O3 -xN -unroll0
	icc --shared -lc -o $out_file dgemv_optimized.o -g -pg 
	cp $out_file ../new_code/
	mv $out_file ../transport/
    else
	gcc -g -fPIC -c -o dgemv_optimized.o dgemv_optimized.c -O2
	gcc --shared -lc -o $out_file dgemv_optimized.o -g -pg 
	cp $out_file ../new_code/
	if [ $primary_or_secondary -eq 1 ]; then
	    mv $out_file ../transport/
	fi
    fi
else
   mv ../new_code/$out_file ../transport/
fi
    
