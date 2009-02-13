#! /bin/bash
# default dabclogin for dabc distribution 
# v1.0 JAM 2/2009
#
echo
echo "***********************************************************************"
echo "*** dabclogin setting up Data Acquisition Backbone Core environments..."


#----------------------------------
# user installation settings:

# for direct invoking script in the installation directory:
#export DABCSYS=`pwd`
#export DIM_DNS_NODE=$HOSTNAME

# for invoking script from outside, give absolute path here:
export DABCSYS=/home/adamczew/dabc_install
# set machine that runs the DIM name server here:
export DIM_DNS_NODE=depc237


#----------------------------------
# the dabc part:
export PATH=$DABCSYS/bin:$PATH
export LD_LIBRARY_PATH=.:$DABCSYS/lib:$LD_LIBRARY_PATH


#--------------------------------
# the dim part:
# we use DIM as included with DABC distribution:
export DIMDIR=$DABCSYS/dim/dim_v18r0
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


#-----------------------------------
# java gui part:
export CLASSPATH=.:$DABCSYS/gui/java/packages/xgui.jar:$DIMDIR/jdim/classes
alias dabc="java xgui.xGui"
alias dabcmoni="java -Xmx200m xgui.xGui -moni"

echo "*** set DABC system directory to $DABCSYS ."
echo "***********************************************************************"
echo
echo "*** Ready."
echo "*** Type dimDns to start DIM name server." 
echo "*** Type dabc   to start DABC gui"
