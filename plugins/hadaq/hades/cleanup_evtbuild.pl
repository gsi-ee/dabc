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

my $pname = "dabc_exe";
my @out_list = `ps -C $pname`;
#&killSoftProcs(\@out_list, $pname);
#sleep 2;
# @out_list = `ps -C $pname`;
# &killHardProcs(\@out_list, $pname);

&killKeyboardInterrupt(\@out_list, $pname);

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

