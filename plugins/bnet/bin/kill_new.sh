#!/bin/bash
#
NODEFILE=nodes.txt

for i in `cat $NODEFILE`
do
   echo $i
   ssh $i killall bnet_exe
done
