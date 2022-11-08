#!/bin/bash
# JAM2018 - script to restart the BNET master from HADES control gui box
echo Re-starting Eventbuilding Control Master  
. /home/hadaq/bin/dabclogin  
#export JSROOTSYS=/home/hadaq/soft/jsroot/v6
cd /home/hadaq/soft/dabc/install/master/plugins/hadaq/app 
#/bin/pidof -x  dabc_exe | /usr/bin/xargs /bin/kill '-s 9'
pkill dabc_master
sleep 1;
pkill -9 dabc_master
# JAM 2021 note that dabc_master is just other name for dabc_exe
# we use this to avoid conflicts with rawmon at restart
/home/hadaq/bin/dabc_master master.xml > /home/hadaq/oper/BNET_Control.log 2>&1 &  
echo BNET Control master has been restarted
