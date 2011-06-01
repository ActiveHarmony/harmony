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

# top-level comments
# all the files required for code generation (suif files, for example) should be 
#  provided. For now, we will just look for required_files.dat in the appname directory


# code_generator_host should be set to the hostname of the machine the the generator is running
#  on.
code_generator_host=$HOSTNAME

# please set this to where the generator is installed
code_generator_base="$HOME/activeharmony/tutorial-2010/harmony/standalone/"

# first argument to the script is the name of the application we are generating code for
appname=$1

# generated codes are stored in $HOME/scratch directory
scratch_dir="\$HOME/scratch/"
 
# How do we want to transfer the files to the remote host? We rely on script to do this.
# Each application will have its own transport_files.sh.
cp $code_generator_base/$appname/transport_files.sh .


# all hosts available for code_generation are stored in generator_hosts_<appname> file.
# the format of this file is the following:
#  host <num_instances>
# For example if we have two dual core machine (named foo and bar) and we want to use all four
    #  cores, the generator_hosts file will look like the following:
#  wood 2
#  spoon 2

# first parse the application specific host file.
`./parse_host_file generator_hosts_${appname} > temp_hosts`

node_list=`cat temp_hosts`

if [[ $node_list == *Unable* ]]
then
    echo "Unable to open generator_hosts_${appname}. Please make sure you have created this file.";
    echo "Cannot proceed further. Baling out."
    exit
fi

if [[ $node_list == "" ]]
then
    echo "generator_hosts_${appname} is empty. The format is as follows:"
    echo "If we have two dual core machine (named driver and spoon) and we want to use all four"
    echo "  cores to generate code for an application baz, then generator_hosts_baz should"
    echo "  look like the following:"
    echo "  driver 2"
    echo "  spoon 2"
    echo "  with each host in its own line."
    echo "Cannot proceed further. Baling out."
    exit
fi

node_list=`cat temp_hosts`

echo "$node_list"


unique_dirs_created_local=0
for __node in $node_list
do
    
    machine_name=`echo $__node | cut -d'_' -f 1`
    echo $machine_name

    # make the directories
    # copy the relevant files
    dir_string=""
    if [ $machine_name == $code_generator_host ]; then

	if [ $unique_dirs_created_local == 0 ]; then
	    dir_string="$HOME/scratch/new_code_${appname}"
	    echo "Creating $dir_string"
	    mkdir $dir_string
	    dir_string="$HOME/scratch/zipped_${appname}"
	    echo "Creating $dir_string"
	    mkdir $dir_string
	    unique_dirs_created_local=1
	fi
	
	dir_string="$HOME/scratch/${__node}_${appname}"
	echo "Creating $dir_string"
	mkdir $dir_string
	cp $appname/chill_script.${appname}.sh ~/scratch/${__node}_${appname}
	while read line
	do
	    #echo $line
	    # comments in the required_files.dat begin with #
	    if [[ $line != *#* ]]; then
		cp $appname/$line ~/scratch/${__node}_${appname}
	    fi
	done < "$appname/required_files.dat"
    else
	#echo "copying remote version"
	dir_string="/fs/mashie/rahulp/scratch/new_code_${appname}"
	ssh $machine_name "mkdir $dir_string"
	dir_string="/fs/mashie/rahulp/scratch/zipped_${appname}"
        ssh $machine_name "mkdir $dir_string"
        dir_string="/fs/mashie/rahulp/scratch/${__node}_${appname}"
	ssh $machine_name "mkdir $dir_string"
	scp $appname/chill_script.${appname}.sh $machine_name:~/scratch/${__node}_${appname}/chill_script.${appname}.sh
	while read line
	do
	    # comments in the required_files.dat begin with #
	    if [[ $line  != *#* ]]; then
		scp $appname/$line $machine_name:~/scratch/${__node}_${appname}
	    fi
	done < "$appname/required_files.dat"
	
    fi
done
