#!/bin/bash

# the first parameter is the parameter to the application
# the assumption here is that the first N parameters are the tunable
# parameters. So if the application has three parameters, the first
# three arguments to this script will be the tunable parameters.
p1=$1

# the second parameter is the application id
appid=$2

# the third parameter is the number of samples
# how many times do we want to run the application
num_samples=$3

# to keep this example simple, we are only running
#  this application on 1 processor. This will most
#  certainly change for "real" applications.

# for now, running the application on only one node

# if you want to run a real application, make changes to the lnodes
# PBS parameter, changes to -np parameter for mpirun, and ./application
# should be replaced with the application (along with the parameters
# for the application).

num_node=1
echo "
#PBS -l nodes=${num_node},walltime=00:01:00
#PBS -N APPTest_${appid}
#PBS -o APP.result.${appid}
#PBS -e APP.error.${appid}
#PBS -V

num_samples=${num_samples}

# change this to your directory.
cd /hivehomes/tiwari/schedular
lamboot
rm -rf temp_result.${appid}
rm -rf timings.${appid}
for i in \$(seq 1 1 \${num_samples}); do
    mpirun -np 1 ./application ${p1} >> temp_result.${appid};
done

# process the results
cat temp_result.${appid} | awk '{print \$2}' | sort -g | head -1 > timings.${appid}

touch done.iter.${appid}
"
