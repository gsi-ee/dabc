#!/bin/sh

echo "Run test1"

NODEFILE=./mynodes.txt

thisdir=`pwd`

numprocs=`cat $NODEFILE | wc -l`

controllerID="$thisdir/serverid.txt"

# 11 - sockets, 22 - verbs, 12 - socket(contr) and verbs (traffic)
devicesID=11

if [ "x$1" != "x" ]; then
   devicesID=$1
fi

devicename=ib0

if [[ "$devicesID" == *2 ]]
then
  devicename=ib0
fi

#logfile=">/tmp/test1.log 2>&1"

counter=0
for node in `cat $NODEFILE`
do
   
#   ipaddr=`ssh $node /sbin/ifconfig $devicename | grep 'inet addr:'| cut -d: -f2 | awk '{ print $1}'`
  
   echo $node $counter $numprocs $devicesID $controllerID

   ssh $node "cd $thisdir; export LD_LIBRARY_PATH=.; ./test1 $counter $numprocs $devicesID $controllerID $logfile" &

#   ssh $node "cd /u/linev/run/test2; taskset 0x00000002 ./wastecpu" &
#   ssh $node "cd /u/linev/run/test2; taskset 0x00000001 ./wastecpu" &

   if [ "$counter" = "0" ]; then
      sleep 1
      controllerID=`cat $controllerID`
   fi
   counter=`expr $counter + 1`
done
