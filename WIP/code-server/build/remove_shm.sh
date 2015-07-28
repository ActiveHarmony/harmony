#!/bin/bash 
#
# Copyright 2003-2015 Jeffrey K. Hollingsworth
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
if [ $# -lt 1 ]
then
   echo "must specify regex for process name"
else

    ipcs -m | grep "tiwari"| awk '{print $2}' > shm_ids

    while read line
    do
        ipcrm shm $line
    done < 'shm_ids'

    ipcs -s | grep "tiwari"| awk '{print $2}' > sem_ids

    while read line
    do
        ipcrm sem $line
    done < 'sem_ids'

fi
