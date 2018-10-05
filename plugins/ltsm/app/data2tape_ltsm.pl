#!/usr/bin/perl -w

use strict;
use Getopt::Long;
use Data::Dumper;
use File::stat;
use FileHandle;
use Time::Local;
use List::MoreUtils qw(any apply);
use File::Basename;
use Time::Local;
use DateTime;

my $opt_help = 0;
my $opt_startDate;
my $opt_endDate;
my @prefix_list = ();
my @file_list;
my $file_list_aref = \@file_list;
my $opt_file;
#my $opt_arch;


my $opt_arch; # jul18
my $opt_ltsmserv = "lxltsm01";
my $opt_ltsmfs   = "/lustre/hebe";

my $opt_ltsmnode = "hades";
my $opt_ltsmpass; #  = "wDhgcvFF7";
my $opt_ltsmowner = "hadesdst";


 

GetOptions ('h|help'      => \$opt_help,
	    'f|file=s'    => \$opt_file,
            's|start=s'   => \$opt_startDate,
            'e|end=s'     => \$opt_endDate,
            'p|prefix=s'  => \@prefix_list,
	    'a|arch=s'    => \$opt_arch,
	    'w|password=s' => \$opt_ltsmpass
    );

if( $opt_help ) {
    &help();
    exit(0);
}
my $ltsmpath = "/hades/raw/";

if (defined $opt_arch){
    $ltsmpath .= $opt_arch; 
} else{
    $ltsmpath .= "jul18";
}
$ltsmpath .= "/default/tsm/";


if(-1 == &checkArgs()){
     exit(0);
}

if(defined $opt_file){
    &getFileListFromFile();
}
else{
    &getFileList();
}
&archive();

exit(0);

########################### END OF MAIN ############################

sub help()
{
    print "\n";
    print << 'EOF';
date2tape_ltsm.pl
 -- adjusted for LTSM interface by JAM 5-Oct-2018
 -- original script by S.Yurevich 2010

This script archives the data if the data were not archived during
data taking automatically via LTSM.

Usage:

   Command line:  date2tape.pl
   [-h|--help]               : Show this help.
   [-s|--start <date_time>]  : Beginning of time interval.
   [-t|--end <date_time>]    : End of time interval.
   [-p|--prefix <prefix>]    : Archive only these prefixes.
   [-f|--file <path>]        : Path to hld file list.
   [-a|--arch <archive>]     : Archiving is done only with this option enabled.
   [-w|--password <pw>]      : TSM server password.

Examples:

   Archive all hld files with prefixes 'st' and 'be' from sept 4th, 2010 to the archive hadessep10raw:
      date2tape.pl -p be -p st -s 2010-09-04_00:00:00 -e 2010-09-04_23:59:59 -a hadessep10raw

   Archive all hld files from the list /home/hadaq/kgoebel.txt to archive "hadesuser/kgoebel/hld/":
      date2tape.pl -a hadesuser -d kgoebel/hld -f /home/hadaq/kgoebel.txt

EOF
}

sub checkArgs()
{
    my $retval = 0;

    unless( defined $opt_file ){
	unless( defined $opt_startDate ){
	    print "Start date is not given!\n";
	    $retval = -1;
	}
	
	unless( defined $opt_endDate ){
	    print "End date is not given!\n";
	    $retval = -1;
	}
    }
    
    if ( defined $opt_arch ){
	unless( defined $opt_ltsmpass){
	   print "TSM password is not specified for node $opt_ltsmnode at server $opt_ltsmserv!\n";
	    $retval = -1;
	}
    }
    else
    {
	$opt_ltsmpass="mysecretpw" 	unless( defined $opt_ltsmpass); # for debug printout
    }
    
    

    return $retval;
}

sub date2sec()
{
    my ($date_time) = @_;

    my $sec_epoch;

    if( $date_time =~ /(\d{4})-(\d{2})-(\d{2})_(\d{2}):(\d{2}):(\d{2})/ ){

	#- Correct to get proper format if needed
	my $year  = $1;
	my $mon   = $2 - 1;  # 0..11
	my $mday  = $3;      # 1..31
	my $hour  = $4;
	my $min   = $5;
	my $sec   = $6;

	#- Convert to Epoch seconds in a local time zone
	$sec_epoch = timelocal($sec, $min, $hour, $mday, $mon, $year);
    }
    else{
	print "Wrong format: $date_time\nExit.\n";
	exit(0);
    }

    return $sec_epoch;
}

