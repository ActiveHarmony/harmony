#!/bin/sh
#
# Copyright 2003-2013 Jeffrey K. Hollingsworth
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

#PBS -l nodes=1,walltime=00:01:00
#PBS -N APPTest_2
#PBS -o APP.result.2
#PBS -e APP.error.2
#PBS -V

num_samples=3

# change this to your directory.
cd /hivehomes/tiwari/schedular
lamboot
rm -rf temp_result.2
rm -rf timings.2
for i in $(seq 1 1 ${num_samples}); do
    mpirun -np 1 ./application 9 >> temp_result.2;
done

# process the results
cat temp_result.2 | awk '{print $2}' | sort -g | head -1 > timings.2

touch done.iter.2

