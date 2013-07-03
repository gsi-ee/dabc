#!/bin/sh

##############  specify DABC location here #################

export DABCSYS=`pwd`

export PATH=$DABCSYS/bin:$PATH

export LD_LIBRARY_PATH=.:$DABCSYS/lib:$LD_LIBRARY_PATH

echo Configure DABC version at $DABCSYS
