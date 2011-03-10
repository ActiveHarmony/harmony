#!/bin/bash

appname=$1

cd $HOME/scratch/new_code_${appname}

tar -cvf all_code_${appname}.tar *

bzip2 all_code_${appname}.tar 

echo "done zipping"
#Wood
scp all_code_${appname}.tar.bz2 rahulp@wood:~/scratch/code/

#Spoon
scp all_code_${appname}.tar.bz2 rahulp@spoon:~/scratch/code/

#hopper
#scp all_code_${appname}.tar.bz2 rahulp@hopper.nersc.gov:/global/scratch/sd/rahulp/code/temp
#echo "done transfering .bz2"

#carver
#scp all_code_${appname}.tar.bz2 rahulp@hopper.nersc.gov:/global/scratch/sd/rahulp/code
#echo "done transfering .bz2"

#generic
#rm *.so



########################## TRANSPORT ####################################


#hopper
rm -rf *.o *.c *.so
rm -rf all_code_${appname}.tar.bz2


# Wood
ssh rahulp@wood tar -xjf /fs/mashie/rahulp/scratch/code/all_code_${appname}.tar.bz2 -C /fs/mashie/rahulp/scratch/code/
ssh rahulp@wood rm /fs/mashie/rahulp/scratch/code/all_code_${appname}.tar.bz2

# Spoon
ssh rahulp@spoon tar -xjf /fs/mashie/rahulp/scratch/code/all_code_${appname}.tar.bz2 -C /fs/mashie/rahulp/scratch/code/
ssh rahulp@spoon rm /fs/mashie/rahulp/scratch/code/all_code_${appname}.tar.bz2

#carver
#ssh rahulp@hopper.nersc.gov 'tar -xjf /global/scratch/sd/rahulp/code/all_code_${appname}.tar.bz2 -C /global/scratch/sd/rahulp/code; rm /global/scratch/sd/rahulp/code/all_code_${appname}.tar.bz2 ' > /dev/null 2>&1
#echo "done"

#hopper
#ssh rahulp@hopper.nersc.gov 'tar -xjf /global/scratch/sd/rahulp/code/temp/all_code_${appname}.tar.bz2 -C /global/scratch/sd/rahulp/code/temp/; cd $SCRATCH2/code/temp; gmake -j 20; rm /global/scratch/sd/rahulp/code/temp/all_code_${appname}.tar.bz2 ' > /dev/null 2>&1
#echo "done"

