#! /bin/bash
### arguments: xdaqport workingdir setupfile dim_dns_node
#if [ -z "$1" ] 
#    then echo Please specify Port number for context as!
#else
echo startxdaq-lxi called with:
echo $1
echo $2
echo $3
echo $4

export DIM_DNS_NODE=$4
export DABC_USER_DIR=$2
# ======= get the DABC path
IFSOLD2=$IFS
IFS='/'
DIRN=""
let "IC=0"
for EL in $0
do
# directory starts with / therefore skip first item
	if [[ "$IC" -gt "0" ]]; then 
		DIR=$DIRN
		DIRN=`echo $DIR?$EL`
	fi
	let "IC+=1"
done
IFS=$IFSOLD2
DIR=`echo $DIR | awk '{ gsub(/?/,"/") ; print }'`
DABCSYS=`echo $DIR | awk '{ gsub(/\/script/,"") ; print }'`
# ======================

if [ -z "$XDAQ_ROOT" ] 
    then . /usr/local/pub/debian3.1/gcc335-13/dabc/bin/dabclogin $DABCSYS
#    then . $DABC_USER_DIR/dabclogin $DABCSYS
fi

cd $2
echo Dim name server expected on $DIM_DNS_NODE.
## this is for normal operation:
echo "Starting XDAQ exec for" $HOSTNAME:$1 "..."
$XDAQ_ROOT/$XDAQ_PLATFORM/bin/xdaq.exe -p $1 -c $3 > dabclog-$HOSTNAME-$1.txt 2>&1
## enable following lines to run under gdb + print stack trace on crash:
#echo "Starting XDAQ exec in debug mode for" $HOSTNAME:$1 "..."
#gdb -ex r -ex bt --args $XDAQ_ROOT/$XDAQ_PLATFORM/bin/xdaq.exe -p $1 -c $3 > dabclog-$HOSTNAME-$1.txt 2>&1

#fi