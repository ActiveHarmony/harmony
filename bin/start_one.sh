#!/bin/bash

cd /hivehomes/tiwari/harmony_pro_release/bin/
port=$1

export HARMONY_S_PORT=$port
./hserver > hserver_${port} &
