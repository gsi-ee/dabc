#!/bin/sh

echo "Run test2"

NODEFILE=./mynodes.txt

thisdir=`pwd`

numprocs=`cat $NODEFILE | wc -l`

controllerID="$thisdir/serverid.txt"

# 11 - sockets, 22 - verbs, 12 - socket(contr) and verbs (traffic)
devicesID=11

if [ "x$1" != "x" ]; then
   devicesID=$1
fi

#firstarg="gdb --command=gdbcmd.txt --args"


counter=0
for node in `cat $NODEFILE`
do
   echo $node $counter $numprocs $devicesID $controllerID
   
   callargs="cd $thisdir; export LD_LIBRARY_PATH=.; $firstarg ./test2 $counter $numprocs $devicesID $controllerID"

   ssh $node $callargs &

#   ssh $node "cd /u/linev/run/test2; taskset 0x00000002 ./wastecpu" &
#   ssh $node "cd /u/linev/run/test2; taskset 0x00000001 ./wastecpu" &

   if [ "$counter" = "0" ]; then
      sleep 1
      controllerID=`cat $controllerID`
   fi
   counter=`expr $counter + 1`
done
