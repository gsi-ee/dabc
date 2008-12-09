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
############# NEW: get nodes from setup file
TARGETSTR="<xc:Context"  
CONTEXTLIST=`grep "$TARGETSTR" "$DABC_XDAQ_CONFIG" ` 
#echo found contextlist $CONTEXTLIST
declare -i SUBCOUNT
SUBCOUNT=0
IFSVERYOLD=$IFS
IFS=$'\012' 
for FULLNAME in $CONTEXTLIST;
    do
        let "SUBCOUNT=0"; 
        NODENAME=`echo defaultnode`
        IFSOLD=$IFS
        IFS=':'
        for SUBNAME in $FULLNAME
            do
                #echo testing $SUBNAME from $FULLNAME
                    if [[ "$SUBCOUNT" -eq "2" ]]; then                        
                        NODENAME=`echo $SUBNAME | awk '{ gsub("//","") ; print }'`
                        #echo "using node  $NODENAME"
                    fi  
                let "SUBCOUNT+=1"; 
            done
        IFS=$IFSOLD
#    echo killing all xdaq applications on node $NODENAME ...   
#    ssh -l $USER $NODENAME  $DABCSYS/script/killxdaq.sh xdaq &
    echo killing all dabc_run applications on node $NODENAME ...   
    ssh -l $USER $NODENAME  $DABCSYS/script/killxdaq.sh dabc_run &
    echo        ready.

done
IFS=$IFSVERYOLD
echo        dabcshutdown is complete. ---






