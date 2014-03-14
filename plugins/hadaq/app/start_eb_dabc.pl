#!/usr/bin/perl -w

use English;
use strict;
use Getopt::Long;
use Data::Dumper;
use Config::Std;
use FileHandle;
use List::MoreUtils qw(any apply first_index);
use File::Basename;
use Cwd;

#- Copy all the arguments because
#  later on the @ARGV becomes empty
my @arg_list = @ARGV;

#- the command line option flags
my $opt_help    = 0;
#my $opt_ebconf  = "/home/hadaq/trbsoft/daq/evtbuild/eb.conf";
my $opt_ebconf  = "/home/joern/dabcwork/head/plugins/hadaq/app/config/eb.conf";
my $opt_ioc = "";
my $opt_test = 0;
my $opt_verb = 0;
my $opt_eb = "";
my @opt_ebrange = ();
my $opt_rfio = 'undef';
my $opt_disk = 'undef';
my $opt_online = 'undef';
my $opt_prefix;

GetOptions ('h|help'      => \$opt_help,
            'c|conf=s'    => \$opt_ebconf,
            'e|eb=s'      => \$opt_eb,
            'i|ioc=s'     => \$opt_ioc,
            't|test'      => \$opt_test,
            'n|nr=s'      => \@opt_ebrange,
            'd|disk=s'    => \$opt_disk,
            'r|rfio=s'    => \$opt_rfio,
            'p|prefix=s'  => \$opt_prefix,
            'o|online=s'  => \$opt_online,
            'v|verb'      => \$opt_verb);

if( $opt_help ) {
    &help();
    exit(0);
}

#- List of EBs provided via command line options
my $active_EBs_aref = &setArgs();

#- Hash with status of CPU cores of EBs (used for 'taskset')
my %EB_CPU_status;
my $EB_CPU_status_href = \%EB_CPU_status;
&init_CPU_status($EB_CPU_status_href);

my $expect_ioc_script = "/tmp/ioc_exit.exp";
my $log_path          = "/tmp/log"; 

my %temp_args;
my $temp_args_href = \%temp_args;
read_config $opt_ebconf => %$temp_args_href;
#print Dumper $temp_args_href;
#exit;

my $numOfEBProcs = 0;
my %EB_Args;
my $EB_Args_href = \%EB_Args;

my @EB_IP_list;

&getEBArgs( $EB_Args_href );

if($opt_ioc eq "start"){
    &killIOC();
    &startIOC();
}
elsif($opt_ioc eq "stop"){
    &killIOC();
}
elsif($opt_eb eq "start"){
    &writeArgs2file();
    &startEvtBuilders();
}
elsif($opt_eb eq "stop"){
    &stopEvtBuilders();
}
elsif($opt_eb eq "restart"){
    &stopEvtBuilders();
    sleep 1;
    &writeArgs2file();
    &startEvtBuilders();
}

exit(0);

################### END OF MAIN ####################

sub help()
{
    print "\n";
    print << 'EOF';
start_eb_gbe.pl

   This script starts parallel Event Building processes.
   The script also starts IOC processes for the run control.

Usage:

   Command line:  start_eb_gbe.pl 
   [-h|--help]                     : Show this help.
   [-c|--conf <path/name>]         : Path to the config file (default: ../evtbuild/eb.conf).
   [-e|--eb <start|stop|restart>]  : Start or stop Event Builders (default: start).
   [-i|--ioc <start|stop>]         : Start or stop IOCs (default: start).
   [-n|--nr <rangeOfEBs>]          : Range of numbers of Event Bulders to be started.
   [-d|--disk <on|off>]            : Switch writing to disk on|off.
   [-r|--rfio <on|off>]            : Switch writing to tape on|off.
   [-p|--prefix <prefix>]          : Prefix of hld file.
   [-o|--online <on|off>]          : Switch RPC server on|off.
   [-t|--test]                     : Test without execution.
   [-v|--verb]                     : More verbouse.

Examples:

   Start 6 EBs with the numbers 1,2,3,5,7 and prefix 'md':
   start_eb_gbe.pl -e start -n 1-3 -n 5 -n 7 -p md

   Start EBs and enable writing to disks but disable writing to tape for all EBs:
   start_eb_gbe.pl -e start --disk on --rfio off

EOF
}

sub init_CPU_status()
{
    my ($EB_CPU_status_href) = @_;

    # CPU affinity with 'taskset'
    #
    # CPU dec     bin  hex
    #     0         1    1
    #     1        10    2 
    #     2       100    4
    #     3      1000    8
    #     4     10000   10 

    #cores 0/1 reserved for system 02-05
    #cores 2/3 reserved for interrupts on 02-05
    
    foreach my $core (0..7){
        if($core == 1){
            $EB_CPU_status_href->{'192.168.100.11'}->{$core} = "res"; #reserved
        }
        else{
            $EB_CPU_status_href->{'192.168.100.11'}->{$core} = "free";
        }
    }

    foreach my $core (0..11){
        if($core < 4){
            $EB_CPU_status_href->{'192.168.100.12'}->{$core} = "res"; #reserved
            $EB_CPU_status_href->{'192.168.100.13'}->{$core} = "res"; #reserved
            $EB_CPU_status_href->{'192.168.100.14'}->{$core} = "res"; #reserved
        }
        else{
            $EB_CPU_status_href->{'192.168.100.12'}->{$core} = "free";
            $EB_CPU_status_href->{'192.168.100.13'}->{$core} = "free";
            $EB_CPU_status_href->{'192.168.100.14'}->{$core} = "free";
        }
    }

    foreach my $core (0..23){
        if($core < 4){
            $EB_CPU_status_href->{'192.168.100.15'}->{$core} = "res"; #reserved
        }
        else{
            $EB_CPU_status_href->{'192.168.100.15'}->{$core} = "free";
        }
    }

}

