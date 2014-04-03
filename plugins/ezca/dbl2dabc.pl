#!/usr/bin/perl -w

# dbl2dabc V0.1
# convert ioc variable list to dabc config file
# v0.1: 7-Dec-2011 Joern Adamczewski-Musch, gsi
#
# to produce input lists, invoke following commands in ioc console:
# dbl ai > ${TOP}/iocBoot/${IOC}/cbm_ai.dbl
# dbl longin > ${TOP}/iocBoot/${IOC}/cbm_longin.dbl
# these files will appear in same directory as st.cmd for ioc

use English;
use strict;
use Getopt::Long;
use FileHandle;
use File::Path;
use File::Basename;

my $dbl_double_file  =  "./cbm_ai.dbl";
my $dbl_longin_file  =  "./cbm_longin.dbl";
my $dabc_config_file = "./EpicsRead.xml";
open( OUTFILE, '>', $dabc_config_file ) or die "Could not open $dabc_config_file: $! \n";

print "Generating dabc config file $dabc_config_file ...\n";
# first provide output file with standard header:
print OUTFILE "<?xml version=\"1.0\"?>\n";
print OUTFILE "<dabc version=\"2\">\n";
print OUTFILE " <Context name=\"EpicsReadout\" host=\"epx01\" >\n";
print OUTFILE "   <Run>\n";
print OUTFILE "     <lib value=\"libDabcEzca.so\"/>\n";
print OUTFILE "     <logfile value=\"Ezca.log\"/>\n";
print OUTFILE "   </Run>\n";
print OUTFILE "   <MemoryPool name=\"Pool\">\n";
print OUTFILE "      <BufferSize value=\"65536\"/>\n";
print OUTFILE "      <NumBuffers value=\"100\"/>\n";
print OUTFILE "   </MemoryPool>\n";
print OUTFILE "  <Module name=\"ezca\" class=\"ezca::Monitor\">\n";
print OUTFILE "	 <NumOutputs value=\"1\"/>\n";
print OUTFILE "    <OutputPort name=\"Output0\" url=\"mbs://Stream:6002\"/>\n";
print OUTFILE "    <EpicsIdentifier value=\"myIOC\"/>\n";
print OUTFILE "    <EpicsSubeventId value=\"0x000000A\"/>\n";
print OUTFILE "    <EpicsFlagRec value=\"\"/>\n";
print OUTFILE "    <EpicsEventIDRec value=\"CBM:hv00:scan1\"/>\n";

#evaluate long records if existing
my $rev=open( IFILE, '<', $dbl_longin_file );
if(!$rev)
	{
		# we do not have file, just set empty number with template:
		print "Could not open $dbl_longin_file\n";
		print OUTFILE "    <EpicsLongRecs value=\"[]\"/>\n";
	}
else
	{

		my @longints= <IFILE>;
		my $numlongs = $#longints + 1;
		print "Using $numlongs longint records from file $dbl_longin_file\n";
		print OUTFILE "    <EpicsLongRecs>\n";
		foreach my $varint (@longints) {
		chomp($varint);
		my $outdata = sprintf("      <item value=\"%s\"/>",$varint);
		print OUTFILE "$outdata\n";
		} 
		print OUTFILE "    </EpicsLongRecs>\n";
	}

#evaluate double records if existing
$rev=open( DFILE, '<', $dbl_double_file ); 
if(!$rev)
	{
		# we do not have file, just set empty number with template:
		print "Could not open $dbl_double_file\n";
		print OUTFILE "    <EpicsDoubleRecs value=\"[]\"/>\n";
	}
else
	{
		my @longdubs= <DFILE>;
		my $numdoubles = $#longdubs + 1;
		print "Using $numdoubles ai records from file $dbl_double_file\n";
		print OUTFILE "    <EpicsDoubleRecs>\n";
		foreach my $vardub (@longdubs) {
		chomp($vardub);
		my $outdata = sprintf("    <item value=\"%s\"/>",$vardub);
		print OUTFILE "$outdata\n";
		} 
		print OUTFILE "    </EpicsDoubleRecs>\n";
	}

#provide footer 

print OUTFILE "    <EpicsPollingTimeout  value=\"10\"/>\n";
print OUTFILE "  </Module>\n";
print OUTFILE " </Context>\n";
print OUTFILE "</dabc>\n";

print "\tdone.\n";
print "! Do not forget to adjust <EpicsEventIDRec> and Run setup! \n";
