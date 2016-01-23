#!/bin/bash
echo "YEAH BITCH"
for i in `seq 1 5`;
do
	./config ASST$i
	cd ../compile/ASST$i
	bmake depend
	bmake
	bmake install
	cd ~cs431-os161/os161-1.99/kern/conf
done
echo "FINISHED BITCH!"
