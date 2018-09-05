#!/bin/sh

############## update stream and dabc installations on hada EB servers#################
export DABCSYS=/home/hadaq/soft/dabc/head
export STREAMSYS=/home/hadaq/soft/stream
cd $STREAMSYS
/usr/bin/svn update
/usr/bin/make clean
/usr/bin/make -j20
cd $DABCSYS
/usr/bin/svn update
/usr/bin/make clean
/usr/bin/make -j20
echo updated installtions of DABC and STREAM