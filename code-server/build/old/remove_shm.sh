#!/bin/bash 
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
