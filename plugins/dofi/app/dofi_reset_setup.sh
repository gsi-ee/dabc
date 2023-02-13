#!/bin/bash
RDOC=${DABCSYS}/bin/rdoficom
#start_dec=0
#stop_dec=191
#for i in $(seq $start_dec $stop_dec);
#do
   #ix=$(printf "0x%x" $i)
  #./spi_muppet1_rw --write ${ix} 0x0    
#done


echo "Restting DOFI setup..."
$RDOC -d -x -w ra-9 0x0 0x00 0xBF 
echo "..................done."
