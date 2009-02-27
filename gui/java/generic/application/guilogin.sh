#! /bin/bash

#-----------------------------------
# java gui part:

#one should specify correct location of dim installation
#export DIMDIR=$DABCSYS/dim/dim_v18r0
#export DIM_DNS_NODE=host.domain

if [ -z $DIMDIR ]; then
  echo "DIMDIR not specified";
  return 1;
fi

if [ ! -f $DIMDIR/linux/libjdim.so ]; then
  echo "libjdim.so not found in $DIMDIR/linux directory - compile DIM again with 'make JDIM=yes'";
  return 1;
fi

if [ -z $DIM_DNS_NODE ]; then
  echo "DIM_DNS_NODE not specified - gui will not work";
fi

export CLASSPATH=.:`pwd`/xgui.jar:$DIMDIR/jdim/classes:$CLASSPATH
export LD_LIBRARY_PATH=$DIMDIR/linux:$LD_LIBRARY_PATH

alias dabc="java -Xmx200m xgui.xGui -dabc"
alias dabs="java -Xmx200m xgui.xGui -dabs"
alias moni="java -Xmx200m xgui.xGui -moni"
alias mbs="java -Xmx200m xgui.xGui -mbs"
alias guru="java -Xmx200m xgui.xGui -guru"

echo "DABC gui login done. To call it, type 'dabc'"