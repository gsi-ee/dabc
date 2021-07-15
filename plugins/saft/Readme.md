\page saft_plugin

## Simplified API For Timing (SAFT)plugin for DABC (libDabcSaft.so)

\subpage saft_plugin_doc

\ingroup dabc_plugins


\page saft_plugin_doc
# Short description of SAFT plugin {#saft_plugin_doc}

## Compilation
Saftlib installation can be done as described at
https://www-acc.gsi.de/wiki/Timing/TimingSystemHowConfigureEnvironment

If Saftlib is already installed on the system, dabc will try to detect it automatically and
build the saftdabc plugin from the top Makefile.
If detection fails, corresponding locations of saftlib and libgiomm can be specified at
build/Makefile.config

To disable compilation of SAFT plugin, call

    [shell] make nosaft=1



## Usage

1)Please adjust the configuration file SaftRead.xml to match the White Rabbit events to monitor.
This can be either latched pulses on the named Timing Receiver Inputs (e.g. "IO2" on EXPLODER5a),
or White Rabbit events received over the timing network.
The first case is defined by entries in tag `<SaftHardwareInputNames>`.
The second case can be specified by tags `<SaftSnoop*>` with corresponding ordered entries for
event id, mask, offset and flags.


2) Start DABC data capturing from timing receiver  by

    dabc_run SaftRead.xml

Data will be exported via mbs event structures at stream server in first example.
Can be inspected/printed with

    go4analysis -print -stream localhost:6111.


3) optional Go4 analysis for SAFT data
Subdirectory plugins/saft/go4 contains simple analysis,
which can monitor the dataas provided by saftplugin.
Just configure Go4 environment (via  '. go4login') and call make in the directory
start go4 monitoring in subfolder by:

    go4analysis -stream hostname:6111

This analysis is specialized for latched input timing events, e.g. delivered by a pulser at EXPLODER5 IO1.
Several histograms check for lost timing events by histogramming the overflow counts ("OverflowScaler"), and by missing alternation
of rising and falling edges ("LostEdges"). Additionally, the delta timestamps of subsequent timing events are histogrammed in
"DeltaT" (execution time), "DeltaT_deadline" and "DeltaT_deadline_fine" (deadline time). Note that for input latch the deadline time
denotes the arrival time of the input, not the execution time!

\author JAM(j.adamczewski@gsi.de)
\date 15-September-2016


