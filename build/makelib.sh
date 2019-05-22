#! /bin/sh

# Script to create a shared library.
# Called by Makefile.

DABC_OS=${1}
LD=${2}
RM=${3}
MV=${4}
LN=${5}
LDFLAGS=${6}
SOFLAGS=${7}
SOSUFFIX=${8}

LIBNAME=${9}
LIBOBJS=${10}
LIBDIR=${11}
EXTRAOPT=${12}

#echo extra = ${EXTRAOPT}
#echo libdir = $LIBDIR
#echo libobjs = $LIBOBJS
#echo libname = $LIBNAME

if [ "x$LIBDIR" != "x" ]; then
   $RM $LIBDIR/$LIBNAME.$SOSUFFIX
else
   $RM $LIBNAME.$SOSUFFIX
fi

if [ "$DABC_OS" = "Mac" ]; then

echo $LD $LDFLAGS $EXTRAOPT -O $LIBOBJS -o $LIBNAME.$SOSUFFIX

$LD $LDFLAGS $EXTRAOPT -O $LIBOBJS -o $LIBNAME.$SOSUFFIX

else

echo $LD $SOFLAGS$LIBNAME.$SOSUFFIX $LDFLAGS $EXTRAOPT -O $LIBOBJS -o $LIBNAME.$SOSUFFIX

$LD $SOFLAGS$LIBNAME.$SOSUFFIX $LDFLAGS $EXTRAOPT -O $LIBOBJS -o $LIBNAME.$SOSUFFIX

fi

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
