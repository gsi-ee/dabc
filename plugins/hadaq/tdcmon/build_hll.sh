#!/bin/bash
# JAM 11-jan-2022 - generate list of files on all eventbuilders for given run id
# JAM 31-oct-23 - adjust for changed eventbuilder servers
export RUNID=$1
rm list.hll
servernode=( "08" "09" "10" "11" "08" "09" "10" "11" )
suffix=( "01" "02" "03" "04" "05" "06" "07" "08" )
for filenum in  {0..7}
do
echo /store/${servernode[$filenum]}/01/data/ct${RUNID}${suffix[$filenum]}.hld >> list.hll
ls -l  /store/${servernode[$filenum]}/01/data/ct${RUNID}${suffix[$filenum]}.hld 
done
