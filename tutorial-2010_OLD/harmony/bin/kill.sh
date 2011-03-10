if [ $# -lt 1 ]
then
   echo "must specify regex for process name"
else

    ps -ef | grep $1 | awk '{print $2}' > pids

    while read line
    do
        kill $line
    done < 'pids'

fi