sub getFileListFromFile()
{
    #- Get file list from $opt_file

    my @tmp_list = `cat $opt_file`;

    foreach my $file (@tmp_list){
	chomp($file);
	$file =~ s/^\s+//;  # remove leading spaces
	$file =~ s/\s+$//;  # remove trailing spaces

	push(@file_list, $file);
    }
}

sub getFileList()
{
    my $startSec = &date2sec($opt_startDate);
    my $endSec   = &date2sec($opt_endDate);

    #- Loop over disks
    foreach my $diskNr (1..22){ 
	my $path = sprintf("/data%02d/data", $diskNr);
	
	my @data = glob("$path/*.hld");

	foreach my $hldfile (@data){

	    #- File size must be above 1 MB
	    next if(stat($hldfile)->size < 1024000);

	    #- Check prefix
	    if($hldfile =~ /(\w{2})\d+\.hld/){
		my $prefix = $1;

		#- File must have a predefined prefix
		next unless( any {$_ eq $prefix} @prefix_list );
	    }
	    else{
		print "=====> Strange hld file name: $hldfile\n";
	    }

	    #- Check time interval
	    my $my_sec = stat($hldfile)->mtime;

	    next unless(stat($hldfile)->mtime > $startSec && stat($hldfile)->mtime < $endSec);

	    push(@file_list, $hldfile);
	}
    }
}

sub archive()
{
    my $nrOfFiles = scalar @file_list;

    print "Number of files to archive: $nrOfFiles\n\n";

    #- Return if nrOfFiles is zero
    return 0 unless($nrOfFiles);
    
  
    if(defined $opt_arch){
	print "The data will be written to the $opt_arch archive!\n";
	&askUser();
    }
 
    foreach my $hldfile (@file_list){

	my $dt   = DateTime->now;
	$dt->set_time_zone('local');
	my $ymd = $dt->ymd(':');
	my $hms =  $dt->hms(':');
	my $description = "Archived manually with data2tape_ltsm.pl on $ymd\_$hms";
	#my $archive = "archive";
	#$archive = $opt_arch if( defined $opt_arch);
	my $cmd  = "cat \"$hldfile\" | ltsmc --pipe -n \"$opt_ltsmnode\" ";
	$cmd .= "-f \"$opt_ltsmfs\" -p \"$opt_ltsmpass\" ";
	$cmd .= "-s \"$opt_ltsmserv\" -o \"$opt_ltsmowner\" -d \"$description\" ";
	my $reduced_file= fileparse($hldfile);
	$cmd .= " \"$opt_ltsmfs$ltsmpath$reduced_file\"";
	
	print "archiving $reduced_file with ltsmc at $hms..\n";		
	print "cmd: $cmd\n" unless ( defined $opt_arch);
	###
	system($cmd) if( defined $opt_arch);
    }
}

sub askUser()
{
    my $answer = &promptUser("Continue?", "yes/no");
    if( $answer eq "no" || $answer eq "n" ){
	print "Exit.\n";
	exit(0);
    }
    else{
	print "Continue...\n";
    }    
}

sub promptUser {

   #  two possible input arguments - $promptString, and $defaultValue 
   #  make the input arguments local variables.                        

   my ($promptString,$defaultValue) = @_;

   #  if there is a default value, use the first print statement; if  
   #  no default is provided, print the second string.                 

   if ($defaultValue) {
      print $promptString, "[", $defaultValue, "]: ";
   } else {
      print $promptString, ": ";
   }

   $| = 1;               # force a flush after our print
   my $input = <STDIN>;  # get the input from STDIN (presumably the keyboard)

   # remove the newline character from the end of the input the user gave us

   chomp($input);

   #  if we had a $default value, and the user gave us input, then   
   #  return the input; if we had a default, and they gave us no     
   #  no input, return the $defaultValue.                            
   #                                                                  
   #  if we did not have a default value, then just return whatever  
   #  the user gave us.  if they just hit the <enter> key,           
   #  the calling routine will have to deal with that.               

   if ("$defaultValue") {
      return $input ? $input : $defaultValue; # return $input if it has a value
   } else {
      return $input;
   }
}
