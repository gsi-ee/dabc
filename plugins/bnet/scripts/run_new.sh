#!/bin/bash

echo "Run bnet_exe"
echo "  Usage: run_new.sh [u11|b11|u12|b12 [kill|gdb|valgrind|log]]"

usegdb=false

NODEFILE=./nodes.txt

numprocs=`cat $NODEFILE | wc -l`
nodeslist=`cat $NODEFILE`

thisdir=`pwd`

uselog=truee

firstarg=""
firstsleep=1

# 11 - sockets, 22 - verbs, 12 - socket(contr) and verbs (traffic)
devicesID=b11
if [[ "x$1" != "x" ]]
then
   devicesID=$1
fi

if [[ "$2" == gdb ]]
then
   usegdb=true
   uselog=true
   firstarg="gdb --command=gdbcmd.txt --args"
fi

if [[ "$2" == valgrind ]]
then
   usevalgrind=true
   uselog=true
   firstarg="valgrind --leak-check=full"
   firstsleep=5
fi

if [[ "$2" == log ]]
then
   uselog=true
fi

userlibs='TestBnetWorker libBnetTest.so'
clusterlibs=BnetCluster 

if [[ "$devicesID" == u* ]]
then
  numprocs=`expr $numprocs + $numprocs`
  nodeslist=`echo $nodeslist $nodeslist`
  echo "Using unidirectional case $numprocs $nodeslist"
fi

if [[ "$devicesID" == *2 ]]
then
  userlibs+=' libDabcVerbs.so' 
  echo "Using verbs for data transport"
fi

numprocs=`expr $numprocs + 1`

lastnode=""
lastcallargs=""
procfilename=""
controllerID=""

counter=0
for node in $nodeslist
do

#  start cluster controller on the first node
 
   if [[ "$counter" == 0 ]] 
   then
      echo "Starting cluster plugin"
      
      if [[ $uselog == true ]]
      then
         logfile=">/tmp/bnet_exe_cluter 2>&1"
      else
         logfile=""
      fi   

      controllerID="$thisdir/serverid.txt"
      
      ssh $node "rm -f $controllerID"

      ssh $node "cd $thisdir; export LD_LIBRARY_PATH=.; $firstarg ./bnet_exe $counter $numprocs $devicesID $controllerID $clusterlibs $logfile" &
      
      counter=`expr $counter + 1`

      sleep $firstsleep
      controllerID=`cat $controllerID`
      
      if [[ "x$controllerID" == "x" ]]
      then
         echo "cannot get controller id"
         exit
      fi
   fi

   if [[ $uselog == true ]]
   then
      logfile=">/tmp/bnet_exe.$counter 2>&1"
   else
      logfile=""
   fi   

   echo $node $counter $numprocs $devicesID $controllerID $userlibs

   callargs="cd $thisdir; export LD_LIBRARY_PATH=.; $firstarg ./bnet_exe $counter $numprocs $devicesID $controllerID $userlibs $logfile"
   
   procfilename="procid.$counter"

   ssh $node "$callargs & jobs -p > $procfilename" &

   counter=`expr $counter + 1`
   
   lastnode=$node
   lastcallargs=$callargs
done

if [[ "$2" == "kill" ]]
then

  echo "We will kill and restore last application !!!!"
  #wait some time
  sleep 5

  lastprocid=`cat $procfilename`
  echo "last procid = $lastprocid - kill it !!!" 
  echo "$lastnode kill -9 $lastprocid"
  ssh $lastnode kill -9 $lastprocid

  #wait some time
  sleep 5
  echo "Restart killed application"
  echo "$lastnode $lastcallargs"
  ssh $lastnode $lastcallargs &
fi


