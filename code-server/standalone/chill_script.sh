#!/bin/bash

WORK_DIR=/fs/spoon/tiwari/smg2000_code_generator/hosts/${6}
DEST_DIR=/hivehomes/tiwari/smg2000_harmony/code/

# cd into proper directory
cd $WORK_DIR

export OMEGA_P=/fs/spoon/tiwari/omega
export SUIFHOME=/fs/spoon/tiwari/suifhome

export PATH=/fs/spoon/tiwari/chill/bin:${SUIFHOME}/i386-linux/bin/:${PATH}
export LD_LIBRARY_PATH=${SUIFHOME}/i386-linux/solib:${LD_LIBRARY_PATH}

# Use CHiLL? If you don't have a working version of the chill, set this
#  flag to 0.
useChill=1
#useICC=$6
useICC=0
# the unroll factor comes in as the first parameter
echo "Currently generating code for configuration: TI: $1, TJ: $2, TK: $3, UI: $4, US: $5"

# remove previous variant of the kernel and the result

rm -rf OUT__1__6119__.so OUT__1__6229__.c temp.script peri.result.overall peri.result.iters

if [ $useChill -eq 1 ]; then
    # generate the new version using the given unroll factor by CHiLL
    # First, generate the CHiLL script
    echo "
    source: OUT__1__6119__.spd
    procedure: 0
    loop: 0

    #TI=$(($1*4))
    #TJ=$(($2*4))
    #TK=$(($3*4))
    TI=$(($1*1))
    TJ=$(($2*1))
    TK=$(($3*1))
    UI=$4
    US=$5

    permute([2,3,1,4])
    #known(hypre__nz<=10)
    #known(hypre__nz>=10)
    tile(0,4,TI)
    tile(0,3,TJ)
    tile(0,3,TK)
    unroll(0,6,US)
    unroll(0,7,UI)
    #print

    " > temp.script;

    # output filename
    out_file=OUT__1__6119__$1_$2_$3_$4_$5.so

    # Run chill and the post-processor
    chill temp.script;
    s2c OUT__1__6119__.lxf > OUT__1__6119__temp.c;

    cat OUT__1__6119__temp.c | sed 's/__out_argv\[15l\]/(int (\*)\[15\]) __out_argv\[15l\]/g' >  OUT__1__6119__temp_2.c
    cat OUT__1__6119__temp_2.c | sed 's/extern void/extern \"C\" void/g' > OUT__1__6119__.c

    rm OUT__1__6119__temp*.c
    # generate the .so library and copy it to the necessary directory

    if [ $useICC -eq 1 ]; then
        #echo "using icc"
        icc -c -fpic OUT__1__6119__.c  -O3 -xN -unroll0
        icc -shared -lc -o  OUT__1__6119__.so OUT__1__6119__.o -xN -unroll0
        echo "done creating the shared library"
    else
        #echo "using gcc"
        g++ -c -fpic OUT__1__6119__.c  -O2 -march=pentium4 -mmmx -msse -msse2 -mfpmath=sse
        #g++ -shared -lc -o  OUT__1__6119__.so OUT__1__6119__.o -O2 -march=pentium4 -mmmx -msse -msse2 -mfpmath=sse
        g++ -shared -lc -o  $out_file  OUT__1__6119__.o -O2 -march=pentium4 -mmmx -msse -msse2 -mfpmath=sse
	mv $out_file ../new_code/
        echo "$6 is done creating the shared library"
    fi
fi
