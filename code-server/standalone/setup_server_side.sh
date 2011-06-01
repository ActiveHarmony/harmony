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
code_generator_host=$HOSTNAME
code_generator_base="$HOME/standalone/"
appname=smg2000
scratch_dir="\$HOME/scratch/"


copy_chill=$2

#replace the file.list
cp $code_generator_base/$appname/file.list .
cp $code_generator_base/$appname/transport_files.sh .

node_list=`cat generator_hosts`

for __node in $node_list
do
    
    machine_name=`echo $__node | cut -d'_' -f 1`
    echo $machine_name

    # make the directories
    # copy the relevant files
    dir_string=""
    #dir_string="\$HOME/scratch/$__node"
    #ssh $machine_name "rm -rf $dir_string"
    #ssh $machine_name "mkdir $dir_string"
    if [ $machine_name == $code_generator_host ]; then
	dir_string="$HOME/scratch/$__node"
	rm -rf $dir_string
	mkdir $dir_string
	cp $appname/chill_script.${appname}.sh ~/scratch/$__node
	while read line
	do
	    cp $appname/$line ~/scratch/$__node
	done < "$appname/file.list"
    else
	echo "copying remote version"
	dir_string="\$HOME/scratch/$__node"
	ssh $machine_name "rm -rf $dir_string"
	ssh $machine_name "mkdir $dir_string"
	scp $appname/chill_script.${appname}.remote.sh $machine_name:~/scratch/$__node/chill_script.${appname}.sh
	while read line
	do
	    scp $appname/$line $machine_name:~/scratch/$__node
	done < "$appname/file.list"
	
    fi
    #while read line
    #do
    #  scp $appname/$line $machine_name:~/scratch/$__node
    #done < "$appname/file.list"

done

#./code_generator
#echo 1




# clean-up
#rm -rf ${host_dir}/*_[0-9]*
#rm -rf ${search_dir}/confs/*
#rm -rf ${app_dir}/code/*
#rm -rf ${harmony_dir}/bin/code_flags/*
#rm -rf ${search_dir}/code_generation.log
#echo "Code server is ready ... " > ${search_dir}/code_generation.log
#cat $PBS_NODEFILE > generator_hosts


#first_node=1
#cd ${search_dir}
#pwd
#export CLASSPATH=$PWD
##java Process_hosts generator_hosts > generator_hosts_processed
#node_list=`cat generator_hosts_processed`


#for bug_node in $node_list
#do
#   mkdir $host_dir/$bug_node
#   while read line
#   do
#      cp $line $host_dir/$bug_node/
#   done < 'file.list'
#done

#cd $search_dir
#./code_generator

