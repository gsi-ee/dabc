#!/usr/bin/perl -w

use strict;
use Getopt::Long;
use Data::Dumper;
use File::stat;
use Time::Local;
use FileHandle;
use File::Basename;
use Time::Local;
use DateTime;

my $opt_verbose =0;
my $opt_help = 0;
my $opt_prefix = "";
my $opt_startDate;
my $opt_endDate;
my $opt_arch = "jul18";
my $opt_output = "tape";  # tape|disk
my $opt_rm;
my $opt_ltsmserv = "lxltsm01";
my $opt_ltsmfs   = "/lustre/hebe";

my $opt_ltsmnode = "hades";
my $opt_ltsmpass;
my $opt_daysago;
my $opt_crc;

GetOptions ('h|help'      => \$opt_help,
            's|start=s'   => \$opt_startDate,
            'e|end=s'     => \$opt_endDate,
            'p|prefix=s'  => \$opt_prefix,
            'a|arch=s'    => \$opt_arch,
	    'o|out=s'     => \$opt_output,
	    'r|rm=s'      => \$opt_rm,
	    'd|daysago=s' => \$opt_daysago,
	    'c|crc'       => \$opt_crc,
	    'w|password=s' => \$opt_ltsmpass
    );

if( $opt_help ) {
    &help();
    exit(0);
}

unless (defined $opt_ltsmpass)
{
    die "Please specify password for tsm node $opt_ltsmnode at server $opt_ltsmserv with option -w \n";
}


my $opt_ltsmpath = "/hades/raw/" . $opt_arch ."/default/tsm/";

my %gstore_hash;
my $gstore_href = \%gstore_hash;
my %eb_hash;
my $eb_href = \%eb_hash;




my $startSec = &date2sec($opt_startDate);
my $endSec   = &date2sec($opt_endDate);
my $date_time_days_ago;

if(defined $opt_daysago){
   $date_time_days_ago =`date --date=\"$opt_daysago  days ago" +\"%Y:%m:%d:%H:%M:%S\"`;
   chop($date_time_days_ago); # remove \n from the date result
   print "date ago: $date_time_days_ago \n" if $opt_verbose;

    
   $startSec =&date2sec( $date_time_days_ago);
   print "start second: $startSec \n" if $opt_verbose; 
   my $dt   = DateTime->now;
   $dt->set_time_zone('local');
   my $ymd = $dt->ymd(':');
   my $hms =  $dt->hms(':');
   print "current date: $ymd:$hms \n" if $opt_verbose;
   $endSec =&date2sec("$ymd:$hms");
   print "end second: $endSec \n" if $opt_verbose;
}

#die "end of test!";

if(defined $opt_rm){
    &rm_files();
}
else{
    &read_hld_tape();
    &read_hld_disk();
    &cmp_files();
}

exit(0);

########################### END OF MAIN ############################

sub help()
{
    print "\n";
    print << 'EOF';
archived_data_ltsm.pl
 -- adjusted for LTSM interface by JAM 5-Oct-2018
 -- original script by S.Yurevich 2010

This script prints two hld file lists:
1. The list of files which were archived already and can be deleted.
2. The list of files which were not archived yet.

Usage:

   Command line:  archived_data_ltsm.pl
   [-h|--help]                 : Show this help.
   [-s|--start <date_time>]    : Beginning of time interval.
   [-t|--end <date_time>]      : End of time interval.
   [-d|--daysago <nrdays>]     : alternative time interval:
                                    'number of days ago' until 'now'.
                                    will overrule options-s and -t 
   [-p|--prefix <prefix>]      : Look only for these prefixes.
   [-a|--arch <archive>]       : Name of archive.
   [-w|--password <passwd>]    : TSM password for server.
   [-o|--out <tape|disk|all>]  : Print output format:
                                    tape : files on tape|cache
                                    disk : files on EB disks only
                                    all  : all files 
   [-r|--rm <file>]            : File path with hld files to be removed 
                                 from the disks of EB.

Examples:

   Check all hld files on tape from archive jul18 with prefix 'be' in the last 14 days:
   archived_data_ltsm.pl -a jul18 -p be -d 14 -w <mysecret> -o tape

EOF
}

