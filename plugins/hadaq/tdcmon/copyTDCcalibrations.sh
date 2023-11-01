#!/bin/bash
# JAM 23-08-2021 - this script will copy most recent trb3 calibration files from bnet servers 
#JAM 11-01-2022 - note that copying of calfiles and starting monitor analysis is now decoupledi
# JAM 31-10-2023: adjust to changed input server hostnames
export CALRUNID=$1
#"2120918040201"
#export CALRUNID="21209180439"
#echo starting tdc analysis with runid $CALRUNID
#/bin/pidof -x  dabc_mon | /usr/bin/xargs /bin/kill '-9' 
#sleep 1
TDCMONHOME=/home/hadaq/local/tdcmon/dabc
cd $TDCMONHOME
rm -rf tmp/*
rm -rf cal/*
for servernode in  14 15 16 17 18 
do
  echo ------------------------------------
  echo Getting calib files from server lxhadeb$servernode
  SCPATH="lxhadesdaq:/home/hadaq/oper/lxhadeb$servernode/calibr/$CALRUNID\_lxhadeb$servernode/"
  echo scppath is $SCPATH
  scp -rq $SCPATH tmp/  
done
mv tmp/*/local* cal/
#. /home/hadaq/local/bin/trb3login

#/home/hadaq/local/bin/dabc_mon tdcmon.xml > $TDCMONHOME/log/dabclog.txt 2>&1
echo finished.
