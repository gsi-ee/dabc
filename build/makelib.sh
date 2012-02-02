#! /bin/sh

# Script to create a shared library.
# Called by Makefile.

LD=${1}
RM=${2}
MV=${3}
LN=${4}
LDFLAGS=${5}
SOFLAGS=${6}
SOSUFFIX=${7}

LIBNAME=${8}
LIBOBJS=${9}
LIBDIR=${10}
EXTRAOPT=${11}

#echo extra = ${EXTRAOPT}
#echo libdir = $LIBDIR
#echo libobjs = $LIBOBJS
#echo libname = $LIBNAME

if [ "x$LIBDIR" != "x" ]; then
   $RM $LIBDIR/$LIBNAME.$SOSUFFIX
else
   $RM $LIBNAME.$SOSUFFIX
fi

echo $LD $SOFLAGS$LIBNAME.$SOSUFFIX $LDFLAGS $LIBOBJS $EXTRAOPT -o $LIBNAME.$SOSUFFIX

$LD $SOFLAGS$LIBNAME.$SOSUFFIX $LDFLAGS $LIBOBJS $EXTRAOPT -o $LIBNAME.$SOSUFFIX

# $LN $LIBNAME.$SOSUFFIX.$VESUFFIX $LIBNAME.$SOSUFFIX

if [ "x$LIBDIR" != "x" ]; then
   if [ "x$LIBDIR" != "x." ]; then
      if [ "$LIBDIR" != `pwd` ]; then
         $MV $LIBNAME.$SOSUFFIX $LIBDIR
      fi
   fi   
fi

echo $LIBNAME.$SOSUFFIX 'done'

exit 0
