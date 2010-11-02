#!/bin/bash
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
 