sub date2sec()
{
    my ($date_time) = @_;

    my $sec_epoch;

    return $sec_epoch unless( defined $date_time );
    my $year;
    my $mon;  
    my $mday; 
    my $hour;
    my $min;
    my $sec;  
    if( $date_time =~ /(\d{4})-(\d{2})-(\d{2})_(\d{2}):(\d{2}):(\d{2})/ ){
	# JAM2018 we keep legacy format if used with commandline options
	#- Correct to get proper format if needed
	$year  = $1;
	$mon   = $2 - 1;  # 0..11
	$mday  = $3;      # 1..31
	$hour  = $4;
	$min   = $5;
	$sec   = $6;
    }
    elsif ( $date_time =~ /(\d{4}):(\d{2}):(\d{2}):(\d{2}):(\d{2}):(\d{2})/ )
    {
	# also process new format as used by ltsmc:
		#- Correct to get proper format if needed
	$year  = $1;
	$mon   = $2 - 1;  # 0..11
	$mday  = $3;      # 1..31
	$hour  = $4;
	$min   = $5;
	$sec   = $6;
    }
	
    else{
	print "Wrong format: $date_time\nExit.\n";
	exit(0);
    }
    	#- Convert to Epoch seconds in a local time zone
	$sec_epoch = timelocal($sec, $min, $hour, $mday, $mon, $year);

    return $sec_epoch;
}

sub read_hld_tape()
{
    my $ltsm_cmd = "ltsmc --query --sort=ascending --verbose=message ";
    $ltsm_cmd .=  "-f '". $opt_ltsmfs ."' -n '". $opt_ltsmnode ."' -p '". $opt_ltsmpass . "' -s '". $opt_ltsmserv . "' ";
    if(defined $date_time_days_ago){
	$ltsm_cmd .= " --datelow " . $date_time_days_ago ." ";
    }
    $ltsm_cmd .=  " \"" . $opt_ltsmfs . $opt_ltsmpath  . $opt_prefix . "*\"";
    
    print "\n $ltsm_cmd \n" if $opt_verbose;
    print "Checking tape files... \n";
    my @ltsm_list = `$ltsm_cmd`;   
    foreach my $line (@ltsm_list){
	my ($n1, $date, $time, $status, $size, $fs, $hl, $ll, $crc) = split(/\s+/, $line);
	# get rid of comma after filesize:
	chop($size);
	# get rid of leading titles at filename parts:
	my $colon = index($fs, ":") + 1;
        $fs = substr($fs, $colon);
	$colon = index($hl, ":") + 1;
	$hl = substr($hl, $colon);
	$colon = index($ll, ":") + 1;
	$ll = substr($ll, $colon);
	my $name = substr($ll,1);
	print $name;
	print ("\b" x length($name));

	if($name =~ /$opt_prefix\d+\.hld/){
	    $gstore_href->{$name}->{'size'} = $size;
	    if (defined ($opt_crc)) {
		# format crc info:
		$colon = index($crc,":") +1;
		$crc = substr($crc, $colon);
		#print "ltsm crc = $crc \n";    
		$gstore_href->{$name}->{'crc'} = $crc;
	    }
	    else
	    {
	    	$gstore_href->{$name}->{'crc'} = "0x1"; # dummy for comp
	    }
	}
    }
    print "\n";
}

