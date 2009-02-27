#!/bin/bash
ARCH=`uname -m`
export LD_LIBRARY_PATH=$PWD/../$ARCH/lib:$PWD/../linuxdrivers/mprace/lib:$PWD/../linuxdrivers/pciDriver2/lib:$LD_LIBRARY_PATH
