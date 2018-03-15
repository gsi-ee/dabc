#!/usr/bin/perl -w

use strict;
use Data::Dumper;

#- Kill daq_netmem and daq_evtbuild if remained

# my $pname = "daq_netmem";
# my @out_list = `ps -C $pname`;
# &killSoftProcs(\@out_list, $pname);
# sleep 2;
# @out_list = `ps -C $pname`;
# &killHardProcs(\@out_list, $pname);
# 
# $pname = "daq_evtbuild";
# @out_list = `ps -C $pname`;
# &killSoftProcs(\@out_list, $pname);
# sleep 2;
# @out_list = `ps -C $pname`;
# &killHardProcs(\@out_list, $pname);
##########################################

my $pname = "dabc_exe.head";
my @out_list = `ps -C $pname`;
&killKeyboardInterrupt(\@out_list, $pname);

$pname = "dabc_exe.275";
@out_list = `ps -C $pname`;
&killKeyboardInterrupt(\@out_list, $pname);

$pname = "dabc_exe";
@out_list = `ps -C $pname`;
&killKeyboardInterrupt(\@out_list, $pname);


# JAM 8-2017 this might be required to give epics time for distribution of new runid?

print "waiting 10 seconds for files to close properly...\n";
sleep 10;

# JAM 8-2017 to ensure dabc processes have gone, kill everything that still remains on the hard way: 
# for the moment, test dabc shutdown again-


$pname = "dabc_exe.head";
@out_list = `ps -C $pname`;
&killHardProcs(\@out_list, $pname);

$pname = "dabc_exe.275";
@out_list = `ps -C $pname`;
&killHardProcs(\@out_list, $pname);

$pname = "dabc_exe";
@out_list = `ps -C $pname`;
&killHardProcs(\@out_list, $pname);

print "                ...done.\n";



######################################
#&killSoftProcs(\@out_list, $pname);
# do not kill dabc twice!
#sleep 2;
# @out_list = `ps -C $pname`;
# &killHardProcs(\@out_list, $pname);
#
#&killKeyboardInterrupt(\@out_list, $pname);

#- Remove shared memory segments if remained

my $shm_dir = "/dev/shm";

opendir(DIR, $shm_dir) or die "Could not open $shm_dir: $!";

my @shm_list = grep(/\w+\.shm/, readdir(DIR));

foreach my $shmem (@shm_list){
    my $cmd = "rm $shm_dir/$shmem";
    #print "cmd: $cmd\n";
    system($cmd);
}

################### END OF MAIN ###################

sub killSoftProcs()
{
    my ($list_aref, $proc_name) = @_;

    foreach my $line (@$list_aref){
	$line =~ s/^\s+//;  # remove leading spaces
	if( $line =~ /$proc_name/ ){
	    my @items = split( /\s+/, $line );
	    my $PID = $items[0];

	    system("kill $PID");
	  print "kill $PID with signal TERM \n"

	}
    }
}

sub killHardProcs()
{
    my ($list_aref, $proc_name) = @_;

    foreach my $line (@$list_aref){

	if( $line =~ /$proc_name/ ){
	    $line =~ s/^\s+//;  # remove leading spaces
	    my @items = split( /\s+/, $line );

	    my $PID = $items[0];

	    system("kill -9 $PID");
	     print "kill $PID with signal -9 \n"

	}
    }
}

sub killKeyboardInterrupt()
{
    my ($list_aref, $proc_name) = @_;

    foreach my $line (@$list_aref){

	if( $line =~ /$proc_name/ ){
	    $line =~ s/^\s+//;  # remove leading spaces
	    my @items = split( /\s+/, $line );

	    my $PID = $items[0];

	    system("kill -2 $PID");
	    print "kill $PID with keyboard interrupt\n"
	}
    }
}

