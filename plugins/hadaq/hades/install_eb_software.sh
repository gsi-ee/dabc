#!/bin/sh
# JAM 25-04-2023: all steps required to install EB software to lxhadeb* from scratch
###
# first we require certain debian packages installed with root:
# aptitude install autoconf automake libtool make libz-dev attr libattr1-dev uuid-dev g++ libssl-dev
###
#
mkdir -p /home/hadaq/bin
mkdir -p /home/hadaq/oper
mkdir -p /home/hadaq/ltsm/ibm_tsm/8.1.14.0
# get tsm packages from other eb server:
cd /home/hadaq/ltsm/ibm_tsm/8.1.14.0/
scp lxhadeb09:/home/hadaq/ltsm/ibm_tsm/8.1.14.0/* .
# hereinstall tsm packages as root:
#cd /home/hadaq/ltsm/ibm_tsm/8.1.14.0/
#dpkg -i gskcrypt64_8.0-55.24.linux.x86_64.deb
#dpkg -i gskssl64_8.0-55.24.linux.x86_64.deb 
#dpkg -i tivsm-api64.amd64.deb
#dpkg -i tivsm-apicit.amd64.deb
#dpkg -i tivsm-ba.amd64.deb
#dpkg -i tivsm-bacit.amd64.deb

## also need to import certificates for tsm:
mkdir -p /home/hadaq/ltsm/certs
scp lxhadeb09:/home/hadaq/ltsm/certs/cert256.arm .
# this has to be done as root:
#cd /home/hadaq/ltsm/certs
#/opt/tivoli/tsm/client/api/bin64/dsmcert -add -server lxltsm01 -file ./cert256.arm
## next are ltsm and fsq libs:
export FSQGIT=/home/hadaq/ltsm/fsq
export LTSMGIT=/home/hadaq/ltsm/ltsm
export LTSMINSTALL=/home/hadaq/ltsm/install
mkdir -p $FSQGIT
mkdir -p $LTSMGIT
mkdir -p /home/hadaq/ltsm/install2023
ln -s /home/hadaq/ltsm/install2023 $LTSMINSTALL
cd $FSQGIT
git clone https://github.com/tstibor/fsq.git .
./autogen.sh 
./configure --prefix=$LTSMINSTALL
make -j4
make install
cd $LTSMGIT
git clone  https://github.com/tstibor/ltsm.git .
./autogen.sh 
./configure --prefix=$LTSMINSTALL
make -j4 
make install
# todo: copy config.h (still missing in install!)
cp $FSQGIT/config.h $LTSMINSTALL/include/fsq/
cp $LTSMGIT/config.h $LTSMINSTALL/include/ltsm/
# now need to install own cmake (version of deb10 too old/buggy for dabc)
mkdir  -p /home/hadaq/soft/cmake
cd  /home/hadaq/soft/cmake
# here get tarball, unpack and build it
scp lxhadeb09:/home/hadaq/soft/cmake/cmake-3.16.6.tar.gz .
tar zxvf cmake-3.16.6.tar.gz 
cd cmake-3.16.6/
./configure 
make -j6
ln -s /home/hadaq/soft/cmake/cmake-3.16.6/bin/cmake /home/hadaq/bin/cmake
export PATH=/home/hadaq/bin:$PATH
# ^this export is done automatically when newly logged in with existing /home/hadaq/bin
# now build dabc and other software:
export DABCGIT=/home/hadaq/soft/dabc/git
export DABCSYS=/home/hadaq/soft/dabc/install/master
export STREAMGIT=/home/hadaq/soft/streamframe/git
export STREAMSYS=/home/hadaq/soft/streamframe/install/master
MYHOST=`/bin/hostname` 
mkdir -p  $STREAMGIT
mkdir -p  $STREAMSYS
cd $STREAMGIT
git clone  https://github.com/gsi-ee/stream.git .
cd $STREAMSYS
/home/hadaq/bin/cmake $STREAMGIT
/usr/bin/make -j20
mkdir -p  $DABCGIT
mkdir -p  $DABCSYS
cd $DABCGIT
git clone  https://github.com/gsi-ee/dabc.git .
cd $DABCSYS
/home/hadaq/bin/cmake $DABCGIT -Dltsm=ON
/usr/bin/make -j20
# now provide some useful links:
cd /home/hadaq/bin
ln -s $DABCSYS/dabclogin .
ln -s $STREAMSYS/streamlogin .
ln -s $DABCSYS/plugins/hadaq/hades/update_dabc_install.sh .
ln -s $DABCSYS/plugins/hadaq/hades/cleanup_evtbuild.pl .
ln -s $DABCSYS/plugins/ltsm/app/archivecal_ltsm_fsq.sh archivecal_ltsm.sh
ln -s $DABCSYS/plugins/ltsm/app/archived_data_ltsm.pl .
ln -s $DABCSYS/plugins/ltsm/app/data2tape_ltsm.pl .
ln -s $DABCSYS/plugins/ltsm/app/deleteTestfiles_ltsm.pl
scp lxhadeb09:/home/hadaq/bin/optimize_taskset.sh .
scp lxhadeb09:/home/hadaq/bin/cleanup_1disk.pl .
ln -s cleanup_1disk.pl cleanup.pl
echo provided new software installations for fsq, ltsm, DABC, and STREAM on $MYHOST
echo please still adjust crontabs for users hades and root /root/bin/set_eth_affinity.pl

