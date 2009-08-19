#!/bin/bash


if [[ "$1" == "-version" ]] ; then
   echo "DABC version 1.03"
   exit 0
fi

echo "Shell script to run dabc application"

if [[ "$1" == "" || "$1" == "?" || "$1" == "-help" || "$1" == "-h" || "$1" == "/?" ]] ; then
   echo "Usage: run.sh filename.xml [run|start|stop|test|kill]  [-v|--verbose] [-dim|-sctrl]"
   echo "    filename.xml - xml file with application(s) configuration "
   echo "Run options: "
   echo "    start[default] - only start application"
   echo "    run - start, initialize and run application"
   echo "    test - test content of xml file, check if login on specified node" 
   echo "            is allowed, test if run directory exists"
   echo "    stop - stop running application, emulates Ctrl-C"
   echo "    kill - kill running application, using kill command"
   echo "For test, stop, kill commands node id can be specified like run.sh file.xml test 3"
   echo "Ctrl options: "
   echo "    -dim - use DIM for application control, DIM name server should run and "
   echo "              DIM_DNS_NODE should be configured in xml file or as enviroment variable"
   echo "    -sctrl - simplified socket-based control to test application without DIM/GUI"
   echo "    -verbose - provide output of all commands, applied by script"
   exit 0
fi   

XMLFILE=$1
shift
if [ ! -f $XMLFILE ] ; then echo "file $XMLFILE not exists"; exit 1; fi

VERBOSE=false
RUNMODE=start
CTRLMODE=
SELECTNODE=-1

while [[ "x$1" != "x" ]] ; do
   if [[ "$1" == "-v" || "$1" == "--vebrose" ]] ; then 
      VERBOSE=true;
   elif [[ "$1" == "-dim" || "$1" == "-sctrl" ]] ; then
      CTRLMODE=$1;
   else
      RUNMODE=$1; 
      if [[ "$RUNMODE" == "test" || "$RUNMODE" == "stop" || "$RUNMODE" == "kill" ]] ; then
         if [[ $2 == $(($2)) ]]; then
            SELECTNODE=$2
            shift  
         fi
      fi  
   fi  
   shift
done

if [[ "$RUNMODE" != "run" && "$RUNMODE" != "start" && "$RUNMODE" != "stop" && "$RUNMODE" != "test" && "$RUNMODE" != "kill" ]] ; then
   echo Wrong run mode  = $RUNMODE selected, use test
   RUNMODE=test
fi 

echo "Chosen run mode = $RUNMODE verbose = $VERBOSE ctrl = $CTRLMODE" 

curdir=`pwd`
if [[ "x$DABCSYS" == "x" ]] ; then DABCSYS=$curdir; echo DABCSYS not specified, use current dir $DABCSYS; fi

if [[ -f $DABCSYS/bin/dabc_xml ]] ; then 
   dabc_xml=$DABCSYS/bin/dabc_xml
else
   dabc_xml=`which dabc_xml`
fi   
if [[ ! -f $dabc_xml ]] ; then echo Cannot find dabc_xml executable; exit 1; fi

numnodes=`$dabc_xml $XMLFILE -number`
retval=$?
if [ $retval -ne 0 ]; then
   echo Cannot identify number of nodes in $XMLFILE - ret = $retval syntax error?
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
# first loop, where only test/stop/kill are done
###########################################################

if [[ "$RUNMODE" == "test" || "$RUNMODE" == "stop" || "$RUNMODE" == "kill" ]] 
then
for (( counter=0; counter<numnodes; counter=counter+1 ))
do
   if (( $SELECTNODE==-1 || $SELECTNODE==$counter )) ; then 

	   callargs=`$dabc_xml $XMLFILE $CTRLMODE -id $counter -workdir $currdir -mode $RUNMODE`
	   retval=$?
	   if [ $retval -ne 0 ]; then
	      echo "Cannot identify test call args for node $counter  err = $retval"
	      exit $retval
	   fi
	   
	   if [[ "$VERBOSE" == "true" ]] ; then echo RUN:: $callargs; fi
	   
	   $callargs
	
	   retval=$? 
	   if [[ "$RUNMODE" == "test" || "$RUNMODE" == "run" ]] ; then 
	      if [ $retval -ne 0 ] ; then
	         echo Mode $RUNMODE fail for node $counter with err = $retval
	         exit $retval
	      fi
	   fi
   fi
done
fi

if [[ "$RUNMODE" == "run" || "$RUNMODE" == "start" ]]
then

remconnfile=dabc_server.id
lclconnfile=dabc_conn.id
connstr=$remconnfile

rm -f $remconnfile $lclconnfile

for (( counter=0; counter<numnodes; counter=counter+1 ))
do
   callargs=`$dabc_xml $XMLFILE $CTRLMODE -id $counter -workdir $currdir -mode copy`
   retval=$?
   if [ $retval -ne 0 ]; then
      echo "Cannot define copy args for node $counter  err = $retval"
      exit $retval
   fi
   
   if [[ "x$callargs" != "x" ]]; then
      if [[ "$VERBOSE" == "true" ]] ; then echo RUN:: $callargs; fi
      $callargs > /dev/null 2>&1
   fi;

   callargs=`$dabc_xml $XMLFILE $CTRLMODE -id $counter -workdir $currdir -conn $connstr -mode $RUNMODE`
   retval=$?
   if [ $retval -ne 0 ]; then
      echo "Cannot define call args for node $counter  err = $retval"
      exit $retval
   fi

   if [[ "$VERBOSE" == "true" ]] ; then echo RUN:: $callargs; fi
   
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
   
   callargs=`$dabc_xml $XMLFILE $CTRLMODE -id $counter -workdir $currdir -conn $connstr -mode conn`
   
   if [[ "x$callargs" != "x" ]]
   then
      connstr=""
   
      if [[ "$VERBOSE" == "true" ]] ; then echo RUN:: $callargs $lclconnfile; fi
      
      for (( cnt=10; cnt>0; cnt=cnt-1)) ; do
       
         $callargs $lclconnfile 2>/dev/null
         
         if [ -f $lclconnfile ]; then 
            connstr=`cat $lclconnfile` 
         fi  
         
         if [[ "x$connstr" == "x" ]] ; then sleep 1; else cnt=0; fi
      done 

      rm -f $remconnfile $lclconnfile

      if [[ "$VERBOSE" == "true" ]] ; then echo RUN:: Connection string is $connstr; fi
   fi
done

fi

