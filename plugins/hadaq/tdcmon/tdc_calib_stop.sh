#!/bin/bash 
 echo stop TDC calibration
 /usr/bin/wget  -a /tmp/EB_runid.log -O /tmp/last_calib_runid.txt "http://lxhadeb12:8099/Master/BNET/RunIdStr/value/get.json"
 runid="$(/bin/cat /tmp/last_calib_runid.txt)"
 echo "  Last calibration run id is $runid";

 echo Setting file prefix no file
  /usr/bin/wget  -a /tmp/EB_filestart.log -O /tmp/EB_fileres.txt "http://lxhadeb12:8099/Master/BNET/StopRun/execute?tmout=10"
 echo set prefix nofile on BNET master controller
 sleep 2 
   echo setting CTS back to regular mode
   export DAQOPSERVER=hadesp31
   echo ${DAQOPSERVER}
   enab="$(/bin/cat /home/hadaq/tmp/precalibctsregs.txt | /usr/bin/cut -d' ' -f3)"
   echo "enabling outputs $enab"
   trbcmd w 0x003 0xa0c7 $enab
   trbcmd clearbit 0x003 0xa0c0 0x10
   trbcmd w 0x003 0xa0e3  0x0
# JAM 24-08-21 - now start tdc monitor
ssh hadesp63 "/home/hadaq/local/bin/startTDCcalibmon.sh $runid &"</dev/null &>/dev/null &
echo "started tdc calibmonitor on http://hades63:8092"
