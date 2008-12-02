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

# use dabc native run script. will parse xml file and launch ssh shells

#cd $DABC_USER_DIR
$DABCSYS/bin/run.sh $DABC_USER_DIR/$DABC_XDAQ_CONFIG -dim

echo        dabcstartup is complete. ---