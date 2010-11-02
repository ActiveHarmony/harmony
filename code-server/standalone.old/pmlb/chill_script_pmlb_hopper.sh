#!/bin/bash

WORK_DIR=/fs/spoon/tiwari/code_generator/hosts/${7}
DEST_DIR=/hivehomes/tiwari/pmlb/fusion_ex/code/
# cd into proper directory
cd $WORK_DIR
#source exports.sh
export OMEGA_P=/fs/spoon/tiwari/omega
export SUIFHOME=/fs/spoon/tiwari/suifhome
#export OPENMPI=/usr/local/stow/openmpi-1.3.3-gm

export PATH=/fs/spoon/tiwari/chill/bin:${SUIFHOME}/i386-linux/bin/:${PATH}
export LD_LIBRARY_PATH=${SUIFHOME}/i386-linux/solib:${LD_LIBRARY_PATH}

#export PATH=/opt/intel/cc/10.0.026/bin/:/opt/intel/fc/10.0.026/bin/:$PATH
# Use CHiLL? If you don't have a working version of the chill, set this
#  flag to 0.
useChill=1
#useICC=$6
useICC=0
# the unroll factor comes in as the first parameter
echo "Currently generating code for configuration: TI: $1, TJ: $2, UK: $3,TI: $4, TJ: $5, UK: $6"

rm -rf streaming_all.so streaming_all.c temp_*.script peri.result.overall peri.result.iters

if [ $useChill -eq 1 ]; then
    # generate the new version using the given unroll factor by CHiLL
    # First, generate the CHiLL script
    echo "
    source: streaming_1_3_5.spd
    procedure: 0
    loop: 0

    TI=$(($1*4))
    TJ=$(($2*4))
    #TK=$(($3*4))
    UK=$3

    original()
    known(imax>1)
    known(jmax>1)
    known(kmax>1)
    tile(0, 1, TI)
    tile(0, 3, TJ)
    #tile(0, 5, TK)
    unroll(0, 5, UK)
    #print code
    " > temp_1.script;

    echo "
    source: streaming_2_4.spd
    procedure: 0
    loop: 0
    TI=$(($4*4))
    TJ=$(($5*4))
    #TK=$(($6*4))
    UK=$6

    original()
    known(imax>1)
    known(jmax>1)
    known(kmax>1)
    tile(0, 1, TI)
    tile(0, 3, TJ)
    #tile(0, 5, TK)
    unroll(0, 5, UK)
    #print code
    " > temp_2.script;

 #  echo "
 #   source: streaming_3.spd
 #   procedure: 0
 #   loop: 0
 #   TI=$(($4*4))
 #   TJ=$(($5*4))
 #   #TK=$(($6*4))
 #   UK=$6
#
#    original()
#    known(imax>1)
#    known(jmax>1)
#    known(kmax>1)
#    tile(0, 1, TI)
#    tile(0, 3, TJ)
#    #tile(0, 5, TK)
#    unroll(0, 5, UK)
#    #print code
#    " > temp_3.script;

    

 #   echo "
 #   source: streaming_4.spd
 #   procedure: 0
 #   loop: 0
 #   TI=$(($4*4))
 #   TJ=$(($5*4))
 #   #TK=$(($6*4))
  #  UK=$6
#
 #   original()
 #   known(imax>1)
 #   known(jmax>1)
 #   known(kmax>1)
 #   tile(0, 1, TI)
 #   tile(0, 3, TJ)
 #   #tile(0, 5, TK)
 #   unroll(0, 5, UK)
 #   #print code
 #   " > temp_4.script;

    

 #   echo "
 #   source: streaming_5.spd
 #   procedure: 0
 #   loop: 0
#
 #   TI=$(($4*4))
 #   TJ=$(($5*4))
 #   #TK=$(($6*4))
  #  UK=$6
