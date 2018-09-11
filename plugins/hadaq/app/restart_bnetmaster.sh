#!/bin/bash
# JAM2018 - script to restart the BNET master from HADES control gui box
echo Re-starting Eventbuilding Control Master  
. /home/hadaq/bin/dabclogin  
cd /home/hadaq/soft/dabc/head/plugins/hadaq/app 
/bin/pidof -x  dabc_exe | /usr/bin/xargs /bin/kill '-s 9' 
dabc_exe master.xml > /home/hadaq/oper/BNET_Control.log 2>&1 &  
echo BNET Control master has been restarted
