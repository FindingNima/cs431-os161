#!/bin/bash

asst_num="ASST$1"

cd ~/cs431-os161/os161-1.99/kern/conf
./config $asst_num
cd ../compile/$asst_num
bmake depend
bmake
bmake install