sub getCoreNr()
{
    my ($ip) = @_;

    my $core_nr;

    foreach my $eb_ip (sort keys %$EB_CPU_status_href){
        next unless($ip eq $eb_ip);
            
        foreach my $core ( sort {$a <=> $b} keys %{$EB_CPU_status_href->{$eb_ip}} ){
            my $core_status = $EB_CPU_status_href->{$eb_ip}->{$core};
            
            next unless(lc($core_status) eq "free");

            $core_nr = $core;
            $EB_CPU_status_href->{$eb_ip}->{$core} = "busy";
            last;
        }
    }

    #- If no free cores left - take reserved cores
    unless( defined $core_nr ){
        foreach my $eb_ip (sort keys %$EB_CPU_status_href){
            next unless($ip eq $eb_ip);
            
            foreach my $core ( sort {$a <=> $b} keys %{$EB_CPU_status_href->{$eb_ip}} ){
                my $core_status = $EB_CPU_status_href->{$eb_ip}->{$core};
                
                if(lc($core_status) eq "res"){
                    $core_nr = $core;
                    $EB_CPU_status_href->{$eb_ip}->{$core} = "busy";
                    last;
                }
            }
        }
    }

    unless( defined $core_nr ){
        print "No free cores left on CPU $ip. Exit.\n";
        exit(0);
    }

    return $core_nr;
}

sub setArgs()
{
    my @active_EBs;

    if(@opt_ebrange){
        foreach my $range (@opt_ebrange){
            if($range =~ /(\d+)-(\d+)/){
                my $max = $1;
                my $min = $2;
                
                foreach my $eb ($max..$min){
                    #- 1 must be subtracted to match
                    #  EB numbering in the register_configgbe_ip.db
                    #  which starts from zero
                    &checkEB_nr($eb);
                    push(@active_EBs, $eb-1);
                }
            }
            elsif($range =~ /(\d+)/){
                &checkEB_nr($1);
                push(@active_EBs, $1-1);
            }
        }
    }

    return \@active_EBs;
}

sub checkEB_nr()
{
    my ($eb_nr) = @_;

    if( $eb_nr < 1 || $eb_nr > 16 ){
        print "ERROR: EB number should be in the range 1-16. Exit.";
        exit(0);
    }
}

