#!/bin/bash

# this script can be used to select some nodes from latest acquired routing tables 
# and produce xml file (or any other files), required to run test on LOEWE-CSC cluster
# for a moment one can select switch name(s), which should be used
#    [shell] select.sh ibswitch05 ibswitch06
# allowed arguments:
#     nodes         - fill interactive nodes, which can be used 
#     ibswitchXX    - add all nodes, connected to the specified switch
#     noibswitchXX  - exclude all nodes, connected to the specified switch
#     addnodes      - add all nodes
#     addquads      - add all quads
#     cpu XX        - from all interactive nodes, select all which has cpu load less than specified in percent




if [[ "$1" == "loewe" ]] ; then
  rdir=$HOME
  rdir+=/routing/
  shift
else 
  rdir=/data.local1/routing/

  if [ ! -d $rdir ] ; then
    rdir=/scratch/linev/routing/
  fi

  if [ ! -d $rdir ] ; then
    rdir=$HOME
    rdir+=/routing/
  fi
fi

if [ ! -d $rdir ] ; then
  echo "directory with routing files not found"
  exit 1
fi


rdir+=`date +%Y-%m-%d`

if [ ! -d $rdir ] ; then
   echo "Directory $rdir not exists, run produce.sh before"
   exit 1
fi

files=`ls $rdir/*_route.txt`

if [ "x$files" == "x" ] ; then
   echo "no any files find";
   exit 1
fi

echo FOUND: $files

args=""
cpu="0"
maxload="30"

while [ "x$1" != "x" ] ; do
   if [ "$1" == "cpu" ] ; then
     cpu="1"
     shift
     if [ "x$1" != "x" ] ; then maxload=$1; fi        
   else
     args+=$1
     args+=" "
   fi
   shift
done

echo ARGS: $args

export ROUTING_FILES=$files
export SELECT_ARGS=$args

export SELECTED_NODES=$PWD/nodes.txt

# this command should generate nodes list from current routing, based on provided args

cd ..; rm -f sc-select.log; dabc_run sc-select.xml; cd scripts

# here we select nodes where we can log in and where CPU load not that high

if [ "$cpu" == "1" ] ; then
  echo "selecting nodes by CPU load, max is $maxload"
  
  newfile=$PWD/cpunodes.txt
  
  rm -rf $newfile
  
  for node in `cat $SELECTED_NODES` ; do
#     load=`ssh $node "top -b -n 1 | grep Cpu" | awk '{ len = length($2); print substr($2,1,len-6); }'`
#     load="24"
 
     load=`ssh $node mpstat | awk '{ if ($2=="all") printf("%d\n",$3+$5); }'`
    
     acc="rejected"

     if [ "x$load" != "x" ] ; then
        if (($load < $maxload)); then
           echo $node >> $newfile
           acc="accepted"
        fi
     fi    

     echo CPU usage on $node is $load, $acc  
  done

  rm -rf $SELECTED_NODES
 
  export SELECTED_NODES=$newfile
 
else
  cat $SELECTED_NODES
fi

if [ ! -f $SELECTED_NODES ] ; then
  echo "Nodes list file $SELECTED_NODES was not generated"
  exit 1
fi

export SELECT_ARGS=schedule
export SCHEDULE_FILE="schedule.txt"

# here we want to generate schedule for our nodes

cd ..; rm -f sc-select.log; dabc_run sc-select.xml; cd scripts



cfgfile="../csc-big.xml"

rm -rf $cfgfile

cat csc-begin.xml >> $cfgfile 

awk '{ printf("  <Context host=\"%s\" port=\"5432\"/>\n",$1); }' < $SELECTED_NODES >> $cfgfile

cat csc-end.xml >> $cfgfile 


rm -rf $SELECTED_NODES
