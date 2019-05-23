#! /bin/sh

# script to generate dependency file
# dependency file itself depend from all sources and rebuild automatically
# 31.01.2012 SL replace makedepend by g++ -MM -MT option

DABC_OS=$1
OBJSUF=$2
DEPSUF=$3
TGTDIR=$4
COMPILER=$5
CFLAGS=$6
DEPFILE=$7
SRCFILE=$8
SRCBASE=$9

# echo "SRCBASE = $SRCBASE TGTDIR = $TGTDIR"


# echo Calling g++ $CFLAGS $SRCFILE -o $DEPFILE -MM -MT $TGTDIR/$SRCBASE.$OBJSUF $TGTDIR/$SRCBASE.$DEPSUF

if [ $DABC_OS = "Darwin" ]; then

$COMPILER $CFLAGS $SRCFILE -o $DEPFILE -MM -MT "$TGTDIR/$SRCBASE.$OBJSUF $TGTDIR/$SRCBASE.$DEPSUF"

else

$COMPILER $CFLAGS $SRCFILE -o $DEPFILE -MM -MT "$TGTDIR/$SRCBASE.$OBJSUF $TGTDIR/$SRCBASE.$DEPSUF"

fi

exit $?
