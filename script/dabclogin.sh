#!/bin/sh

echo "Simple DABC login"

if [[ "x$JAVA_HOME" == "x" ]] ; then 
   if [[ -d /usr/lib/jvm/java-1.5.0-sun ]] ; 
     then
       export JAVA_HOME=/usr/lib/jvm/java-1.5.0-sun
       echo "Java location (JAVA_HOME) set to $JAVA_HOME"
     else
       echo "!!!!!!!!! Java location (JAVA_HOME) not specified - try default !!!!!!!!!"
       export JAVA_HOME=/usr/lib/jvm/default-java
     fi
fi

export DABCSYS=`pwd`

export PATH=$DABCSYS/bin:$PATH

export LD_LIBRARY_PATH=$DABCSYS/lib:$LD_LIBRARY_PATH

export CLASSPATH=.:$DABCSYS/gui/java/packages/xgui.jar:$DABCSYS/dim/dim_v19r9/jdim/classes

alias dabc="$JAVA_HOME/bin/java -Xmx200m xgui.xGui"

alias dabcmoni="$JAVA_HOME/bin/java -Xmx200m xgui.xGui -moni"

echo ">>> Set DIM_DNS_NODE to the node to run the DIM name server"
echo ">>> Start DIM name server on that node by dimDns"
echo ">>> Start DIM browser on that node by dimDid (recommended)"
