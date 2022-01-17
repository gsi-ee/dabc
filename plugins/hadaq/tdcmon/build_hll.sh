#!/bin/bash
# JAM 11-jan-2022 - generate list of files on all eventbuilders for given run id
export RUNID=$1
rm list.hll
servernode=( "08" "09" "14" "15" "16" "08" "09" "14" "15" "16" )
suffix=( "01" "02" "03" "04" "05" "06" "07" "08" "09" "10" )
for filenum in  {0..9}
do
echo /store/${servernode[$filenum]}/01/data/ct${RUNID}${suffix[$filenum]}.hld >> list.hll
ls -l  /store/${servernode[$filenum]}/01/data/ct${RUNID}${suffix[$filenum]}.hld 
done
