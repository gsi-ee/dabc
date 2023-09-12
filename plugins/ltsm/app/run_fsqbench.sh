#!/bin/bash
# execute fsqbench several times
# JAM 22-06-2023
##################################
#usage: fsqbench [options]
#        -z, --size <long> [default: 16777216 bytes]
#        -b, --number <int> [default: 16]
#        -t, --threads <int> [default: 1]
#        -d, --wdelay <int> [default: 0]
#        -f, --fsname <string> [default: '/lustre']
#        -a, --fpath <string> [default: '/lustre/fsqbench/']
#        -o, --storagedest {null, local, lustre, tsm, lustre_tsm} [default: 'FSQ_STORAGE_NULL']
#        -n, --node <string>
#        -p, --password <string>
#        -s, --servername <string>
#        -v, --verbose {error, warn, message, info, debug} [default: message]
#        -h, --help
####
SERVER=$1
FSQBIN="/usr/local/bin/fsqbench"
FPATH="/lustre/hades/fsqtest/jun23"
NODE=hadestest
PASS=hadestest 
#DEST=null
DEST=$2
THREADS=4
FIRSTTIMES=1024
BLOCK=4194304
FACTOR=1
SIZE=1
for EXPO in {0..8}
do
    let "FACTOR=1<<$EXPO" 
    let SIZE=$FACTOR*$BLOCK
    let TIMES=$FIRSTTIMES/$FACTOR
    COM="fsqbench -s $SERVER  --fpath $FPATH -n $NODE -p $PASS -o $DEST -t $THREADS -z $SIZE -b $TIMES"
    echo using size $SIZE for $TIMES times from factor $FACTOR
    echo executing $COM
    $COM
done
    
	    
