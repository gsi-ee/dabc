\page ezca_plugin  EPICS plugin for DABC (libDabcEzca.so)

\subpage ezca_plugin_doc

\ingroup dabc_plugins


\page ezca_plugin_doc Short description of EPICS plugin

## Compilation
EPICS plugin will be automatically compiled, when EPICS_BASE and
EPICS_EXTENSIONS variables are defined.

EPICS_BASE should contain path to top of your EPICS installation.
EPICS_EXTENSIONS to top of your EPICS extensions installation
which must contain the ezca extension.

To disable compilation of EPICS plugin, call

    [shell] cmake .. -Depics=OFF


## Usage

1.Please adjust the configuration file CaRead.xml to match the epics process variables to monitor.

Tag EpicsFlagRec specifies name of record which triggers a readout of all other desired records.
DABC will poll and request the variables whenever the flag record is not zero.

Tag EpicsEventIDRec specifies name of record which contains an arbitrary identifier number
to synchronize the epics variables with the other DAQ data later.

Tag EpicsNumLongRecs defines variable number of longword data records to request.
The Names of such records are specified with tags EpicsLongRec-0 ... EpicsLongRec-n

Tag EpicsNumDoubleRecs defines variable number of double (float) data records to request.
The Names of such records are specified with tags EpicsDoubleRec-0 ... EpicsDoubleRec-n

Tag PollingTimeout sets time in seconds for checking the EpicsFlagRec if new epics data shall be requested.

2. Startup your epics ioc with the respective process variables.

3. Start DABC data taking from epics ioc by

    dabc_exe CaRead.xml



## Go4 analysis for EPICS data
Subdirectory plugins/ezca/go4 contains simple analysis,
which could monitor data, provided by EPICS plugin.
Just configure Go4 environment (via  '. go4login') and call make in the directory

Optionally, start go4 monitoring of DABC in subfolder go4:

    go4analysis -stream hostname

(with hostname the name of the machine that runs the DABC process with MbsServerKind "Stream")
