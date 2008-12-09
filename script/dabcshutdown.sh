#! /bin/bash
# Arguments:
#  1 DABCROOT
#  2 PWD
#  3 DIM_DNS_NODE
#  4 GuiNode
#  5-9 others
echo ---- dabcshutdown called with:
echo $1
echo $2
echo $3
echo $4
#echo $5

export DABCSYS=$1
export DABC_USER_DIR=$2
export DABC_XDAQ_CONFIG=$3 
export DIM_DNS_NODE=$4
export DABC_GUI_NODE=$5
cd $DABC_USER_DIR


echo   dabcshutdown for plain dim control....
#printenv 
# $5 $6 $7 $8
$DABCSYS/bin/run.sh $DABC_XDAQ_CONFIG kill -v 

echo        dabcshutdown is complete. ---






