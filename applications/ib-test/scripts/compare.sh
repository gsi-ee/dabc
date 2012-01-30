#!/bin/bash

# this script can be used to compare routing tables, acquired on the LOEWE-CSC cluster
# for a moment one can see only number of changes between two consequintive tables
# as argument, date can be specified like:
#    [shell] compare.sh 2011-04-15

rdir=/data.local1/routing/

if [ ! -d $rdir ] ; then
  rdir=/scratch/linev/routing/
fi

if [ ! -d $rdir ] ; then
  rdir=$HOME
  rdir+=/routing/
fi

if [ ! -d $rdir ] ; then
  echo "directory with routing files not found"
  exit 1
fi

files=""

while [ "x$1" != "x" ] ; do
   if [ -d $rdir$1 ] ; then
      mask=$rdir
      mask+=$1
      mask+="/*_route.txt"
      files+=`ls $mask`
      files+=" "
   elif [ -f $rdir$1 ] ; then
      files+=$rdir$1
      files+=" "
   elif [ -f $1 ] ; then
      files+=$1
      files+=" "       
   elif [[ "$1" == "now" || "$1" == "today" ]] ; then
      mask=$rdir
      mask+=`date +%Y-%m-%d`
      mask+="/*_route.txt"
      files+=`ls $mask`
      files+=" "
   fi      
   shift
done

if [ "x$files" == "x" ] ; then
   echo "no any files specified";
   exit 1
fi

export ROUTING_FILES=$files

echo $ROUTING_FILES

cd ..; rm -f sc-comp.log; dabc_run sc-comp.xml
