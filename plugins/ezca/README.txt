--------------------------------------------------------------
------- Easy Channel Access (EZCA) data input plug-in for DABC
------- version 0.9 : 8-November-2010 J.Adamczewski-Musch
--------------------------------------------------------------

Contents:
---------
-README.txt
-DABC data input plug-in (subfolders ezca, src)
-Example configuration file CaRead.xml
-optional Go4 online monitoring (subfolder go4)


Required:
---------
-DABC framework installation 
-EPICS installation
-EZCA extension for EPICS
	see http://www.aps.anl.gov/epics/extensions/ezca/index.php
-GO4 analysis framework (optional)

Installation:
-------------
1) Set environment variable DABCSYS to top of DABC installation

2) Set environment variable EPICS_BASE to top of your EPICS installation.
	alternatively, edit the Makefile and change variable EPICS_BASE

3) Set environment variable EPICS_EXTENSIONS to top of your EPICS extensions installation
	which must contain the ezca extension.
	Alternatively, edit the Makefile and change variable EPICS_EXTENSIONS accordingly.

4) Build the plug-in library with "make" in top ezcaplugin directory.	

5) Optionally, build the go4 monitoring library in subfolder go4:

- Set environment for Go4 framework ($GO4SYS, $PATH, $LD_LIBRAR_PATH) as done with ". go4login"
- "cd ezcaplugin/go4"
- "make"


Usage:
------
1)Please adjust the configuration file CaRead.xml to match the epics process variables to monitor.

Tag EpicsFlagRec specifies name of record which triggers a readout of all other desired records.
DABC will poll and request the variables whenever the flag record is not zero.

Tag EpicsEventIDRec specifies name of record which contains an arbitrary identifier number
to synchronize the epics variables with the other DAQ data later.

Tag EpicsNumLongRecs defines variable number of longword data records to request.
The Names of such records are specified with tags EpicsLongRec-0 ... EpicsLongRec-n

Tag EpicsNumDoubleRecs defines variable number of double (float) data records to request.
The Names of such records are specified with tags EpicsDoubleRec-0 ... EpicsDoubleRec-n

Tag PollingTimeout sets time in seconds for checking the EpicsFlagRec if new epics data shall be requested.

Tag MbsServerKind defines "Stream" or "Event" server output to connect go4 online monitor. If blank, no
data server is opened.

Tag MbsFileName defines a filename for the *.lmd data output of the test readout. Note that this currently
only contains the EPICS data; to combine with other DAQ readout, the ReadoutApplication has to be modified!

2) Startup your epics ioc with the respective process variables.

3) Start DABC data taking from epics ioc by

"dabc_run CaRead.xml"

4) optionally, start go4 monitoring of DABC in subfolder go4:
"go4analysis -stream hostname"
(with hostname the name of the machine that runs the DABC process with MbsServerKind "Stream")



-JAM--------9-Nov-2010--------------



	