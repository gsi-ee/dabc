#! /bin/bash
### arguments: xdaqport workingdir setupfile dim_dns_node
#if [ -z "$1" ] 
#    then echo Please specify Port number for context as!
#else
echo startxdaq called with:
echo $1
echo $2
echo $3
echo $4

export DIM_DNS_NODE=$4
export DABC_USER_DIR=$2

 
. /misc/adamczew/dabc.workspace/dabc/script/dabclogin-gsi-devel.sh
#. /u/adamczew/lxgsi/dabc.workspace/dabc/script/dabclogin-ibcluster-devel.sh


cd $2
echo Dim name server expected on $DIM_DNS_NODE.
## this is for normal operation:
echo "Starting XDAQ exec for" $HOSTNAME:$1 "..."
$XDAQ_ROOT/$XDAQ_PLATFORM/bin/xdaq.exe -p $1 -c $3 > dabclog-$HOSTNAME-$1.txt 2>&1
## enable following lines to run under gdb + print stack trace on crash:
#echo "Starting XDAQ exec in debug mode for" $HOSTNAME:$1 "..."
### new gdb accepts -ex option:
#gdb -ex r -ex bt --args $XDAQ_ROOT/$XDAQ_PLATFORM/bin/xdaq.exe -p $1 -c $3 > dabclog-$HOSTNAME-$1.txt 2>&1
### following for old gdb:
#echo r >   gcoms.tmp
#echo bt >> gcoms.tmp
#gdb -x=gcoms.tmp --args $XDAQ_ROOT/$XDAQ_PLATFORM/bin/xdaq.exe -p $1 -c $3 > dabclog-$HOSTNAME-$1.txt 2>&1



#fi