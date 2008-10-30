#!/bin/sh
#
NODEFILE=mynodes.txt

for i in `cat $NODEFILE`
do
   echo $i
   ssh $i killall test1
   ssh $i killall wastecpu
done