#
 #   original()
  #  known(imax>1)
 #   known(jmax>1)
 #   known(kmax>1)
 #   tile(0, 1, TI)
 #   tile(0, 3, TJ)
 #   #tile(0, 5, TK)
 #   unroll(0, 5, UK)
  #  #print code
 #   " > temp_5.script;


    # output filename
    out_file=streaming_all_$1_$2_$3_$4_$5_$6.so

    # Run chill and the post-processor
    chill temp_1.script;
    s2c streaming_1_3_5.lxf streaming_1_3_5.chill.c;

    chill temp_2.script;
    s2c streaming_2_4.lxf streaming_2_4.chill.c;

#    chill temp_3.script;
#    s2c streaming_3.lxf streaming_3.chill.c;

#    chill temp_4.script;
#    s2c streaming_4.lxf streaming_4.chill.c;

#    chill temp_5.script;
#    s2c streaming_5.lxf streaming_5.chill.c;

    # combine the files
    #sed -n '/imax = /,$p' streaming_1_3.chill.c | sed 's/return;//g' | awk NF | sed '$d' > transformed_kernel.1
    #sed -n '/imax = /,$p' streaming_2.chill.c | sed 's/return;//g'   | awk NF | sed '$d' > transformed_kernel.2
    #sed -n '/imax = /,$p' streaming_4.chill.c | sed 's/return;//g'   | awk NF | sed '$d' > transformed_kernel.3
    #sed -n '/imax = /,$p' streaming_5.chill.c | sed 's/return;//g'   | awk NF | sed '$d' > transformed_kernel.4
    #cat header.txt > streaming_all.c
    #for i in $(seq 1 4);
    #do
    #  cat transformed_kernel.$i >> streaming_all.c
    #done

    #echo "}" >> streaming_all.c

    #rm transformed_kernel.* 

    # generate the .so library and copy it to the necessary directory

    if [ $useICC -eq 1 ]; then
        icc -g -pg -fpic -c -o streaming_1_3.o streaming_1_3.chill.c -O3 -xN -unroll0
        icc -g -pg -fpic -c -o streaming_2.o streaming_2.chill.c -O3 -O3 -xN -unroll0
        icc -g -pg -fpic -c -o streaming_4.o streaming_4.chill.c -O3 -O3 -xN -unroll0
        icc -g -pg -fpic -c -o streaming_5.o streaming_5.chill.c -O3 -O3 -xN -unroll0
        icc --shared -lc -o $DEST_DIR/$out_file streaming_1_3.o streaming_2.o streaming_4.o streaming_5.o 
        echo "$7 is done creating the shared library using icc"
        #echo "using icc"
        #icc -c -fpic OUT__1__6119__.c  -O3 -xN -unroll0
        #icc -shared -lc -o  OUT__1__6119__.so OUT__1__6119__.o -xN -unroll0
        ###echo "done creating the shared library"
    else
        #echo "using gcc"
        gcc  -fpic -c -o streaming_1_3_5.o streaming_1_3_5.chill.c -O3 -msse2
        gcc  -fpic -c -o streaming_2_4.o streaming_2_4.chill.c -O3 -msse2
        gcc --shared -lc -o /fs/spoon/tiwari/code_generator/lib_code_cray/$out_file streaming_1_3_5.o streaming_2_4.o
        
        #gcc  -fpic -c -o streaming_1.o streaming_1.chill.c -O3 -msse2
        #gcc  -fpic -c -o streaming_3.o streaming_3.chill.c -O3 -msse2
        #gcc  -fpic -c -o streaming_2.o streaming_2.chill.c -O3 -msse2
        #gcc  -fpic -c -o streaming_4.o streaming_4.chill.c -O3 -msse2
        #gcc  -fpic -c -o streaming_5.o streaming_5.chill.c -O3 -msse2
        #gcc --shared -lc -o /scratch2/scratchdirs/tiwari/code/$out_file streaming_1.o streaming_3.o streaming_2.o streaming_4.o streaming_5.o
	#scp ${out_file} tiwari@brood00:~/pmlb/fusion_ex/code
        echo "$7 is done creating the shared library"
    fi
fi
 
