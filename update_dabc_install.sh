#!/bin/sh

############## update stream and dabc installations on hada EB servers#################
export DABCSYS=/home/hadaq/soft/dabc/head
export STREAMSYS=/home/hadaq/soft/stream
cd $STREAMSYS;
/usr/bin/svn update;
/usr/bin/make -j10;
cd $DABCSYS;
/usr/bin/svn update;
/usr/bin/make -j10;
echo updated installtions of DABC and STREAM