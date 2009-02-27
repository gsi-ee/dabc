#! /bin/bash

#-----------------------------------
# java gui part:

#one should specify correct location of dim installation
#export DIMDIR=$DABCSYS/dim/dim_v18r0

export CLASSPATH=.:`pwd`/xgui.jar:$DIMDIR/jdim/classes:$CLASSPATH
alias dabc="java -Xmx200m xgui.xGui -dabc"
alias dabs="java -Xmx200m xgui.xGui -dabs"
alias moni="java -Xmx200m xgui.xGui -moni"
alias mbs="java -Xmx200m xgui.xGui -mbs"
alias guru="java -Xmx200m xgui.xGui -guru"
