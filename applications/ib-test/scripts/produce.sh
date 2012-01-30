#!/bin/bash

# define place where current topology will be created

dirname=$HOME
#dirname=/scratch/linev
dirname+="/routing/"
dirname+=`date +%Y-%m-%d`

mkdir -p $dirname

filebase=$dirname
filebase+="/"
filebase+=`date +%H:%M:%S`

file0=$filebase"_disc.txt"
file1=$filebase"_comp.txt"
file2=$filebase"_sort.txt"
file3=$filebase"_route.txt"

numlids=$1

if [ "x$numlids" == "x" ]; then numlids="16"; fi

echo "Producing trace files $filebase with numlids=$numlids"

# first produce complete topology

ibnetdiscover -g >$file0

# than extract nodenames and lids for all hosts

awk -f extract.awk $file0 > $file1

#sort all hosts by their names

sort $file1 > $file2

# and the longest time need to trace all routes in the system

#awk -f trace.awk < $file2 > $file3

# -o : means maximal optimisation - can be switched off to test if optimisation is really correctly working
# -c : using cached values for switch lookup tables, speedup tracing

ibtracertm -l $file2 -t $numlids -o -c 1 2 > $file3
