#!/bin/bash

# this script can be used to compress routing files, stored before
# as argument, date should be specified like
#    [shell] compress.sh 2011-04-15

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

currdir=`pwd`

while [ "x$1" != "x" ] ; do
   if [ -d $rdir$1 ] ; then
      echo "compressing files in directory $rdir$1"
      cd $rdir
      file=$1
      file+=".tar"
      mask=$1
      mask+="/*"
      if [[ -f $file || -f $file.gz ]] ; then
        echo "Files $file.gz already exists - check command again"
      else
         tar chf $file $mask
         gzip $file
         rm -rf $1
         echo "File $file.gz produced"
         ls -lh $file.*
      fi
      cd $curdir
   else 
      echo "directory $1 not found"
   fi      
   shift
done
