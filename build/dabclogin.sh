#!/bin/sh

##############  specify DABC location here #################

export DABCSYS=`pwd`

export PATH=$DABCSYS/bin:$PATH

export LD_LIBRARY_PATH=.:$DABCSYS/lib:$LD_LIBRARY_PATH

echo Configure DABC version at $DABCSYS

##### here is JAVA configuration, enable it only if java gui will be used #######

#export CLASSPATH=.:$DABCSYS/lib/xgui.jar:$DABCSYS/lib/dim.jar
#
#if [[ "x$JAVA_HOME" == "x" ]] ; then 
#   if [[ -d /usr/lib/jvm/java-1.6.0-sun ]] ; then
#      export JAVA_HOME=/usr/lib/jvm/java-1.6.0-sun
#      echo "Set Java location (JAVA_HOME) to $JAVA_HOME"
#   elif [[ -d /usr/lib64/jvm/java-1.6.0-sun ]] ; then
#      export JAVA_HOME=/usr/lib64/jvm/java-1.6.0-sun
#      echo "Set Java location (JAVA_HOME) to $JAVA_HOME"
#   else
#      echo "!!!!!!!!! Java location (JAVA_HOME) not specified - try default !!!!!!!!!"
#      export JAVA_HOME=/usr/lib/jvm/default-java
#   fi
#fi
#
#alias dabc="$JAVA_HOME/bin/java -Xmx400m xgui.xGui"
#
#alias dabcmoni="$JAVA_HOME/bin/java -Xmx400m xgui.xGui -moni"
#
#echo ">>> Set DIM_DNS_NODE to the node to run the DIM name server"
#echo ">>> Start DIM name server on that node by dimDns"
#echo ">>> Start DIM browser on that node by dimDid (recommended)"