sub getEBArgs()
{
    my ($href) = @_;

    my $prefix = $temp_args_href->{'Main'}->{'EB_EXT'};
    $prefix = $opt_prefix if( defined $opt_prefix );
    my $filesize = $temp_args_href->{'Main'}->{'EB_FSIZE'};

    my $base_port  = $temp_args_href->{'Parallel'}->{'BASE_PORT'};
    my $shift_port = $temp_args_href->{'Parallel'}->{'SHIFT_PORT'};
    my $source_num = $temp_args_href->{'Parallel'}->{'NUM_OF_SOURCES'};
    my $queuesize  = $temp_args_href->{'Parallel'}->{'QUEUESIZE'};

    my $multidisk  = $temp_args_href->{'Parallel'}->{'MULTIDISK'};

    #- Number of EB process
    my $ebproc = 0;

    #- List of BEs
    my $listOfEBs = $temp_args_href->{'Parallel'}->{'EB_LIST'};
    my @eb_list = split(/\s+/, $listOfEBs);

    #- DABC mode selection
    my $listOfDABC = $temp_args_href->{'Parallel'}->{'DABC'};
    my @dabc_list = split(/\s+/, $listOfDABC);


    #- Default RFIO settings
    my $rfio               = $temp_args_href->{'Parallel'}->{'RFIO'};
    my $rfio_path          = $temp_args_href->{'Parallel'}->{'RFIO_PATH'};
    my $rfio_pcOptions     = $temp_args_href->{'Parallel'}->{'RFIO_pcOptions'};
    my $rfio_iCopyMode     = $temp_args_href->{'Parallel'}->{'RFIO_iCopyMode'};
    my $rfio_pcCopyPath    = $temp_args_href->{'Parallel'}->{'RFIO_pcCopyPath'};
    my $rfio_iCopyFrac     = $temp_args_href->{'Parallel'}->{'RFIO_iCopyFraction'};
    my $rfio_iMaxFile      = $temp_args_href->{'Parallel'}->{'RFIO_iMaxFile'};
    my $rfio_iPathConv     = $temp_args_href->{'Parallel'}->{'RFIO_iPathConvention'};
    
    my @rfio_list = split(/\s+/, $rfio);

    #- EPICS Controled 
    my $epics_ctrl = $temp_args_href->{'Parallel'}->{'EPICS_CTRL'};

    my @epics_list = split(/\s+/, $epics_ctrl);

    #- Logging the output of EBs
    my $eb_log     = $temp_args_href->{'Parallel'}->{'EB_LOG'};
    my $nm_log     = $temp_args_href->{'Parallel'}->{'NM_LOG'};
    my @eblog_list =  split(/\s+/, $eb_log);
    my @nmlog_list =  split(/\s+/, $nm_log);

    #- Write to disk
    my $write2disk = $temp_args_href->{'Parallel'}->{'WRITE_TO_DISK'};
    my @write2disk_list = split(/\s+/, $write2disk);

    #--- Read GbE configuration
    my %eb_ids_gbe_hash;
    my $eb_ids_gbe_href = \%eb_ids_gbe_hash;

    &getGbEconfig($eb_ids_gbe_href);

    #--- Loop over all EB processes
    #print Dumper $eb_ids_gbe_href;
    #exit;
    foreach my $ebproc ( sort keys %{$eb_ids_gbe_href} ){

        #- If there was a list of EBs provided via command line options
        #  go to the next $ebproc if the current $ebproc is not in this list.
        #print "active EBs:\n";
        #print Dumper $active_EBs_aref;

        if(@$active_EBs_aref){
            next unless( any {$_ == $ebproc} @$active_EBs_aref );  #from command line args
        }
        else{
            next unless( $eb_list[$ebproc] );  #from eb.conf
        }


        my $eb_ip = $eb_ids_gbe_href->{$ebproc}->{'IP'};

        #- Save IP needed by other function to stop EBs.
        push(@EB_IP_list, $eb_ip) unless( any {$_ eq $eb_ip} @EB_IP_list );

        #- Some checks on number of EB processes
        die "Number of EB processes exceeds the number in RFIO setting! Exit." if($ebproc > $#rfio_list);
        die "Number of EB processes exceeds the number in EPICS_CTRL setting! Exit." if($ebproc > $#epics_list);
        
        #- Here we can overwrite default rfio settings with individual settings per EB processes
        my $procname = sprintf("EB_PROC_%d", 1+$ebproc);
        # $rfio_iCopyMode     = $temp_args_href->{$procname}->{'RFIO_iCopyMode'};
        
        $href->{$ebproc}->{'IP'}        = $eb_ip;
        $href->{$ebproc}->{'EBNUM'}     = $ebproc+1;
        $href->{$ebproc}->{'BASEPORT'}  = $base_port;
        $href->{$ebproc}->{'PORT_LIST'} = $eb_ids_gbe_href->{$ebproc}->{'port_list'};
        $href->{$ebproc}->{'SOURCENUM'} = scalar @{$eb_ids_gbe_href->{$ebproc}->{'port_list'}};
        $href->{$ebproc}->{'BUFSIZE_LIST'} = $eb_ids_gbe_href->{$ebproc}->{'bufsize_list'};
        $href->{$ebproc}->{'PREFIX'}    = $prefix;
        $href->{$ebproc}->{'QUEUESIZE'} = $queuesize;
        $href->{$ebproc}->{'MULTIDISK'} = $multidisk;
        $href->{$ebproc}->{'FILESIZE'} = $filesize;

        if( defined $temp_args_href->{$procname}->{'MULTIDISK'} ){
            $href->{$ebproc}->{'MULTIDISK'} = $temp_args_href->{$procname}->{'MULTIDISK'};
        }
        elsif($multidisk){
            $href->{$ebproc}->{'MULTIDISK'} = $href->{$ebproc}->{'EBNUM'};
        }
        else{
            $href->{$ebproc}->{'MULTIDISK'} = $multidisk;
        }

        if( defined $temp_args_href->{$procname}->{'RESDOWNSCALE'} ){
            $href->{$ebproc}->{'RESDOWNSCALE'} = $temp_args_href->{$procname}->{'RESDOWNSCALE'};
            $href->{$ebproc}->{'RESNUMEVENTS'} = $temp_args_href->{$procname}->{'RESNUMEVENTS'};
            $href->{$ebproc}->{'RESPATH'}      = $temp_args_href->{$procname}->{'RESPATH'};
            $href->{$ebproc}->{'RESSIZELIMIT'} = $temp_args_href->{$procname}->{'RESSIZELIMIT'};
        }

        if( defined $temp_args_href->{$procname}->{'ONLINESERVER'} ){
            if($opt_online eq "on"){
                $href->{$ebproc}->{'ONLINESERVER'} = "on";
            }
            elsif($opt_online eq "off"){
                $href->{$ebproc}->{'ONLINESERVER'} = "off";
            }
            else{
                $href->{$ebproc}->{'ONLINESERVER'} = $temp_args_href->{$procname}->{'ONLINESERVER'};
            }
        }
        else{
            $href->{$ebproc}->{'ONLINESERVER'} = "off";
        }

        $href->{$ebproc}->{'RFIO'}             = $rfio_list[$ebproc] if(lc($opt_rfio) eq 'undef');  # 0|1
        $href->{$ebproc}->{'RFIO'}             = 1 if(lc($opt_rfio) eq 'on');  # 0|1
        $href->{$ebproc}->{'RFIO'}             = 0 if(lc($opt_rfio) eq 'off');  # 0|1
        $href->{$ebproc}->{'RFIO_PATH'}        = $rfio_path;
        $href->{$ebproc}->{'RFIO_pcOptions'}   = $rfio_pcOptions;
        $href->{$ebproc}->{'RFIO_iCopyMode'}   = $rfio_iCopyMode;
        $href->{$ebproc}->{'RFIO_pcCopyPath'}  = $rfio_pcCopyPath;
        $href->{$ebproc}->{'RFIO_iCopyFrac'}   = $rfio_iCopyFrac;
        $href->{$ebproc}->{'RFIO_iMaxFile'}    = $rfio_iMaxFile;
        $href->{$ebproc}->{'RFIO_iPathConv'}   = $rfio_iPathConv;

        $href->{$ebproc}->{'EPICS_CTRL'}       = $epics_list[$ebproc];  # 0|1

	$href->{$ebproc}->{'DABC'}       = $dabc_list[$ebproc];  # 0|1


        $href->{$ebproc}->{'EB_LOG'}       = $eblog_list[$ebproc];  # 0|1
        $href->{$ebproc}->{'NM_LOG'}       = $nmlog_list[$ebproc];  # 0|1

        if( $write2disk_list[$ebproc] && lc($opt_disk) eq 'undef' ){
            if(&isVarDefined($temp_args_href->{$procname}->{'OUTDIR'}, "OUTDIR for $procname")){
                $href->{$ebproc}->{'OUTDIR'} = $temp_args_href->{$procname}->{'OUTDIR'};
            }
        }
        elsif( lc($opt_disk) eq 'on' ){
            if(&isVarDefined($temp_args_href->{$procname}->{'OUTDIR'}, "OUTDIR for $procname")){
                $href->{$ebproc}->{'OUTDIR'} = $temp_args_href->{$procname}->{'OUTDIR'};
            }
        }
        elsif( lc($opt_disk) eq 'off' ){
            #- do not do anything. If $href->{$ebproc}->{'OUTDIR'} is undefined, 
            #  the data will go to /dev/null
        }
    }    

    $numOfEBProcs = $ebproc;
}

sub isVarDefined()
{
    my ($var, $msg) = @_;

    my $retval = 1;

    unless( defined $var ){
        print "Undefined variable found: $msg\n";
        $retval = 0;
    }
        
    return $retval;
}

sub getVarSizeArg()
{
    my ($ebproc) = @_;
    
    my $i = 0;
    my $arg = " ";

    foreach my $size (@{$EB_Args_href->{$ebproc}->{'BUFSIZE_LIST'}}){

        if($EB_Args_href->{$ebproc}->{'BUFSIZE_LIST'}->[$i] == 
           $EB_Args_href->{$ebproc}->{'QUEUESIZE'}){
            $i++;
            next;
        }

        $arg = $arg . " -Q " . $i . ":" . $EB_Args_href->{$ebproc}->{'BUFSIZE_LIST'}->[$i];
        $i++;
    }

    return $arg;
}

sub startEvtBuilders()
{

    my $username = "joern";

    my (@process_list);

    foreach my $ebproc (sort {$a <=> $b} keys %$EB_Args_href){

        my $ebnum2print = $ebproc+1;
        print "EB process: $ebnum2print\n";


# JAM first test if we should activate dabc eventbuilder or old one

 if( $EB_Args_href->{$ebproc}->{'DABC'} ){
    print "Starting DABC process..\n";
# TODO: here evaluate parameters and code into dabc command execution

#". /home/joern/dabcwork/head/dabclogin;cd /home/joern/dabcwork/head/plugins/hadaq/app; export EBNUM=1; export STREAMS=5; export UDP00=10101; export UDP01=10102; export UDP02=10103; export UDP03=10104; export UDP04=10105 export PREFIX=be; /home/joern/dabcwork/head/bin/dabc_exe EventBuilderHades.xml &" > /dev/null 2>&1  &

  my $cpu = $EB_Args_href->{$ebproc}->{'IP'};
  my $dabclogin = ". /home/joern/dabcwork/head/dabclogin;";
  my $cdworkdir = "cd /home/joern/dabcwork/head/plugins/hadaq/app;";

  my $cmd_dabc = "/home/joern/dabcwork/head/bin/dabc_exe ";
  my $conf_dabc = " EventBuilderHades.xml";
  my $exports = " export EBNUM=" . $EB_Args_href->{$ebproc}->{'EBNUM'} . "; " .
		" export STREAMS=" . $EB_Args_href->{$ebproc}->{'SOURCENUM'} . "; " .
		" export PREFIX=" . $EB_Args_href->{$ebproc}->{'PREFIX'}. "; " ;

   my @port_list = ();

        #- add ports
	my $ix =0;
        foreach my $port (@{$EB_Args_href->{$ebproc}->{'PORT_LIST'}}){ 
            #$cmd_nm = $cmd_nm . " -i UDP:0.0.0.0:" . $port;
	    my $index=sprintf("%02d", $ix++);
            $exports = $exports . " export UDP". $index. "=" . $port . "; "; 
            push(@port_list, $port);
     }
        &cpPortList2EB(\@port_list, $EB_Args_href->{$ebproc}->{'EBNUM'}, $cpu);






# 	MULTIDISK

#- add output type

	if($EB_Args_href->{$ebproc}->{'OUTDIR'} ){
	       $exports = $exports . " export FILEOUTPUTS=2;";
             if($EB_Args_href->{$ebproc}->{'MULTIDISK'}){
	         $exports = $exports . "export DAQDISK=1; export OUTDIR=/home/joern/data; ";
             }
             else{
		 $exports = $exports . "export DAQDISK=0; export OUTDIR=" . $EB_Args_href->{$ebproc}->{'OUTDIR'} .";";
             }
         }
	  else{
	        $exports = $exports . " export FILEOUTPUTS=1;";
		# no output except for the stream server...
         }


# TODO: additional exports for
# 	RFIO
  if( $EB_Args_href->{$ebproc}->{'RFIO'} ){
	$exports = $exports . " export FILEOUTPUTS=3;";
	$exports = $exports . " export RFIOPATH=". $EB_Args_href->{$ebproc}->{'RFIO_PATH'} . ";"

}
# TODO: implement lustre copy to dabc!


#  my $rfio;
#         if( $EB_Args_href->{$ebproc}->{'RFIO'} ){
#             $rfio = " --rfio "       . $EB_Args_href->{$ebproc}->{'RFIO_PATH'} .
#                 " --rfiolustre "     . $EB_Args_href->{$ebproc}->{'RFIO_pcCopyPath'} .
#                 " --rfio_pcoption "  . $EB_Args_href->{$ebproc}->{'RFIO_pcOptions'} .
#                 " --rfio_icopymode " . $EB_Args_href->{$ebproc}->{'RFIO_iCopyMode'} .
#                 " --rfio_icopyfrac " . $EB_Args_href->{$ebproc}->{'RFIO_iCopyFrac'} .
#                 " --rfio_imaxfile "  . $EB_Args_href->{$ebproc}->{'RFIO_iMaxFile'} .
#                 " --rfio_ipathconv " . $EB_Args_href->{$ebproc}->{'RFIO_iPathConv'};
#         }


# 	EPICSCONTROL ? always enabled for production
#       SMALLFILES  for online monitoring node


  
#   my $core_nr = &getCoreNr($cpu);
# JAM use fixed core number for kp1pc092 tests:
   my $core_nr = 1;
   my $exe_dabc = "ssh -n $cpu -l $username \"$dabclogin $cdworkdir $exports taskset -c $core_nr  $cmd_dabc $conf_dabc &\"";



   #my $log = $log_path . "/log_" . $ebproc . "_" . "startEB.txt";
   my $log = "/dev/null 2>&1";
   
   print "Forking:" . $exe_dabc ."\n";
   forkMe($exe_dabc, $log, \@process_list) unless($opt_test);

}

else
{
# the standard EB processes mode:
 print "Starting evtbuild/netmem processes..\n";

        #--- Prepare execution of daq_evtbuild
        my $cmd_eb = "/home/hadaq/bin/daq_evtbuild" .
            " -m "          . $EB_Args_href->{$ebproc}->{'SOURCENUM'} . 
            " -q "          . $EB_Args_href->{$ebproc}->{'QUEUESIZE'} . 
            " -S "          . $EB_Args_href->{$ebproc}->{'EBNUM'} .
            " --ebnum "     . $EB_Args_href->{$ebproc}->{'EBNUM'} . 
            " -x "          . $EB_Args_href->{$ebproc}->{'PREFIX'};

        #- add queue variable size args
        my $varsize_arg = &getVarSizeArg($ebproc);
        $cmd_eb = $cmd_eb . $varsize_arg;

        #- add output type
        if( defined $EB_Args_href->{$ebproc}->{'OUTDIR'} ){
            if($EB_Args_href->{$ebproc}->{'MULTIDISK'}){
                $cmd_eb = $cmd_eb . " -d file -o " . "/data01/data";
            }
            else{
                $cmd_eb = $cmd_eb . " -d file -o " . $EB_Args_href->{$ebproc}->{'OUTDIR'};
            }
        }
        else{
            $cmd_eb = $cmd_eb . " -d null";
        }

        #- add file size
        $cmd_eb = $cmd_eb . " --filesize " . $EB_Args_href->{$ebproc}->{'FILESIZE'};

        #- add second output with small hdl files
        if( defined $EB_Args_href->{$ebproc}->{'RESDOWNSCALE'} ){
            $cmd_eb = $cmd_eb . " --resdownscale " . $EB_Args_href->{$ebproc}->{'RESDOWNSCALE'} .
                                " --resnumevents " . $EB_Args_href->{$ebproc}->{'RESNUMEVENTS'} .
                                " --respath "      . $EB_Args_href->{$ebproc}->{'RESPATH'} .
                                " --ressizelimit " . $EB_Args_href->{$ebproc}->{'RESSIZELIMIT'};
        }

        my $cpu = $EB_Args_href->{$ebproc}->{'IP'};

        #- add rfio args
        my $rfio;
        if( $EB_Args_href->{$ebproc}->{'RFIO'} ){
            $rfio = " --rfio rfiodaq:gstore:" . $EB_Args_href->{$ebproc}->{'RFIO_PATH'} .
                " --rfiolustre "     . $EB_Args_href->{$ebproc}->{'RFIO_pcCopyPath'} .
                " --rfio_pcoption "  . $EB_Args_href->{$ebproc}->{'RFIO_pcOptions'} .
                " --rfio_icopymode " . $EB_Args_href->{$ebproc}->{'RFIO_iCopyMode'} .
                " --rfio_icopyfrac " . $EB_Args_href->{$ebproc}->{'RFIO_iCopyFrac'} .
                " --rfio_imaxfile "  . $EB_Args_href->{$ebproc}->{'RFIO_iMaxFile'} .
                " --rfio_ipathconv " . $EB_Args_href->{$ebproc}->{'RFIO_iPathConv'};
        }

        $cmd_eb = $cmd_eb . $rfio if( defined $rfio );

        #- add multiple disk arg (ctrl via daq_disks)
        if($EB_Args_href->{$ebproc}->{'MULTIDISK'} && 
           defined $EB_Args_href->{$ebproc}->{'OUTDIR'}){
            $cmd_eb = $cmd_eb . " --multidisk " . $EB_Args_href->{$ebproc}->{'MULTIDISK'};
        }

        #- add online RPC server
        if( $EB_Args_href->{$ebproc}->{'ONLINESERVER'} eq "on" ){
            $cmd_eb = $cmd_eb . " --online";
        }

        #- add epics controlled
        $cmd_eb = $cmd_eb . " --epicsctrl " if( $EB_Args_href->{$ebproc}->{'EPICS_CTRL'} );

        #- logging the output
        my $eblog_file = "/tmp/log_eb_" . $EB_Args_href->{$ebproc}->{'EBNUM'} . ".txt";
        my $eb_log = "1>$eblog_file 2>$eblog_file";
        $eb_log = "1>/dev/null 2>/dev/null" unless( $EB_Args_href->{$ebproc}->{'EB_LOG'} );

        my $time = 1. * $ebproc;
        my $sleep_cmd = "sleep " . $time;

        my $core_nr = &getCoreNr($cpu);

        my $exe_eb = "ssh -n $cpu -l $username \"cd /home/hadaq/oper; export DAQ_SETUP=/home/hadaq/oper/eb; taskset -c $core_nr  $cmd_eb $eb_log &\"";

        #print "exec: $exe_eb\n";

        #--- Prepare execution of daq_netmem
        my $cmd_nm = "/home/hadaq/bin/daq_netmem" .
            " -m " . $EB_Args_href->{$ebproc}->{'SOURCENUM'} . 
            " -q " . $EB_Args_href->{$ebproc}->{'QUEUESIZE'} . 
            " -S " . $EB_Args_href->{$ebproc}->{'EBNUM'};

        #- add queue variable size args
        $cmd_nm = $cmd_nm . $varsize_arg;

        my @port_list = ();

        #- add ports
        foreach my $port (@{$EB_Args_href->{$ebproc}->{'PORT_LIST'}}){ 
            #$cmd_nm = $cmd_nm . " -i UDP:0.0.0.0:" . $port;
            $cmd_nm = $cmd_nm . " -i " . $port;

            push(@port_list, $port);
        }

        &cpPortList2EB(\@port_list, $EB_Args_href->{$ebproc}->{'EBNUM'}, $cpu);

        #- logging the output
        my $nmlog_file = "/tmp/log_nm_" . $EB_Args_href->{$ebproc}->{'EBNUM'} . ".txt";
        my $nm_log = "1>$nmlog_file 2>$nmlog_file";
        $nm_log = "1>/dev/null 2>/dev/null" unless( $EB_Args_href->{$ebproc}->{'NM_LOG'} );

        $core_nr = &getCoreNr($cpu);

        my $exe_nm = "ssh -n $cpu -l $username \"cd /home/hadaq/oper; export DAQ_SETUP=/home/hadaq/oper/eb; taskset -c $core_nr $cmd_nm $nm_log &\"";

        #print "exec: $exe_nm\n";

        #--- Open permissions for shared memory
        my $eb_shmem = "daq_evtbuild" . $EB_Args_href->{$ebproc}->{'EBNUM'} . ".shm";
        my $nm_shmem = "daq_netmem" . $EB_Args_href->{$ebproc}->{'EBNUM'} . ".shm";
        my $exe_open_eb = "ssh -n $cpu -l $username \"chmod 775 /dev/shm/$eb_shmem\"";
        my $exe_open_nm = "ssh -n $cpu -l $username \"chmod 775 /dev/shm/$nm_shmem\"";
        
        &forkEB($exe_eb, $exe_nm, $exe_open_eb, $exe_open_nm, \@process_list);
    }

} 
# if dabc

    #- Wait for children
    foreach my $cur_child_pid (@process_list) {
        waitpid($cur_child_pid,0);
    }

} 
# foreach

sub stopEvtBuilders()
{
    my $username = "joern";

    my @process_list = ();

    #--- Loop over server IPs
    foreach my $ip (@EB_IP_list){

        my $exe = "ssh -n $ip -l $username \"/home/joern/dabcwork/head/plugins/hadaq/app/cleanup_evtbuild.pl;\"";

        if($opt_verb){
            print "Killing running EBs...\n";
            print "Exec: $exe\n";
        }

        my $log = $log_path . "/log_" . $ip . "_" . "stopEB.txt";

        forkMe($exe, $log, \@process_list) unless($opt_test);
    }

    #- Wait for children
    foreach my $cur_child_pid (@process_list) {
        print "wait for $cur_child_pid\n";
        waitpid($cur_child_pid,0);
    }
}

sub cpPortList2EB()
{
    my ($port_list_aref, $ebnr, $cpu) = @_;

    my $tmpfile = "/tmp/eb" . $ebnr . "_" . $cpu . ".txt";

    #- First write ports to tmp file
    my $fh = new FileHandle(">$tmpfile");

    if(!$fh) {
        my $txt = "\nError! Could not open file \"$tmpfile\" for output. Exit.\n";
        print STDERR $txt;
        print $txt;
        exit(128);
    }
    
    foreach my $port (@$port_list_aref){
        print $fh "$port\n";
    }

    $fh->close();

    #- Copy this tmp file to EB
    my $exe_cp = "scp $tmpfile joern\@$cpu:/tmp/ 1>/dev/null 2>/dev/null";
    system($exe_cp);
}

sub startIOC()
{
    my $ioc_dir = "/home/scs/ebctrl/ioc/iocBoot/iocebctrl";

    &writeIOC_stcmd( $ioc_dir );

    print "Starting IOCs...\n" if($opt_verb);

    foreach my $ebproc (keys %$EB_Args_href){

        my $stcmd = sprintf("st_eb%02d.cmd", 1 + $ebproc);
        my $screen_name = sprintf("ioc_eb%02d", 1 + $ebproc);

        my $cmd = "bash; . /home/scs/.bashrc; export HOSTNAME=\\\$(hostname); cd $ioc_dir; screen -dmS $screen_name ../../bin/linux-x86_64/ebctrl $stcmd";
        my $cpu = $EB_Args_href->{$ebproc}->{'IP'};

        my $exe = "ssh -n $cpu -l scs \"$cmd\"";

        print "Exec: $exe\n" if($opt_verb);
        system($exe) unless($opt_test);
    }
}

sub smallestEBProcNum()
{
    my $smallest = 1000;

    foreach my $ebproc (keys %$EB_Args_href){
        $smallest = $ebproc if($smallest > $ebproc);
    }

    return $smallest;
}

sub writeIOC_stcmd()
{
    my ($ioc_dir) = @_;

    print "Copying st.cmd files to servers...\n" if($opt_verb);

    my $smallest_ebproc = &smallestEBProcNum();

    foreach my $ebproc (keys %$EB_Args_href){

        my $ebNr  = 1 + $ebproc;
        my $ebnum = sprintf("eb%02d", $ebNr);

        #- in MBytes
        my $maxFileSize = $EB_Args_href->{$ebproc}->{'FILESIZE'};

        my $ebtype = "slave";
        my $comment_genrunid   = "#";
        my $comment_totalevt   = "#";

        if($ebproc == $smallest_ebproc){
            $ebtype           = "master";
            $comment_genrunid = "";
            $comment_totalevt   = "";
        }

#        if($ebNr == 1){
#            $comment_totalevt   = "";
#        }

        my $ioc_stcmd = <<EOF;
#!../../bin/linux-x86_64/ebctrl

## Set EPICS environment

< envPaths

epicsEnvSet(FILESIZE,"$maxFileSize")
epicsEnvSet(EBTYPE,"$ebtype")
epicsEnvSet(EBNUM,"$ebNr")
epicsEnvSet(ERRBITLOG, "1")
epicsEnvSet(ERRBITWAIT, "30")
epicsEnvSet(EPICS_CA_ADDR_LIST,"192.168.103.255")
epicsEnvSet(EPICS_CA_AUTO_ADDR_LIST,"NO")
epicsEnvSet(PATH,"/home/scs/base-3-14-11/bin/linux-x86_64:/usr/local/bin:/usr/bin:/bin:/usr/bin/X11:.")

cd \${TOP}

## Register all support components
dbLoadDatabase("dbd/ebctrl.dbd")
ebctrl_registerRecordDeviceDriver(pdbbase)

## Load record instances
dbLoadRecords("db/stats.db", "PREFIX=HAD:IOC:,IOC=$ebnum")
dbLoadRecords("db/evtbuild.db","eb=$ebnum")
dbLoadRecords("db/netmem.db","eb=$ebnum")
dbLoadRecords("db/errbit1.db","eb=$ebnum")
dbLoadRecords("db/errbit2.db","eb=$ebnum")
dbLoadRecords("db/trignr1.db","eb=$ebnum")
dbLoadRecords("db/trignr2.db","eb=$ebnum")
dbLoadRecords("db/portnr1.db","eb=$ebnum")
dbLoadRecords("db/portnr2.db","eb=$ebnum")
dbLoadRecords("db/trigtype.db","eb=$ebnum")
dbLoadRecords("db/cpu.db","eb=$ebnum")
dbLoadRecords("db/errbitstat.db","eb=$ebnum")
$comment_totalevt dbLoadRecords("db/totalevtstat.db")
$comment_genrunid dbLoadRecords("db/genrunid.db","eb=$ebnum")

## Set this to see messages from mySub
var evtbuildDebug 0
var netmemDebug 0
var genrunidDebug 0
var writerunidDebug 0
var errbit1Debug 0
var errbit2Debug 0
var trigtypeDebug 0
var cpuDebug 0
var errbitstatDebug 0
$comment_totalevt var totalevtscompDebug 0
cd \${TOP}/iocBoot/\${IOC}
iocInit()

## Start any sequence programs
#seq sncExample,"user=scsHost"

dbl > \${TOP}/iocBoot/\${IOC}/$ebnum.dbl

EOF

        my $outfile = "/tmp/st_" . $ebnum . ".cmd";
        my $fh = new FileHandle(">$outfile");

        if(!$fh) {
            my $txt = "\nError! Could not open file \"$outfile\" for output. Exit.\n";
            print STDERR $txt;
            print $txt;
            exit(128);
        }

        print $fh $ioc_stcmd;
        $fh->close();

        my $ip  = $EB_Args_href->{$ebproc}->{'IP'};
        my $cmd = "scp $outfile scs\@$ip:$ioc_dir/.";
        
        print "Exec: $cmd\n" if($opt_verb);
        system($cmd) unless($opt_test);        
    }
}

sub killIOC()
{
    my %ioc;
    my $ioc_href = \%ioc;

    print "Looking for running IOCs...\n" if($opt_verb);

    #--- Loop over server IPs
    foreach my $ip (@EB_IP_list){

        &findRunningIOC($ip, $ioc_href);
    }

    #print Dumper \%$ioc_href;

    &writeExpectIOC() if(%$ioc_href);

    if($opt_verb){
        print "Killing running IOCs...\n";
        print "No IOCs found - nothing to kill, continue...\n" unless(%$ioc_href);
    }

    my (@process_list);

    foreach my $ip ( %$ioc_href ){
        foreach my $ioc ( @{$ioc_href->{$ip}} ){
            
            my $cmd = $expect_ioc_script . " " . $ip . " " . $ioc;
            my $log = $log_path . "/log_" . $ip . "_" . $ioc . ".txt";
            print "cmd: $cmd\n" if($opt_verb);
            &forkMe($cmd, $log, \@process_list);
        }
    }

    #- Wait for children
    foreach my $cur_child_pid (@process_list) {
        waitpid($cur_child_pid,0);
    }
}

sub forkMe()
{
    my ($cmd, $log, $proc_list) = @_;

    my $child = fork();

    if( $child ){                           # parent
        push( @$proc_list, $child );
    }
    elsif( $child == 0 ) {                        # child
        system("$cmd > $log");
        exit(0);
    }
    else{
        print "Could not fork: $!\n";
        exit(1);
    }
}

sub forkEB()
{
    my ($exe_eb, $exe_nm, $exe_open_eb, $exe_open_nm, $proc_list) = @_;

    my $child = fork();

    if( $child ){                           # parent
        push( @$proc_list, $child );
    }
    elsif( $child == 0 ) {                        # child
        #--- Execute Event Builder
        print "Exec: $exe_eb\n" if($opt_verb);
        system($exe_eb) unless($opt_test);

        sleep(1);

        #--- Open permissions for EB shared memory
        #  ! Permissions should be opened by EB process
        #print "Exec: $exe_open_eb\n" if($opt_verb);
        #system($exe_open_eb) unless($opt_test);

        sleep(2);

        #--- Execute Net-2-Memory
        print "Exec: $exe_nm\n" if($opt_verb);
        system($exe_nm) unless($opt_test);

        sleep(1);

        #--- Open permissions for NM shared memory
        #  ! Permissions should be opened by EB process
        #print "Exec: $exe_open_nm\n" if($opt_verb);
        #system($exe_open_nm) unless($opt_test);

        exit(0);
    }
    else{
        print "Could not fork: $!\n";
        exit(1);
    }
}

sub findRunningIOC()
{
    my ($cpu, $ioc_href) = @_;
    
    `ssh -n $cpu -l scs \"screen -wipe\"`;
    my $exe = "ssh -n $cpu -l scs \"screen -ls\"";

    my @output = `$exe`;

    foreach my $line (@output){
        if($line =~ /\d+\.(ioc_eb\d{2})\s+/){
            my $name = $1;
            push( @{$ioc_href->{$cpu}}, $name );
            print "Found IOC: $name on $cpu\n" if($opt_verb);
        }
    }
}

sub writeExpectIOC()
{
    # This expect script can be executed to exit IOC.

    #! Look if /tmp dir exists
    my $tmp_dir = dirname("/tmp");
    if ( !(-d $tmp_dir) ){
        print "\nCannot access /tmp directory!\nExit.\n";
        exit(1);
    }

    my $expect_script_my = <<EOF;
#!/usr/bin/expect -f

# This script is automatically generated by startup.pl
# Do not edit, the changes will be lost.

# Print args
send_user "\$argv0 [lrange \$argv 0 \$argc]\\n"

# Get args
#
# ip      : IP address of the server
# iocname : name of IOC screen process (screen -ls)
#
if {\$argc>0} {
  set ip      [lindex \$argv 0]
  set iocname [lindex \$argv 1]
} else {
  send_user "Usage: \$argv0 ip iocname\\n"
}

spawn ssh scs@\$ip

#expect {
#        "error"     { exit; }
#        "login:"    { exit; }
#        "Password:" { exit; }
#}

set timeout 20
#240

expect "~\$ "
send   "screen -r \$iocname\\r"
expect "epics> "
send   "exit\\r"
expect "~\$ "
    
EOF

    my $fh = new FileHandle(">$expect_ioc_script");

    if(!$fh) {
        my $txt = "\nError! Could not open file \"$expect_ioc_script\" for output. Exit.\n";
        print STDERR $txt;
        print $txt;
        exit(128);
    }

    print $fh $expect_script_my;
    $fh->close();    

    #- open permissions
    system("chmod 755 $expect_ioc_script");
}

sub getGbEconfig()
{
    #
    # Read DB configurations of GbE and CTS,
    # look for active data sources as well as
    # for EB IPs and ports.
    #

    my ($eb_ids_href) = @_;

    my $data_sources = $temp_args_href->{'Parallel'}->{'DATA_SOURCES'};
    my $gbe_conf     = $temp_args_href->{'Parallel'}->{'GBE_CONF'};
    #my $cts_conf     = $temp_args_href->{'Parallel'}->{'CTS_CONF'};

    my %activeSources_hash;
    my $activeSources_href = \%activeSources_hash;

    &readActiveSources($data_sources, $activeSources_href);

    my @id_list;
    my $id_list_aref = \@id_list;

    #&readEBids($cts_conf, $id_list_aref);

    #- Overwrite array with EB numbers
    @id_list = (0 .. 4);
    #print Dumper $id_list_aref;

    &readEBports($gbe_conf, $activeSources_href, $id_list_aref, $eb_ids_href);
}

sub readEBids()
{
    #
    # Read EB Ids
    #

    my ($file, $id_list_aref) = @_;

    my $nnn_table = 0;
    my $val_table = 0;

    my $SPACE = "";

    my $fh = new FileHandle("$file", "r");

    while(<$fh>){

        #- Remove all comments
        $_ =~ s{                # Substitue...
                 \#             # ...a literal octothorpe
                 [^\n]*         # ...followed by any number of non-newlines
               }
               {$SPACE}gxms;    # Raplace it with a single space        

        #- Skip line if it contains only whitespaces
        next unless(/\S/);

            if(/^(\s+)?!Value\stable/){
            $val_table = 1;
            $nnn_table = 0;
            next;
        }
        elsif(/^(\s+)?!\w+/){
            $val_table = 0;
            $nnn_table = 1;
        }

        if($val_table){
            my (@vals)   = split(" ", $_);
            my @id_list1 = split("", $vals[12]);
            my @id_list2 = split("", $vals[13]);
            foreach my $id (@id_list1){
                push(@$id_list_aref, hex($id));
            }
            foreach my $id (@id_list2){
                push(@$id_list_aref, hex($id));
            }
        }
        elsif($nnn_table){
        }
    }

    $fh->close;
}

sub readEBports()
{
    #
    # Read EB IPs and ports accoring to EB Id (type) 
    # and TRB-Net addresses of active data sources.
    #

    my ($file, $activeSources_href, $id_list_aref, $ports_href) = @_;

    my $nnn_table = 0;
    my $val_table = 0;

    my $fh = new FileHandle("$file", "r");

    &isFileDefined($fh, $file);

    my %tmp;
    my $tmp_href = \%tmp; 

    my $SPACE = "";

    while(<$fh>){

        #print $_;
        #- Remove all comments
        $_ =~ s{                # Substitue...
                 \#             # ...a literal octothorpe
                 [^\n]*         # ...followed by any number of non-newlines
               }
               {$SPACE}gxms;    # Raplace it with a single space

        #- Skip line if it contains only whitespaces
        next unless(/\S/);

        #print $_;
            if(/^(\s+)?!Value\stable/){
            $val_table = 1;
            $nnn_table = 0;
            next;
        }
        elsif(/^(\s+)?!\w+/){
            $nnn_table = 1;
            $val_table = 0;
        }

        if($val_table){
            my (@vals)   = split(" ", $_);
            my $id = $vals[1];

            #if($id <0 or $id >15) {
            #  print "error: in $file there is a line with an eventbuilder number different than 0..15, the number given in the file is $id. please correct the config file.\n";
            #  exit(128);
            #}


            #- Accept only EB Ids from CTS config file
            #print "value: $_";
            next unless( any {$_ eq $id} @$id_list_aref );

            #print Dumper \@vals;
            #print "active sources: "; print Dumper $activeSources_href->{'addr_list'};
            #exit;

            my $ip   = &getIP_hex2dec($vals[6]);
            my $port = &getPort_hex2dec($vals[2]);
            my $addr = $vals[0];

            #print "got: ip: $ip, port: $port, addr: $addr\n";
            #- Accept only sources from active source list
            if( any {hex($_) == hex($addr)} @{$activeSources_href->{'addr_list'}} ){
                $tmp_href->{$id}->{'IP'} = $ip;
                push( @{$tmp_href->{$id}->{'port_list'}}, $port );
                push( @{$tmp_href->{$id}->{'addr_list'}}, $addr );
            }
        }
    }

    $fh->close;

    #print Dumper $tmp_href;

    #- Sort hash according to active data source list
    foreach my $id (keys %tmp){
        $ports_href->{$id}->{'IP'} = $tmp_href->{$id}->{'IP'};
        
        foreach my $addr (@{$activeSources_href->{'addr_list'}}){
            
            my $ind1 = first_index {$_ eq $addr} @{$tmp_href->{$id}->{'addr_list'}};
            my $ind2 = first_index {$_ eq $addr} @{$activeSources_href->{'addr_list'}};

            next if($ind1 == -1);

            push( @{$ports_href->{$id}->{'port_list'}}, $tmp_href->{$id}->{'port_list'}->[$ind1]);
            push( @{$ports_href->{$id}->{'addr_list'}}, $addr);
            push( @{$ports_href->{$id}->{'bufsize_list'}}, $activeSources_href->{'bufsize_list'}->[$ind2]);
        }
    }

    #print Dumper $ports_href;
}

sub readActiveSources()
{
    #
    # Read TRB-Net addresses of active data sources
    #

    my ($file, $activeSources_href) = @_;

    my $fh = new FileHandle("$file", "r");

    &isFileDefined($fh, $file);

    my $SPACE = "";

    while(<$fh>){
        
        #- Remove all comments
        $_ =~ s{                # Substitue...
                 \#             # ...a literal octothorpe
                 [^\n]*         # ...followed by any number of non-newlines
               }
               {$SPACE}gxms;    # Raplace it with a single space

        #- Skip line if it contains only whitespaces
        next unless(/\S/);

        my ($addr, $astat, $sys, $size)  = split(" ", $_);

        next if($astat == 0);

        push( @{$activeSources_href->{'addr_list'}}, $addr);
        push( @{$activeSources_href->{'bufsize_list'}}, &getBufSize($size)); 
    }

    $fh->close;
}

sub getBufSize()
{
    my ($bufSize) = @_;

    if(lc($bufSize) eq "low"){
        return $temp_args_href->{'Main'}->{'BUF_SIZE_LOW'};
    }
    elsif(lc($bufSize) eq "mid"){
        return $temp_args_href->{'Main'}->{'BUF_SIZE_MID'};
    }
    elsif(lc($bufSize) eq "high"){
        return $temp_args_href->{'Main'}->{'BUF_SIZE_HIGH'};
    }
    else{
        print "Cannot understand $bufSize from data_sources.db.\n";
        exit(0);
    }
}

sub getIP_hex2dec()
{
    my ($ip_hex) = @_;

    my $ip_dec;

    if( $ip_hex =~ /0x(\w{2})(\w{2})(\w{2})(\w{2})/ ){
        $ip_dec = hex($1) . "." . hex($2) . "." . hex($3) . "." . hex($4);
    }
    else{
        print "getIP_hex2dec(): cannot extract ip address because of diferent format! Exit.";
        exit(0);
    }

    return $ip_dec;
}

sub getPort_hex2dec()
{
    my ($port_hex) = @_;

    my $port_dec;

    if( $port_hex =~ /0x(\w+)/ ){
        $port_dec = hex($1);
    }
    else{
        print "getPort_hex2dec(): cannot extract port number because of diferent format! Exit.";
        exit(0);
    }

    return $port_dec;
}

sub isFileDefined()
{
    my ($fh, $name) = @_;

   if(!$fh) {
        my $txt = "\nError! Could not open file \'$name\'. Exit.\n";
        print STDERR $txt;
        print $txt;
        exit(128);
    }

    return 0;
}

sub writeArgs2file()
{
    my $fileName = $0;

    #- Replace .pl with .sh
    $fileName =~ s/\.pl/\.sh/;

    my $fh = new FileHandle(">./$fileName");
    if(!$fh) {
        my $txt = "\nError! Could not open file \"$fileName\" for output. Exit.\n";
        print STDERR $txt;
        print $txt;
        exit(128);
    }

    my $current_dir = cwd();
    my $ptogName = $0;


    #- Write to the file the script name itself
    print $fh $0;

    #- Write to the file the arguments
    foreach my $arg (@arg_list){
        print $fh " $arg";
    }
    print $fh "\n";

    $fh->close();

    system("chmod 755 ./$fileName");
}

