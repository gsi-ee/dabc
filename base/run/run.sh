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

currdir=`pwd`

###########################################################
# first loop, where only test login to the nodes are done
###########################################################

for (( counter=0; counter<numnodes; counter=counter+1 ))
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
done

if [[ "$2" == "test" ]]
then
   echo "We only did tests of login"
   exit 0
fi

connstr=file.id

for (( counter=0; counter<numnodes; counter=counter+1 ))
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

   ####################################################################
   # this is special part to get connection string from first node
   # only required for SimpleControl, for DIM control must be deactivated
   ####################################################################
   
   if (( counter == 0 ))
   then
      callargs=`$dabc_xml $XMLFILE -id $counter -workdir $currdir -conn $connstr -sshconn`

      echo $callargs
      
      for (( cnt=10; cnt>0; cnt=cnt-1)) ; do 
         connstr=`$callargs`
         retval=$?
         if [ $retval -ne 0 ]; then
            echo "Get connect information fails err = $retval"
            exit $retval
         fi
         if [[ "x$connstr" == "x" ]] ; then sleep 1; else cnt=0; fi
      done 
   
      echo "New connection string is $connstr"
   fi
done
