#!/bin/sh

echo "Simple DABC login"

export DABCSYS=`pwd`

export PATH=$DABCSYS/bin:$PATH

export LD_LIBRARY_PATH=$DABCSYS/lib:$LD_LIBRARY_PATH

export CLASSPATH=.:$DABCSYS/gui/java/packages/xgui.jar:$DABCSYS/dim/dim_v18r0/jdim/classes

alias dabc="java -Xmx200m xgui.xGui"

alias dabcmoni="java -Xmx200m xgui.xGui -moni"

export DIM_DNS_NODE=lxg0526