sub read_hld_disk()
{
    my $eb_cmd = "ls -ltr /data*/data/" . $opt_prefix . "*.hld";
    #print "cmd: $eb_cmd\n";

    print "Checking disk files... \n";
    my @eb_list = `$eb_cmd`;
    foreach my $line (@eb_list){
	my ($mode, $n1, $user, $group, $size, $day, $month, $time, $path) = split(/\s+/, $line);

	#- Check time interval for the files on disks
	if( defined $startSec && defined $endSec ){
	    next unless(stat($path)->mtime > $startSec && stat($path)->mtime < $endSec);
	}

	my ($name) = fileparse($path);

	if($name =~ /$opt_prefix\d+\.hld/){
	    print $name;
	    print ("\b" x length($name));
	    $eb_href->{$name}->{'size'} = $size;
	    $eb_href->{$name}->{'path'} = $path;
	    if (defined ($opt_crc)) {
		# evaluate crc of diskfile using ltsmc
		my $crcinfo =   `ltsmc --checksum $path`;
		#print "$crcinfo \n";
		my ($cheader, $crchex, $crcdec, $fheader, $fpath) = split(/\s+/, $crcinfo);
		#print "crc =$crchex \n";
		$eb_href->{$name}->{'crc'} = $crchex;
	    }
	    else
	    {
	    	$eb_href->{$name}->{'crc'} = "0x1"; # dummy for comp
	    }
	}
    }
    	print "\n";
}

sub cmp_files()
{
    my @tape_list;            # file on tape 
    my @tape_diffsize_list;   # on tape but different size
    my @tape_diffcrc_list;   # on tape but different crc
#    my @cach_list;            # file in cache
#    my @cach_diffsize_list;   # in cache but different size

#    my %other_hash;           # file with other status
#    my $other_href = \%other_hash;
#    my %other_diffsize_hash;  # but different size
#    my $other_diffsize_href = \%other_diffsize_hash;

    my @disk_list;            # file is only on EB disks

    foreach my $eb_file (sort keys %$eb_href){
	my $eb_size = $eb_href->{$eb_file}->{'size'};
	my $eb_path = $eb_href->{$eb_file}->{'path'};
	my $eb_crc  = $eb_href->{$eb_file}->{'crc'};
	
	#- Look only at the files above 1kByte
	next if($eb_size < 1000);

	if( defined $gstore_href->{$eb_file} ){
	    my $gstore_size   = $gstore_href->{$eb_file}->{'size'};
	    my $gstore_crc    = $gstore_href->{$eb_file}->{'crc'};


	    if($gstore_size == $eb_size){
		if(hex ($gstore_crc) ==  hex ($eb_crc)){
		    push(@tape_list, $eb_path);
		}
		else
		{
		    push(@tape_diffcrc_list, $eb_path);
		}
		
	    }
	    else{
		    push(@tape_diffsize_list, $eb_path);
	    }
	}
	else{
	    #- If the file is not on tape
	    push(@disk_list, $eb_path);
	}
    }

    #- Print all the lists
    if($opt_output eq "all" || $opt_output eq "tape"){
	my $file2rm = "/tmp/Files_on_TAPE_can_be_removed.txt";
	print "Files on TAPE ($file2rm):\n";
	my $fh = new FileHandle(">$file2rm");
	if(!$fh) {
	    my $txt = "\nError! Could not open file \"$file2rm\" for output. Exit.\n";
	    print STDERR $txt;                                                        
	    exit(128);                                                            
	}
	foreach my $file (@tape_list){
	    print "$file\n";
	    print $fh "$file\n";
	}
	$fh->close();
	print "\n\n";
    
	print "Files on TAPE have different size:\n";
	foreach my $file (@tape_diffsize_list){
	    print "$file\n";
	}
	print "\n\n";
	if (defined ($opt_crc)) {
	    print "Files on TAPE have different crc:\n";
	    foreach my $file (@tape_diffcrc_list){
	    print "$file\n";
	    }
	}
	print "\n\n";    
    }

    if($opt_output eq "all" || $opt_output eq "disk"){
	print "Files on EB disks only:\n";
	foreach my $file (@disk_list){
	    print "$file\n";
	}
	print "\n\n";
    }
}

sub rm_files()
{
    print "Remove all hld files from $opt_rm...\n";
    &askUser();

    my $fh = new FileHandle("$opt_rm", "r");

    &isItDefined($fh, $opt_rm);

    while(<$fh>){
	chomp($_);
	my $cmd = "rm $_";
	print "cmd: $cmd\n";
	system($cmd);
    }

    $fh->close();
}

sub isItDefined()
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
