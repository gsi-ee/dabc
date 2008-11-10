#!/bin/bash

echo "Shell script to run dabc application in simple (without XDAQ/dim) environment"
echo "  Usage: run.sh filename.xml [test|kill]"

XMLFILE=$1

if [[ "x$XMLFILE" == "x" ]]
then
   echo "XML file is not specified"
   exit 1
fi

curdir=`pwd`

if [[ "x$DABCSYS" == "x" ]]
then
   DABCSYS=$curdir
   echo "DABCSYS not specified, use current dir $DABCSYS"
fi

numnodes=`$DABCSYS/bin/dabc_run $XMLFILE -number`

if [[ "x$numnodes" == "x" || "$numnodes" == "0" ]] 
then
   echo "cannot identify numnodes in $XMLFILE"
   exit 1
fi


echo "Total numnodes = $numnodes, check all of then that we can log in"

counter=0

while [[ "$counter" != "$numnodes" ]]
do
   nodename=`$DABCSYS/bin/dabc_run $XMLFILE -name$counter`
   
   if [[ "$2" == "kill" ]]
   then
     echo "Node $counter is $nodename, kill all dabc_run applications"
     ssh $nodename "killall dabc_run"
   else
      echo "Node $counter is $nodename, try to login"
      ssh $nodename "echo Hallo from $nodename"
   fi
   counter=`expr $counter + 1`
done

if [[ "$2" == "test" ]]
then
   echo "We only did tests of login"
   exit 0
fi

if [[ "$2" == "kill" ]]
then
   echo "We only did kills"
   exit 0
fi


echo "Now try to run DABC an all tested nodes"

counter=0

connstr=file.id
rm -f $connstr 

while [[ "$counter" != "$numnodes" ]]
do
   nodename=`$DABCSYS/bin/dabc_run $XMLFILE -name$counter`
   echo "Node $counter is $nodename"
   callargs="cd $curdir; export DABCSYS=$DABCSYS; export LD_LIBRARY_PATH=$DABCSYS/lib; $DABCSYS/bin/dabc_run $XMLFILE -id$counter -num$numnodes -conn$connstr"
   echo $callargs
   
   ssh $nodename "$callargs" &
   
   if [[ "$counter" == "0" ]]
   then
      waitcnt=0
      while [[ ! -f $connstr ]] 
      do
         sleep 1
         waitcnt=`expr $waitcnt + 1`
         if [[ "$waitcnt" == "10" ]] 
         then
            echo "No connection info after 10 sec"
            exit 1 
         fi
      done
      connstr=`cat $connstr`
      echo "Connection id for master = $connstr  wait $waitcnt"
      rm -f file.id
   fi
   
#   ssh $nodename "echo hello" &
   
   counter=`expr $counter + 1`
done

exit 0
