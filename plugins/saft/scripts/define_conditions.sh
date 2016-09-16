#!/bin/bash
# JAM  define condition that should fire to output IO8 whenever event 0xfffe000000000001 comes:
# first enable output IO8 and set termination resistor low:
/usr/local/bin/saft-io-ctl baseboard -n IO8 -o 1 -t 0
# conditions to rise output IO8 on event and lower it after 100ns:
/usr/local/bin/saft-io-ctl baseboard -n IO8 -c 0xfffffff00000000 0xffffffffffffffff  0 0xf 1 -u
/usr/local/bin/saft-io-ctl baseboard -n IO8 -c 0xfffffff00000000 0xffffffffffffffff  100 0xf 0 -u
echo defined condition for output I08         
/usr/local/bin/saft-io-ctl baseboard -n IO8 -l
