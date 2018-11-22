#!/bin/bash
# JAM2018 - script to restart the dabc/stream raw data monitor HADES control gui box
echo Re-starting DAQ Raw data monitor  
. /home/hadaq/bin/dabclogin  
. /home/hadaq/bin/streamlogin
cd /home/hadaq/monitor/hades/dabc
/bin/pidof -x  dabc_exe | /usr/bin/xargs /bin/kill '-s 9' 
dabc_exe hades.xml > /home/hadaq/oper/DabcRawMon.log 2>&1 &  
echo DABC Raw DAQ Monitor has been restarted
