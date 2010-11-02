#!/bin/bash


# command line arguments
#  the first argument indicates whether or not to produce new code
#  the second argument indicates the configuration is a primary or 
#   secondary (from speculative code generation)
#  the third argument is the name of the application we are trying to tune. Note that
#   this name needs to match the name of the directory in the /scripts directory.
#  the remaining arguments are for code generation.

primary_or_secondary=$1
produce_code=$2
app_name=$3

# if the transformation parameters need to be mapped to another domain, for example
#   if we are looking at tile sizes that are multiples of 4, then fo the mapping
#   here.

TI=$(($4*4))
TJ=$(($5*4))
TK=$(($6*4))
UI=$7
US=$8

# exports: indicate where the CHiLL components and libraries are

export OMEGA_P=$HOME/omega
export SUIFHOME=$HOME/suifhome

export PATH=$HOME/chill/bin:${SUIFHOME}/i386-linux/bin/:${PATH}
export LD_LIBRARY_PATH=${SUIFHOME}/i386-linux/solib:${LD_LIBRARY_PATH}

# Working directory
WORK_DIR=$HOME/code-server/scratch_space/tmp/${app_name}
TRANSPORT_DIR=$HOME/code-server/scratch_space/tmp/${app_name}/transport/
# cd into the work_directory

cd $WORK_DIR

# set the compilers and optimization flags
useICC=0
useGCC=1
GCC_COMMAND="g++ -O2 -march=pentium4 -mmmx -msse -msse2 -mfpmath=sse "
ICC_COMMAND="icc -O3 -xN -unroll0 "

# debugging
echo "Currently generating code for configuration: TI: $TI, TJ: $TJ, TK: $TK, UI: $UI, US: $US"

# remove previous variant of the kernel and the result

rm -rf OUT__1__6119__.so OUT__1__6229__.c temp.script peri.result.overall peri.result.iters

# output filename
out_file=OUT__1__6119__${TI}_${TJ}_${TK}_${UI}_${US}.so

if [ $produce_code -eq 1 ]; then
    # generate a new CHiLL script using the transformation parameters
    echo "
    source: OUT__1__6119__.spd
    procedure: 0
    loop: 0

    TI=$TI
    TJ=$TJ
    TK=$TK
    UI=$UI
    US=$US

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

    # Run chill and the post-processor
    chill temp.script;
    s2c OUT__1__6119__.lxf > OUT__1__6119__temp.c;

    cat OUT__1__6119__temp.c | sed 's/__out_argv\[15l\]/(int (\*)\[15\]) __out_argv\[15l\]/g' >  OUT__1__6119__temp_2.c
    cat OUT__1__6119__temp_2.c | sed 's/extern void/extern \"C\" void/g' > OUT__1__6119__.c

    rm OUT__1__6119__temp*.c
    # generate the .so library and copy it to the necessary directory

    if [ $useICC -eq 1 ]; then
        #echo "using icc"
        $ICC_COMMAND -c -fpic OUT__1__6119__.c
        icc -shared -lc -o  OUT__1__6119__.so OUT__1__6119__.o
	mv $out_file ../new_code/
	if [ $primary_or_secondary -eq 1 ] then
	    # this is primary conf, move this to the transport dir
	    cp $WORK_DIR/new_code/${out_file} $TRANSPORT_DIR
	fi
        echo "done creating the shared library"
    fi
    if [ $useGCC -eq 1 ]; then
        echo "using gcc"
        $GCC_COMMAND -c -fpic OUT__1__6119__.c
        $GCC_COMMAND -shared -lc -o  $out_file  OUT__1__6119__.o
	mv $out_file ../new_code/
	if [ $primary_or_secondary -eq 1 ] then
	    # this is primary conf, move this to the transport dir
	    cp $WORK_DIR/new_code/${out_file} $TRANSPORT_DIR
	fi
        echo "$6 is done creating the shared library"
    fi

fi

if [ $produce_code -eq 0 ] then

    # simply move the out_file to transport directory
    cp $WORK_DIR/new_code/${out_file} $TRANSPORT_DIR
fi
