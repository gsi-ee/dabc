#!/bin/bash
# JAM 13-02-2023: example how to use rdoficom for example script functgions
RDOC="/daq/usr/adamczew/pi/dabc/git/bin/rdoficom"
echo "input scaler:" 
#start_dec=192
#stop_dec=201
#for i in $(seq $start_dec $stop_dec);
#do
   #ix=$(printf "0x%x" $i)
  #./spi_muppet1_rw --read ${ix}
#done

$RDOC -x -r ra-9 0x100 9 

echo ""
echo "output scaler:" 
#start_dec=256
#stop_dec=265
#for i in $(seq $start_dec $stop_dec);
#do
   #ix=$(printf "0x%x" $i)
  #./spi_muppet1_rw --read ${ix}
#done
$RDOC -x -r ra-9 0xC0 9 


