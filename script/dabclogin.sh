#!/bin/sh

echo "Simple DABC login"

export DABCSYS=`pwd`

export PATH=$DABCSYS/bin:$PATH

export LD_LIBRARY_PATH=$DABCSYS/lib:$LD_LIBRARY_PATH
