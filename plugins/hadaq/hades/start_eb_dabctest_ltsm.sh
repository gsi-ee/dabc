#! /bin/bash
ssh -n hades45 -l adamczew ". /home/adamczew/workspace/dabc2/dabclogin; cd /home/adamczew/workspace/dabc2/plugins/hadaq/hades; export EBNUM=1; export STREAMS=5; export UDP00=10101; export UDP01=10102; export UDP02=10103; export UDP03=10104; export UDP04=10105 export PREFIX=te; export FILEOUTPUTS=3; export OUTDIR=/home/adamczew/data/hadaq; export LTSMSERVER=lxltsm01-tsm-server;  export LTSMNODE=LTSM_TEST01; export LTSMPASSWD=LTSM_TEST01; export LTSMPATH=/home/hadaq/raw/generator; /home/adamczew/workspace/dabc2/bin/dabc_exe EventBuilderHades.xml &" &
#> /dev/null 2>&1  

echo logged in 1

ssh -n hades45 -l adamczew ". /home/adamczew/workspace/dabc2/dabclogin; cd /home/adamczew/workspace/dabc2/plugins/hadaq/hades; export EBNUM=2; export STREAMS=5; export UDP00=10201; export UDP01=10202; export UDP02=10203; export UDP03=10204; export UDP04=10205 export PREFIX=te; export FILEOUTPUTS=3; export OUTDIR=/home/adamczew/data/hadaq; export LTSMSERVER=lxltsm01-tsm-server;  export LTSMNODE=LTSM_TEST01; export LTSMPASSWD=LTSM_TEST01; export LTSMPATH=/home/hadaq/raw/generator; /home/adamczew/workspace/dabc2/bin/dabc_exe EventBuilderHades.xml &" &
#> /dev/null 2>&1  

echo logged in 2


#my $exe_nm = "ssh -n $cpu -l $username \"cd /home/hadaq/oper; export DAQ_SETUP=/home/hadaq/oper/eb; taskset -c $core_nr $cmd_nm $nm_log &\"";
