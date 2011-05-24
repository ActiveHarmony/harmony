#!/bin/bash
#
# Copyright 2003-2011 Jeffrey K. Hollingsworth
#
# This file is part of Active Harmony.
#
# Active Harmony is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Active Harmony is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Active Harmony.  If not, see <http://www.gnu.org/licenses/>.
#

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

