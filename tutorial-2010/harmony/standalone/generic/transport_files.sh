#!/bin/bash

appname=$1

cd $HOME/scratch/new_code_${appname}

tar -cvf all_code_${appname}.tar *

bzip2 all_code_${appname}.tar 

echo "done zipping"
# brood
scp all_code_${appname}.tar.bz2 tiwari@brood00:~/scratch/code/

#hopper
#scp all_code_${appname}.tar.bz2 tiwari@hopper.nersc.gov:/global/scratch/sd/tiwari/code/temp
#echo "done transfering .bz2"

#carver
#scp all_code_${appname}.tar.bz2 tiwari@hopper.nersc.gov:/global/scratch/sd/tiwari/code
#echo "done transfering .bz2"

#generic
#rm *.so



########################## TRANSPORT ####################################


#hopper
rm -rf *.o *.c *.so
rm -rf all_code_${appname}.tar.bz2


# brood
ssh tiwari@brood00 tar -xjf /hivehomes/tiwari/scratch/code/all_code_${appname}.tar.bz2 -C /hivehomes/tiwari/scratch/code/
ssh tiwari@brood00 rm /hivehomes/tiwari/scratch/code/all_code_${appname}.tar.bz2

#carver
#ssh tiwari@hopper.nersc.gov 'tar -xjf /global/scratch/sd/tiwari/code/all_code_${appname}.tar.bz2 -C /global/scratch/sd/tiwari/code; rm /global/scratch/sd/tiwari/code/all_code_${appname}.tar.bz2 ' > /dev/null 2>&1
#echo "done"



#hopper
#ssh tiwari@hopper.nersc.gov 'tar -xjf /global/scratch/sd/tiwari/code/temp/all_code_${appname}.tar.bz2 -C /global/scratch/sd/tiwari/code/temp/; cd $SCRATCH2/code/temp; gmake -j 20; rm /global/scratch/sd/tiwari/code/temp/all_code_${appname}.tar.bz2 ' > /dev/null 2>&1
#echo "done"

