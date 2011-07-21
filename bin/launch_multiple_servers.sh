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

 
