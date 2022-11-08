#!/bin/sh
### november 2022: adjusted for git JAM
############## update stream and dabc installations on hadaq EB servers#################
export DABCGIT=/home/hadaq/soft/dabc/git
export DABCSYS=/home/hadaq/soft/dabc/install/master
export STREAMGIT=/home/hadaq/soft/streamframe/git
export STREAMSYS=/home/hadaq/soft/streamframe/install/master
MYHOST=`/bin/hostname` 
cd $STREAMGIT
/usr/bin/git pull origin
cd $STREAMSYS
/bin/rm CMakeCache.txt
/home/hadaq/bin/cmake $STREAMGIT
/usr/bin/make clean
/usr/bin/make -j20
cd $DABCGIT
/usr/bin/git pull origin
cd $DABCSYS
//bin/rm CMakeCache.txt
/home/hadaq/bin/cmake $DABCGIT -Dltsm=ON
/usr/bin/make clean
/usr/bin/make -j20
echo updated git installations of DABC and STREAM on $MYHOST
