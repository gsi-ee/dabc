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
cd $DABC_USER_DIR

#printenv 
# $5 $6 $7 $8
############# NEW: start nodes from setup file
#XMLFILE=`cat $DABC_XDAQ_CONFIG`
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
        PORTNUM=`echo 1911`
        IFSOLD=$IFS
        IFS=':'
        for SUBNAME in $FULLNAME
            do
                #echo testing $SUBNAME from $FULLNAME
                    if [[ "$SUBCOUNT" -eq "2" ]]; then                        
                        NODENAME=`echo $SUBNAME | awk '{ gsub("//","") ; print }'`
                        #echo "using node  $NODENAME"
                    elif [[ "$SUBCOUNT" -eq "3" ]]; then
                         PORTNUM=`echo $SUBNAME | awk '{ gsub("\\">","") ; print }'`
                        #echo "using port $PORTNUM"                     
                    fi  
                let "SUBCOUNT+=1"; 
            done
        IFS=$IFSOLD
    echo starting xdaq node $NODENAME on port $PORTNUM...
### first with invisible shells:
#    ssh -l $USER $NODENAME $DABC_USER_DIR/startxdaq.sh $PORTNUM $DABC_USER_DIR $DABC_XDAQ_CONFIG $DIM_DNS_NODE &
### second variant using konsole terms:
#ssh -X -l $USER $NODENAME /usr/bin/X11/xterm  -T DABC_Worker:$NODENAME -e "$DABC_USER_DIR/startxdaq.sh $PORTNUM $DABC_USER_DIR $DABC_XDAQ_CONFIG $DIM_DNS_NODE" & 
###############
# for gsi installation, use start script in DABCSYS:
ssh -l $USER $NODENAME $DABCSYS/script/startxdaq.sh $PORTNUM $DABC_USER_DIR $DABC_XDAQ_CONFIG $DIM_DNS_NODE &


    echo        ready.
    sleep 1
# might get race condition for authority file without this waitstate...
done
IFS=$IFSVERYOLD
#sleep 30
echo        dabcstartup is complete. ---