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
my $opt_ltsmpass;


 

GetOptions ('h|help'      => \$opt_help,
	    'f|file=s'    => \$opt_file,
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

    &getFileListFromFile();

    &delete();

exit(0);

########################### END OF MAIN ############################

sub help()
{
    print "\n";
    print << 'EOF';
deleteTestfiles_ltsm.pl
 -- implemented for LTSM interface by JAM 30-Oct-2018
 -- contains parts by S.Yurevich 2010

This script removes te*.hld data files from the archive which are taken from a file list
Note that only prefix te is allowed to be deleted here.

Usage:

   Command line:  date2tape.pl
   [-h|--help]               : Show this help.
   [-f|--file <path>]        : Path to hld file list.
   [-a|--arch <archive>]     : Archiving is done only with this option enabled.
   [-w|--password <pw>]      : TSM server password.

Examples:

   Delete all hld files  from the list /home/hadaq/oper/disks/archived_te.txt
      deleteTestfile_ltsm.pl -a jul18  -f /home/hadaq/oper/disks/archived_te.txt -w <secret>


EOF
}

sub checkArgs()
{
    my $retval = 0;

    unless( defined $opt_file ){
       die "No input files with list of te files is given!\n";
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



sub getFileListFromFile()
{
    #- Get file list from $opt_file
    my $SPACE = "";
    my @tmp_list = `cat $opt_file`;

    foreach my $file (@tmp_list){
	chomp($file);
	$file =~ s/^\s+//;  # remove leading spaces
	$file =~ s/\s+$//;  # remove trailing spaces
	 #- Remove all comments
        $file=~ s{                # Substitue...
                 \#             # ...a literal octothorpe
                 [^\n]*         # ...followed by any number of non-newlines
               }
               {$SPACE}gxms;    # Raplace it with a single space
      #- Skip line if it contains only whitespaces
        next unless($file=~/\S/);
        next if ($file=~/[\b]/); 
        # filter out the "invisible" current filenames in the log, always overwritten with backspace by archived_data_ltsm.pl -> would lead to empty filenames with prefix te!
	push(@file_list, $file);
    }
}



sub delete()
{
   
    my $nrOfFiles = scalar @file_list;

    print "Number of files to delete: $nrOfFiles\n\n";

    #- Return if nrOfFiles is zero
    return 0 unless($nrOfFiles);
    
  
    if(defined $opt_arch){
	print "The data will be deleted from the $opt_arch archive!\n";
	&askUser();
    }
  
    my $pre="";
    foreach my $hldfile (@file_list){
#    print "Line is: $hldfile .\n";
    
    #- Remove all comments
#         $hldfile=~ s{                # Substitue...
#                  \#             # ...a literal octothorpe
#                  [^\n]*         # ...followed by any number of non-newlines
#                }
#                {$SPACE}gxms;    # Raplace it with a single space
# 
#         #- Skip line if it contains only whitespaces
#         next unless($hldfile=~/\S/);
        next if($hldfile eq '  ');
    
	#my $archive = "archive";
	#$archive = $opt_arch if( defined $opt_arch);
	my $reduced_file= fileparse($hldfile);

	# here check if the prefix is really te:
	$pre= substr ($reduced_file, 0, 2);
	next unless (defined $pre);
	#chomp($pre);
#	print "found prefix " . $pre . "\n";
	
	next unless ($pre eq 'te');
	
	
	my $cmd  = "ltsmc --delete ";
	$cmd .= "-n \"$opt_ltsmnode\" -f \"$opt_ltsmfs\" -p \"$opt_ltsmpass\" ";
	$cmd .= "-s \"$opt_ltsmserv\" ";
	$cmd .= " \"$opt_ltsmfs$ltsmpath$reduced_file\"";
	
	
	print "delete $reduced_file with ltsmc..\n";		
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
