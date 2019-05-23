#!/bin/sh

##############  specify DABC location here #################

export DABCSYS=`pwd`

export PATH=$DABCSYS/bin:$PATH

if [ `uname` = "Darwin" ]; then
   export DYLD_LIBRARY_PATH=.:$DABCSYS/lib:$DYLD_LIBRARY_PATH
else
   export LD_LIBRARY_PATH=.:$DABCSYS/lib:$LD_LIBRARY_PATH
fi

echo Configure DABC version at $DABCSYS
