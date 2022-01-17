#!/bin/bash
# JAM 11-01-2022 - this script will start stream analysis on hld files from calibration test run
export CALRUNID=$1

#"2120918040201"
#export CALRUNID="21209180439"
echo starting tdc test analysis for runid $CALRUNID
/bin/pidof -x  dabc_mon | /usr/bin/xargs /bin/kill '-9' 
sleep 1
TDCMONHOME=/home/hadaq/local/tdcmon/dabc
cd $TDCMONHOME
# create here the hll file with names of files on all eventbuilders:
. ./build_hll.sh ${CALRUNDID}

. /home/hadaq/local/bin/trb3login

/home/hadaq/local/bin/dabc_mon tdcmon.xml > $TDCMONHOME/log/dabclog.txt 2>&1
echo finished.
