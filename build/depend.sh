#! /bin/sh

# script to generate dependency file
# dependency file itself depend from all sources and rebuild automatically
# 31.01.2012 SL replace makedepend by g++ -MM -MT option

OBJSUF=$1
DEPSUF=$2
TGTDIR=$3
CFLAGS=$4
DEPFILE=$5
SRCFILE=$6
SRCBASE=$7

# echo "SRCBASE = $SRCBASE TGTDIR = $TGTDIR"


# echo Calling g++ $CFLAGS $SRCFILE -o $DEPFILE -MM -MT $TGTDIR/$SRCBASE.$OBJSUF $TGTDIR/$SRCBASE.$DEPSUF

g++ $CFLAGS $SRCFILE -o $DEPFILE -MM -MT "$TGTDIR/$SRCBASE.$OBJSUF $TGTDIR/$SRCBASE.$DEPSUF"

exit $?


touch $DEPFILE

makedepend -f$DEPFILE -Y -o.$OBJSUF -w 30000 -- $CFLAGS -- $SRCFILE > /dev/null 2>&1

depstat=$?
if [ $depstat -ne 0 ]; then
   rm -f $DEPFILE $DEPFILE.bak
   exit $depstat
fi

# echo "s|$SRCBASE.$OBJSUF|$TGTDIR/$SRCBASE.$OBJSUF $TGTDIR/$SRCBASE.$DEPSUF|g"
# echo 's|$SRCBASE.$OBJSUF|$TGTDIR/$SRCBASE.$OBJSUF $TGTDIR/$SRCBASE.$DEPSUF|g'

sed -i "s|$SRCBASE.$OBJSUF|$TGTDIR/$SRCBASE.$OBJSUF $TGTDIR/$SRCBASE.$DEPSUF|g" $DEPFILE

rm -f $DEPFILE.bak

exit 0
