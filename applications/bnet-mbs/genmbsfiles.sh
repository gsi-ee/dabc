#!/bin/sh

echo "Generating mbs files"
echo "  Usage: genmbsfiles.sh onlychek FILEBASE numreadouts numevents"

NODEFILE=./nodes.txt

numprocs=`cat $NODEFILE | wc -l`

thisdir=`pwd`

onlycheck=0
filebase="/tmp/genmbs"
numreadouts=4
numevents=10000

if [[ -n $1 ]]; then onlycheck=$1; fi
if [[ -n $2 ]]; then filebase=$2; fi
if [[ -n $3 ]]; then numreadouts=$3; fi
if [[ -n $4 ]]; then numevents=$4; fi

counter=0
for node in `cat $NODEFILE`
do
   echo $node ./genmbsfiles $filebase $counter $numreadouts $numevents $onlycheck
   
   if [[ "$onlycheck" == 0 ]]; then ssh $node rm -f $filebase*; fi

   ssh $node "cd $thisdir; export LD_LIBRARY_PATH=.; ./genmbsfiles $filebase $counter $numreadouts $numevents $onlycheck" &

   counter=`expr $counter + 1`
done
