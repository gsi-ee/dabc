#! /bin/bash
# Arguments:
#  1 DABCSYS
#  2 PWD
#  3 setupfile
#  4 DIM_DNS_NODE
#  5 GuiNode
#  5-9 others
echo ---- dabcstartup called with:
echo $1
echo $2
echo $3
echo $4
echo $5

export DABCSYS=$1
export DABC_USER_DIR=$2
export DIM_DNS_NODE=$4
export DABC_MASTER_NODE=$5
export DABC_XDAQ_CONFIG=$3 
#DABC_MBS_Setup.xml
#export PATH=$DABCSYS/bin:$PATH
cd $DABC_USER_DIR
echo   dabcstartup for plain dim control....
#printenv 
# $5 $6 $7 $8
$DABCSYS/bin/run.sh $DABC_XDAQ_CONFIG start -dim -v 
echo        dabcstartup is complete. --- 