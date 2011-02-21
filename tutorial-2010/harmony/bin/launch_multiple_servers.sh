#!/bin/bash

if [ $# -lt 2 ]; then
	echo "usage: ./launch_multiple_servers.sh <num servers> <port_start>"
else
	num_servers=$1
	port=$2
	echo "launching $num_servers harmony servers starting at port number $port"
	for i in $(seq 1 1 $num_servers); do
	    export HARMONY_S_PORT=$port
	    ./hserver > hserver_${port} &
	    port=$((port+1))
	done
fi

 
