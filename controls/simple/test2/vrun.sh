#!/bin/sh


echo "Run test2 with valgrind"

NODEFILE=./mynodes.txt

thisdir=`pwd`

numprocs=`cat $NODEFILE | wc -l`

controllerID="$thisdir/serverid.txt"

# 11 - sockets, 22 - verbs, 12 - socket(contr) and verbs (traffic)
devicesID=11

if [ "x$1" != "x" ]; then
   devicesID=$1
fi


counter=0

lastcounter=`expr $numprocs - 1`
echo "lastcounter = $lastcounter"

for node in `cat $NODEFILE`
do
   echo $node $counter $numprocs $devicesID $controllerID
   
   if [ "$counter" = "$lastcounter" ]; then
     valgrind --suppressions=valgrindsupp.txt --gen-suppressions=all --leak-check=full --log-file=vout.txt ./test2 $counter $numprocs $devicesID $controllerID
#     valgrind --suppressions=valgrindsupp.txt --leak-check=full --log-file=vout.txt ./test2 $counter $numprocs $devicesID $controllerID
   else
      ssh $node "cd $thisdir; export LD_LIBRARY_PATH=.; ./test2 $counter $numprocs $devicesID $controllerID" &
   fi

#   ssh $node "cd /u/linev/run/test2; taskset 0x00000002 ./wastecpu" &
#   ssh $node "cd /u/linev/run/test2; taskset 0x00000001 ./wastecpu" &

   if [ "$counter" = "0" ]; then
      sleep 1
      controllerID=`cat $controllerID`
   fi
   counter=`expr $counter + 1`
done
