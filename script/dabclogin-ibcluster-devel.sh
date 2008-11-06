#! /bin/bash
# dabclogin
# first version 01-Jul-2008, Joern Adamczewski gsi
# adjusted for infiniband cluster 02-Jul-2008 JA
#
#
echo
echo "***********************************************************************"
echo "*** dabclogin setting up Data Acquisition Backbone Core environments..."

VERSIONPATH=/usr/pub/daq
#------------------------------------
# cleanup old library path:
  IFSOLD=$IFS
  NEWPATH=
  IFS=':'
  for dir in $LD_LIBRARY_PATH
    do
#    echo testing $dir from ldpath
    para=`echo $dir |awk '{gsub(".*dabc.*","");print $0}'`   
    if [ -n "$para" ]; then
#	echo keeping $para in ldpath
	NEWPATH=$NEWPATH':'$para
    fi  
  done
  IFS=$IFSOLD
  export LD_LIBRARY_PATH=${NEWPATH#:}
# cleanup old path:
  IFSOLD=$IFS
  NEWPATH=
  IFS=':'
  for dir in $PATH
    do
#    echo testing $dir from path
    para=`echo $dir |awk '{gsub(".*dabc.*","");print $0}'`    
    if  [ -n "$para" ]; then
#	echo keeping $para in path
	NEWPATH=$NEWPATH':'$para
    fi  
  done
  IFS=$IFSOLD
  export PATH=${NEWPATH#:}
# cleanup old classpath:
   IFSOLD=$IFS
  NEWPATH=
  IFS=':'
  for dir in $CLASSPATH
    do
#    echo testing $dir from classpath
    para=`echo $dir |awk '{gsub(".*dabc.*","");print $0}'`   
    if [ -n "$para" ]; then
#	echo keeping $para in classpath
	NEWPATH=$NEWPATH':'$para
    fi 
  done
  IFS=$IFSOLD
  export CLASSPATH=${NEWPATH#:}

#--------------------------------
# the dim part:
export DIMDIR=$VERSIONPATH/dabc/dim/dim
ODIR=linux

export DIMDIR ODIR
export LD_LIBRARY_PATH=$DIMDIR/$ODIR:$LD_LIBRARY_PATH



alias dimTestServer=$DIMDIR/$ODIR/testServer
alias dimTestClient=$DIMDIR/$ODIR/testClient
alias dimTest_server=$DIMDIR/$ODIR/test_server
alias dimTest_client=$DIMDIR/$ODIR/test_client
alias dimDns=$DIMDIR/$ODIR/dns
alias dim_get_service=$DIMDIR/$ODIR/dim_get_service
alias dim_send_command=$DIMDIR/$ODIR/dim_send_command
alias dimBridge=$DIMDIR/$ODIR/DimBridge
alias dimDid=$DIMDIR/$ODIR/did
echo "*** enabled DIM at $DIMDIR ."

#----------------------------------
# the xdaq part:

export XDAQ_PLATFORM=x86_64
export PLATFORM=x86_64
export XDAQ_OS=linux
export XDAQ_ROOT=$VERSIONPATH/dabc/xdaq/xdaq/TriDAS
export XDAQ_DOCUMENT_ROOT=$XDAQ_ROOT/daq
export XDAQ_VERSION=314

LD_LIBRARY_PATH=$XDAQ_ROOT/$PLATFORM/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
PATH=$XDAQ_ROOT/$PLATFORM/bin:$PATH
export PATH
alias xdaq=$XDAQ_ROOT/$PLATFORM/bin/xdaq.exe
echo "*** enabled XDAQ at $XDAQ_ROOT ."
#----------------------------------
# the dabc part:

#export DABCSYS=$VERSIONPATH/dabc/dabc/dabc
export DABCSYS=/u/adamczew/lxgsi/dabc.workspace/dabc
export DABCSYSCORE=$DABCSYS

export PATH=$DABCSYSCORE/bin:$PATH


export LD_LIBRARY_PATH=.:$DABCSYSCORE/lib:$DABCSYS/controls/xdaq/dabc/nodecontrol/lib/$XDAQ_OS/$PLATFORM:$LD_LIBRARY_PATH

# for ibcluster: add path to pci drivers
#export LD_LIBRARY_PATH=$DABCSYS/plugins/abb/linuxdrivers/pciDriver2/lib:$DABCSYS/plugins/abb/linuxdrivers/mprace/lib:$LD_LIBRARY_PATH

#-----------------------------------
# java gui part:
export CLASSPATH=.:$DABCSYS/gui/java/generic/:$DIMDIR/jdim/classes

alias dabc="java xgui.xGui"


echo "*** set DABC system directory to $DABCSYS ."
echo "***********************************************************************"
echo
echo "*** Ready. Type dabc to start DABC gui"
