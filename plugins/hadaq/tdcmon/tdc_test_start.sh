#!/bin/bash

echo setting CTS to calibration mode;
export DAQOPSERVER=hadesp31;
   trbcmd r 0x003 0xa0c7 > /home/hadaq/tmp/precalibctsregs.txt 2> /home/hadaq/tmp/precalibctserr.txt ;
   echo disabled output mask: `cat /home/hadaq/tmp/precalibctsregs.txt`;
   trbcmd w 0x003 0xa0c7 0;
   trbcmd clearbit 0x003 0xa0c0 0x1F;
   trbcmd setbit   0x003 0xa0c0 0x1D;
#   trbcmd w 0x003 0xa0e3  0x1E8480;
   trbcmd w 0x003 0xa0e3  0x61A80;

echo Setting file prefix ct;
   /usr/bin/wget  -a /tmp/EB_filestart.log -O /tmp/EB_fileres.txt "http://lxhadeb12:8099/Master/BNET/StartRun/execute?prefix=ct&oninit=10";
   echo set prefix ct on BNET master controller;
   
  
