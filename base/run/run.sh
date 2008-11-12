#!/bin/bash

echo "Shell script to run dabc application in simple (without XDAQ/dim) environment"
echo "  Usage: run.sh filename.xml [test|kill]"

XMLFILE=$1

#arg1="ssh lxi002"
#arg='. gsi_environment.sh; echo $HOST : $HOST; ls'
#$arg1 $arg
#exit 0


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

dabc_xml=`which dabc_xml`
if [[ "x$dabc_xml" == "x" ]]
then
   echo "Cannot find dabc_xml executable"
   exit 1
fi


numnodes=`$dabc_xml $XMLFILE -number`

retval=$?
if [ $retval -ne 0 ]; then
   echo "Cannot identify number of nodes in $XMLFILE - ret = $retval syntax error?"
   exit $retval
fi

if [[ "x$numnodes" == "x" || "$numnodes" == "0" ]] 
then
   echo "Internal error in dabc_run - cannot identify numnodes in $XMLFILE"
   exit 1
fi

echo "Total numnodes = $numnodes, check all of them that we can log in"

counter=0

currdir=`pwd`

while [[ "$counter" != "$numnodes" ]]
do
   callargs=`$dabc_xml $XMLFILE -id $counter -workdir $currdir -sshtest`
   retval=$?
   if [ $retval -ne 0 ]; then
      echo "Cannot identify test call args for node $counter  err = $retval"
      exit $retval
   fi
   
#   echo $callargs
   
   $callargs

   retval=$?
   if [ $retval -ne 0 ]; then
      echo "Test sequence for node $counter fail err = $retval"
      exit $retval
   fi

   counter=`expr $counter + 1`
done

if [[ "$2" == "test" ]]
then
   echo "We only did tests of login"
   exit 0
fi

counter=0
connstr=file.id

while [[ "$counter" != "$numnodes" ]]
do
   callargs=`$dabc_xml $XMLFILE -id $counter -workdir $currdir -conn $connstr -sshrun`
   retval=$?
   if [ $retval -ne 0 ]; then
      echo "Cannot identify test call args for node $counter  err = $retval"
      exit $retval
   fi
   
   echo $callargs
   
   $callargs &

   retval=$?
   if [ $retval -ne 0 ]; then
      echo "Run of dabc application for node $counter fails err = $retval"
      exit $retval
   fi
   
   if [[ "$counter" == "0" ]]
   then
      callargs=`$dabc_xml $XMLFILE -id $counter -workdir $currdir -conn $connstr -sshconn`

      echo $callargs
      
      sleep 3
      
      connstr=`$callargs`
   
      echo "New connection string is $connstr"
      
      exit 1 
   fi
   
   counter=`expr $counter + 1`
done

exit 0


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
