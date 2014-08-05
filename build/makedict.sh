#!/bin/bash

# Script to create a ROOT dictionary
# Called by Makefile.

DABC_ROOTVERSION=${1}
DABC_ROOTSYS=${2}
DABC_ROOTBIN=${3}
CXXOPTIONS=${4}
DICTIONARY=${5}
LIBNAME=${6}
HEADERS=${7}


if [[ "$DABC_ROOTVERSION" == "6" ]]; then

   echo "Generating root6 dictionary $DICTIONARY ..." 

   $DABC_ROOTBIN/rootcling -f $DICTIONARY -s $LIBNAME -c $CXXOPTIONS $HEADERS

   exit 0
fi


echo "Generating root5 dictionary $DICTIONARY ..." 

$DABC_ROOTBIN/rootcint -f $DICTIONARY -c -p  $CXXOPTIONS  $HEADERS
   
exit 0
