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
demo_home=/fs/spoon/tiwari/standalone/build
search_dir=${demo_home}
host_dir=${demo_home}/hosts
app_dir=${demo_home}


# clean-up
rm -rf ${host_dir}/*_*
rm -rf ${search_dir}/confs/*
rm -rf ${search_dir}/code_generation.log
echo "Code server is ready ... " > ${search_dir}/code_generation.log


first_node=1
cd ${search_dir}
pwd
export CLASSPATH=$PWD
node_list=`cat generator_hosts_processed`


for one_line in $node_list
do

   mkdir $host_dir/$one_line
   while read line
   do
      cp $line $host_dir/$bug_node/
   done < 'file.list'
done

#cd ${harmony_dir}/bin
#pwd
#./hserver &
cd $search_dir
./code_generator
 
