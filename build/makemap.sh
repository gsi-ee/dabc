#!/bin/bash

# Script to create a ROOT map file
# Called by Makefile.

DABC_ROOTVERSION=${1}
DABC_ROOTSYS=${2}
DABC_ROOTBIN=${3}
CXXOPTIONS=${4}
MAPFILE=${5}
LIBNAME=${6}
LIBDEP=${7}
DICTIONARY=${8}
LINKDEF=${9}
HEADERS=${10}

# xpath=${LIBNAME%/*} 
# xbase=${LIBNAME##*/}
# xfext=${xbase##*.}
# xpref=${xbase%.*}
# MAPFILE=${xpath}/${xpref}.rootmap

SHORTLIBNAME=$(basename $LIBNAME)

if [[ "$DABC_ROOTVERSION" == "6" ]]; then

   echo "Generating root6 mapfile $MAPFILE" 

   ${DABC_ROOTBIN}/rootcling -r ${DICTIONARY} -rml ${SHORTLIBNAME} -rmf ${MAPFILE} -s ${LIBNAME} ${CXXOPTIONS} ${HEADERS} ${LINKDEF}

   exit 0
fi

echo "Generating root5 mapfile $MAPFILE" 

#echo ${DABC_ROOTBIN}rlibmap -r ${MAPFILE} -l ${LIBNAME} -d ${LIBDEP} -c ${LINKDEF}

${DABC_ROOTBIN}rlibmap -r ${MAPFILE} -l ${LIBNAME} -d ${LIBDEP} -c ${LINKDEF}
   
exit 0
