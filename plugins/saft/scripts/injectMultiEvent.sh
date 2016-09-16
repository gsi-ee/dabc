#!/bin/bash
# JAM  little helper to inject special WB event into of exploder5:
echo injecting multiple event at date: $(/usr/bin/date)  eb-time: $(/usr/local/bin/eb-time dev/wbm0)
/usr/local/bin/saft-ctl baseboard inject 0xfffffff00000000 0x0 0 -p
/usr/local/bin/saft-ctl baseboard inject 0xabcdef0000000000 42 100 -p
/usr/local/bin/saft-ctl baseboard inject 0xdeadaffe00000000 43 100 -p
#echo done at         $(/usr/bin/date)
