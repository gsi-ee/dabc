# Software collection for TRB3 {#hadaq_trb3_package}  

## Introduction

This is short instruction how dabc/go4/stream frameworks can be installed and
used for data taking and online/offline analysis of [TRB3](http://trb.gsi.de) data.

There are two main parts of software:
  1. DABC which is required to take data from TRB3 and store to hld or lmd files
  2. ROOT/Go4/stream frameworks to analyse online/offline TRB3/TDC data

The easiest way to install all necessary software components is use repository
   https://subversion.gsi.de/dabc/trb3


# Installation of all components in once {#trb3_install}

This method describes how DABC, ROOT, Go4 and stream analysis can be installed
with minimal efforts.

### Requirements

Following packages should be installed:
* libqt4-devel
* xorg-devel
* g++

[Here is full list of prerequisites for ROOT](http://root.cern.ch/drupal/content/build-prerequisites) 

It is recommended to use bash (at least, during compilation)


### Reuse existing ROOT installation

Most of the time is consumed by ROOT compilation, therefore if ROOT
already installed on your machine, it can be reused. Just configure
ROOTSYS, PATH and LD_LIBRARY_PATH variables before starting.
For instance, call thisroot.sh script:

    [shell] . your_root_path/bin/thisroot.sh
    
Be aware that at least ROOT 5-34-32 version should be used and
compiled with '--enable-http' flag.      


### Compilation

To checkout and compile all components, just do:

    [shell] svn co https://subversion.gsi.de/dabc/trb3 trb3
    [shell] cd trb3
    [shell] make -j4

During compilation makelog.txt file will be created in each sub-directory.
In case of any compilation problem please send me (S.Linev(at)gsi.de)
error message from that file.


### Before using

There is login script 'trb3login', which must be called before software can be used

    [shell] . your_trb3_path/trb3login

It set all shell variables, which are required for DAQ and analysis


### Update from repository

To obtain newest version from repository do:

    [shell] cd your_trb3_path
    [shell] make -j4 update



## Installation of packages separately

Use this methods only if installation of all packages at once did not work or
one need only some specific package to install.

### Installation of DABC

If only DAQ functionality is required, than one need to install DABC only.
It is much faster and easier. Just do:

    [shell] svn co https://subversion.gsi.de/dabc/trunk dabc
    [shell] cd dabc; make nodim=1 -j4
    [shell] . ./dabclogin

### Installation of ROOT

See http://root.cern.ch for information, compile at least 5-34-32 version with `--enable-http` flag.

### Installation of Go4

Main information on http://go4.gsi.de.
To install from repository first initialize ROOT variables, than do:

    [shell] svn co https://subversion.gsi.de/go4/trunk go4
    [shell] cd go4; make -j4
    [shell] . ./go4login

### Installation of stream

First compile and configure go4. Then:

    [shell] svn co https://subversion.gsi.de/go4/app/stream stream
    [shell] cd stream; make -j4
    [shell] . ./streamlogin



# Running of DAQ {#trb3_daq}

To run DAQ, only DABC installation is required.

Example configuration file can be found in [$DABCSYS/plugins/hadaq/app/EventBuilder.xml](https://subversion.gsi.de/dabc/trunk/plugins/hadaq/app/EventBuilder.xml).
Copy it in any suitable place and modify for your needs.

Main configuration parameters:

### Memory pool

Defines number and size of buffers, used in application. Normally must remain as is.
Should be increased if queue sizes of input/output ports are increased

### Combiner module

It is central functional module of the DAQ application.
It could have arbitrary number of inputs, defined by NumInputs parameter.
Each input corresponds to separate TRB3 board which should be readout.
For each input only correct UDP port number should be specified like:

     <InputPort name="Input0" url="hadaq://host:10101"/>

Here only port number 10101 is relevant, all other parameters could remain as is.
Transport parameters typically speicfied in extra xml line for all ports together:

     <InputPort name="Input*" queue="10" urlopt="udpbuf=200000&mtu=64512&flush=0.1&observer=false&maxloop=50" resort="false"/>

Following URL parameters can be used for UDP transport:

| Parameter | Description |
| --------: | :----------- |
|   udpbuf  |  size of socket buffer for receiving UDP packets (default 200000)   |
|   mtu     |  Maximial Transport Unit (MTU) for UDP packet (default 64512) |
|   flush   |  flush time in seconds, how fast data will be delivered to combiner (default 1 sec) |
|  observer |  when true, generates information for HADES control system (default false) |
|  maxloop  |  how many single UDP packets can be read in single loop (default 100), could be reduced for fair thread resource sharing |
|    reduce |  reduce factor for output buffer size, may be configured together with TDC calibration option where more data could be produced, default 1 |
|       tdc |  array of TDC IDs like [0x1001,0x1002]. Activates TDC calibration |
|       trb |  value of TRB ID, to verify when data used for TDC calibration |
|       hub |  value of HUB ID(s), to correctly unpack data for TDC calibration |
|      trig |  trigger type used for calibration (default all or 0xFFFFF), can be 0xD |
|    resort |  when specified, resorting of packets order done with trigger number order |
| upd_queue |  buffers queue size, used by UDP transport (use together with *tdc* or *resort* parameter) | 

If parameter (like resort) should be specified only for particular port, one could write:

       <InputPort name="Input2" url="hadaq://host:10101" urlopt2="resort&udp_queue=20"/>
 
 Or to activate TDC calibration
 
       <InputPort name="Input3" url="hadaq://host:10101" urlopt2="tdc=[0xC001,0xC002]&trb=0x8010"/>

Events, produced by combiner module, can be stored in hld file or (and) delivered
via online server to online analysis.


### Write HLD files

To write HLD files, one should specify following parameters in combiner module:

     <NumOutputs value="2"/>

     <OutputPort name="Output1" url="hld://dabc.hld?maxsize=30"/>

Typically second output port (name Output1) used for HLD file storage, but several output files could be opened in parallel. `maxsize` parameter defines maximum size (in MB) of file, which than will be closed and new file will be started.

In case of any I/O error file is closed, but DAQ continues to run. One could change such default behavior. For instance, application will be immediately stopped if `onerror="exit"` property specified:

     <OutputPort name="Output1" url="hld://dabc.hld?maxsize=30" onerror="exit"/>
     
Or one could try to reestablish file transport, providing following parameters:

     <OutputPort name="Output1" url="hld://dabc.hld?maxsize=30" reconnect="3" onerror="exit"/>
     
Here `reconnect="3"` means that transport will be try to reconnected after 3 seconds pause.
If 10 attempts fail, application will exit as specified with `onerror` parameter. One could specify
number of attempts with `numreconn="5"` parameter. While reconnecting, buffers will be skipped.        

Different approach is - keep transport running in any case, just retrying to open new file with some time period. Like:

    <OutputPort name="Output1" url="hld://dabc.hld?maxsize=2000" retry="5" blocking="never" thread="FileThread"/>

With such configuration file transport after error will try to start writing new file after 5 second wait time. Parameter `blocking="never"` says DABC, that transport should not block event building. 
If file writing hangs (or too slow), buffers could be skipped and not block main building process. Special thread is assigned, while write operation on full disk can hang for many seconds, blocking other transports running by default in the same thread. Such configuration good to produce files for debugging purposes - if possible such file is written, if not - this not disturb main DAQ process.   

    

### Configure online server
 
First output of combiner module used for online server.
It is MBS stream server, which simply adds MBS-specific header to
each HLD events. Configuration for online server looks like:

    <OutputPort name="Output0" url="mbs://Stream:6002?iter=hadaq_iter&subid=0x1f"/>

For instance, online server can be used to printout raw data with `hldprint` command:

    [shell] hldprint localhost:6002

Very often default port for online server [6002] used by VNC. Select any other port, use it in **hldprint** or **go4analysis** to connect with the server.
 


### Running DABC

Once configuration file is adjusted, one should call:

     [shell] dabc_exe EventBuilder.xml

Execution can always be regularly stopped by Ctrl-C.
All opened files will be closed normally.


### Usage of web-server ###

One able to observe and control running DAQ application via web browser.
After DAQ is started, one could open in web browser address like 
http://localhost:8090. Port number 8090 can be changed in configuration
of _HttpServer_.

In browser one should be able to see hierarchy with "EventBuilder/Combiner" folder
for parameters and commands of main combiner module.

One of the reason for web-server usage - possibility to interactively start/stop file writings.
For this two commands can be used: `StartHldFile` for starting file and `StopHldFile` for stopping.   


## Running analysis ## {#trb3_stream_go4}

Analysis code is provided with [stream framework](https://subversion.gsi.de/go4/app/stream).
It is dedicated for synchronization and
processing of different kinds of time-stamped data streams. Classes,
relevant for TRB3/FPGA-TDC processing located in 
[$STREAMSYS/include/hadaq](https://subversion.gsi.de/go4/app/stream/include/hadaq/) and 
[$STREAMSYS/framework/hadaq](https://subversion.gsi.de/go4/app/stream/framework/hadaq/) directories. 
 
In principle, in most cases it is not required to change these classes -
all user-specific configurations provided in ROOT script, which can be found in 
[$STREAMSYS/applications/trb3tdc/](https://subversion.gsi.de/go4/app/stream/applications/trb3tdc/) directory. It shows how to process data from several TDCs. Please read comments in scripts for more details. One can always copy such script to any other location and modify it to specific needs.


### Running in batch

To run analysis in batch (offline), start from directory where _first.C_ script is
situated:

    [shell]  go4analysis -user file_0000.hld

After analysis finished, filled histograms will be saved in Go4AutoSave.root file and
can be viewed in ROOT or in Go4 browser. Just type:

    [shell] go4 Go4AutoSave.root

There are many parameters of go4analysis executable (run go4analysis -help).
For instance, one can run only specified number of events or change output file name:

    [shell] go4analysis -user file_0000.hld -number 100000 -asf new_name.root


### Running analysis online

First of all, online server should be configured in DABC. In any moment one
could start analysis from batch, connecting to DABC server with command:

    [shell] go4analysis -stream dabc_host_name

With Ctrl-C one can always stop execution and check histograms in auto-save file.

But more convenient way is to run analysis from the gui to be able monitor
all histogram in live mode. For that one need:
1. start go4 gui (type go4) from directory with _first.C_ macro
2. Select "Launch analysis" menu command in go4
3. set "Dir" parameter to "." (current directory)
4. keep empty library name file of analysis code
    (library will be located automatically by go4 itself)
5. when analysis configuration window appears,
   select "MBS stream server" and host name of DABC
   (can be localhost if DABC runs on same machine)
6. press "Submit and start" button

Via analysis browser one can display and monitor any histogram.
For more details about go4 see introduction on http://go4.gsi.de.



## Running analysis with DABC ## {#trb3_stream_dabc}

Core functionality of stream framework written without ROOT usage and
can be run with different engines. Such run engine is now provided in DABC. 
Main difference between Go4/ROOT and DABC engines - with DABC special histogram
format is used, which makes code ~10-30% faster. Histograms, filled in DABC processes,
can be displayed with normal ROOT graphics in web browser or in Go4 GUI. 
Such histograms can be stored in normal ROOT files as TH1/TH2 objects.   
 

### Batch job with multiple threads

DABC provides possibility to run code in parallel in several threads, merging produced histograms
at the end and storing them in ROOT file. Existing _first.C_ and _second.C_ files can be used as it is, only ROOT-specific parts should be removed (if exists). 

To run analysis, one requires configuration file like 
[$DABCSYS/plugins/stream/app/stream.xml](https://subversion.gsi.de/dabc/trunk/plugins/stream/app/stream.xml). Just copy it in directory where scripts are and run with the command:

    dabc_exe stream.xml file="pilas_1517816245*.hld" asf=test.root parallel=4
    
Here one specifies input HLD **file(s)** (one could use wildcard symbol), auto-save ROOT file **asf** 
where histograms will be stored and number of **parallel** threads used for analysis (default 0). 
During analysis run histogram content can be monitored via http channel, using web browser or Go4 GUI.

With single process one achieve ~10-30% gain compare with ROOT histograms filling. If running parallel  on 15 cores (on lxhadeb06 machine), performance increased on 800% compare with single-thread analysis.    
 

### Online analysis in DAQ task

One also could run analysis in the same process where event builder is running.
Analysis will process as much events as possible and produce histograms, which could be monitored via web browser.

Main benefit of such approach - one do not require extra process running,
quality monitoring always available via http channel. Example configuration
file can be found in [$DABCSYS/plugins/hadaq/app/EventBuilderStream.xml](https://subversion.gsi.de/dabc/trunk/plugins/hadaq/app/EventBuilderStream.xml).

Scripts _first.C_ and (optional) _second.C_ should be copied into directory where DABC will be started.
When DAQ is running, one could always open web browser with address `http://localhost:8090` or
directly `http://localhost:8090/EventBuilder/Analysis/`. 

Interested histograms can be shown directly, opening address like:

    http://localhost:8090/EventBuilder/Analysis/HLD/HLD_EvSize/draw.htm 

One also could produce 1-D histogram statistic, submitting requests like:

    wget http://localhost:8090/EventBuilder/Analysis/HLD/HLD_EvSize/cmd.json?command=GetEntries -O entries.txt
    wget http://localhost:8090/EventBuilder/Analysis/HLD/HLD_EvSize/cmd.json?command=GetMean -O mean.txt
    wget http://localhost:8090/EventBuilder/Analysis/HLD/HLD_EvSize/cmd.json?command=GetRMS -O rms.txt

   


# Running hldprint {#trb3_hldprint}

hldprint is small utility to printout HLD data from different sources: 
local hld files, remote hld files and running dabc application.
It also supports printout of TDC messages.
For instance, printing of messages from TDC with mask 0xC003 can be done with command:

    [shell] hldprint file_0000.hld -tdc 0xc003 -hub 0x9000 -num 1
 
Result is:

~~~~~~~~~~~~~~~~
*** Event #0x10b7b7 fullid=0x2001 runid=0x0bc01162 size 424 ***
   *** Subevent size 388 decoding 0x020011 id 0x8000 trig 0x9578eaf0 swapped align 4 ***
      *** Subsubevent size  19 id 0xc000 full 0013c000
         [ 1] 21660000  tdc header
         [ 2] 6f68537d  epoch 258495357 tm 5587568640.000 ns
         [ 3] 80118d18  hit ch: 0 isrising:1 tc:0x518 tf:0x118 tm:5587575162.766 ns
         [ 4] 6f68537c  epoch 258495356 tm 5587558400.000 ns
         [ 5] 808e0fb6  hit ch: 2 isrising:1 tc:0x7b6 tf:0x0e0 tm:5587568272.170 ns
         [ 6] 6f68537c  epoch 258495356 tm 5587558400.000 ns
         [ 7] 8104efb5  hit ch: 4 isrising:1 tc:0x7b5 tf:0x04e tm:5587568265.617 ns
         [ 8] 6f68537c  epoch 258495356 tm 5587558400.000 ns
         [ 9] 81848fb5  hit ch: 6 isrising:1 tc:0x7b5 tf:0x048 tm:5587568265.553 ns
         [10] 6f68537c  epoch 258495356 tm 5587558400.000 ns
         [11] 82063fb5  hit ch: 8 isrising:1 tc:0x7b5 tf:0x063 tm:5587568265.840 ns
         [12] 6f68537c  epoch 258495356 tm 5587558400.000 ns
         [13] 8283afb5  hit ch:10 isrising:1 tc:0x7b5 tf:0x03a tm:5587568265.404 ns
         [14] 6f68537c  epoch 258495356 tm 5587558400.000 ns
         [15] 830befb5  hit ch:12 isrising:1 tc:0x7b5 tf:0x0be tm:5587568266.809 ns
         [16] 6f68537c  epoch 258495356 tm 5587558400.000 ns
         [17] 83989fb6  hit ch:14 isrising:1 tc:0x7b6 tf:0x189 tm:5587568273.968 ns
         [18] 6f68537c  epoch 258495356 tm 5587558400.000 ns
         [19] 8415cfb6  hit ch:16 isrising:1 tc:0x7b6 tf:0x15c tm:5587568273.489 ns
      *** Subsubevent size   3 id 0xc001 full 0003c001
         [21] 21660000  tdc header
         [22] 6f68537d  epoch 258495357 tm 5587568640.000 ns
         [23] 8007dd18  hit ch: 0 isrising:1 tc:0x518 tf:0x07d tm:5587575161.117 ns
      *** Subsubevent size  20 id 0x9000 full 00149000
      *** Subsubevent size  19 id 0xc003 full 0013c003
         [26] 21660000  tdc header
         [27] 6f68537d  epoch 258495357 tm 5587568640.000 ns
         [28] 80151cce  hit ch: 0 isrising:1 tc:0x4ce tf:0x151 tm:5587574793.372 ns
         [29] 6f68537c  epoch 258495356 tm 5587558400.000 ns
         [30] 8083cf6b  hit ch: 2 isrising:1 tc:0x76b tf:0x03c tm:5587567895.426 ns
         [31] 6f68537c  epoch 258495356 tm 5587558400.000 ns
         [32] 8106ef6b  hit ch: 4 isrising:1 tc:0x76b tf:0x06e tm:5587567895.957 ns
         [33] 6f68537c  epoch 258495356 tm 5587558400.000 ns
         [34] 81840f6b  hit ch: 6 isrising:1 tc:0x76b tf:0x040 tm:5587567895.468 ns
         [35] 6f68537c  epoch 258495356 tm 5587558400.000 ns
         [36] 82033f6b  hit ch: 8 isrising:1 tc:0x76b tf:0x033 tm:5587567895.330 ns
         [37] 6f68537c  epoch 258495356 tm 5587558400.000 ns
         [38] 8289bf6b  hit ch:10 isrising:1 tc:0x76b tf:0x09b tm:5587567896.436 ns
         [39] 6f68537c  epoch 258495356 tm 5587558400.000 ns
         [40] 83066f6b  hit ch:12 isrising:1 tc:0x76b tf:0x066 tm:5587567895.872 ns
         [41] 6f68537c  epoch 258495356 tm 5587558400.000 ns
         [42] 8394af6b  hit ch:14 isrising:1 tc:0x76b tf:0x14a tm:5587567898.298 ns
         [43] 6f68537c  epoch 258495356 tm 5587558400.000 ns
         [44] 8414bf6b  hit ch:16 isrising:1 tc:0x76b tf:0x14b tm:5587567898.309 ns
      *** Subsubevent size  29 id 0xc002 full 001dc002
         [46] 21660000  tdc header
         [47] 6f68537d  epoch 258495357 tm 5587568640.000 ns
         [48] 80119d19  hit ch: 0 isrising:1 tc:0x519 tf:0x119 tm:5587575167.777 ns
         [49] 6f68537d  epoch 258495357 tm 5587568640.000 ns
         [50] 80c7b840  hit ch: 3 isrising:1 tc:0x040 tf:0x07b tm:5587568961.096 ns
         [51] 80d26845  hit ch: 3 isrising:1 tc:0x045 tf:0x126 tm:5587568987.915 ns
         [52] 80d0d8d1  hit ch: 3 isrising:1 tc:0x0d1 tf:0x10d tm:5587569687.649 ns
         [53] 80dc58fc  hit ch: 3 isrising:1 tc:0x0fc tf:0x1c5 tm:5587569904.606 ns
         [54] 80d06a23  hit ch: 3 isrising:1 tc:0x223 tf:0x106 tm:5587571377.574 ns
         [55] 80c2ea81  hit ch: 3 isrising:1 tc:0x281 tf:0x02e tm:5587571845.277 ns
         [56] 80c31ae2  hit ch: 3 isrising:1 tc:0x2e2 tf:0x031 tm:5587572330.309 ns
         [57] 80c62b42  hit ch: 3 isrising:1 tc:0x342 tf:0x062 tm:5587572810.830 ns
         [58] 80c72b77  hit ch: 3 isrising:1 tc:0x377 tf:0x072 tm:5587573076.000 ns
         [59] 80d30ba6  hit ch: 3 isrising:1 tc:0x3a6 tf:0x130 tm:5587573313.021 ns
         [60] 80d06bd3  hit ch: 3 isrising:1 tc:0x3d3 tf:0x106 tm:5587573537.574 ns
         [61] 80d3bcbc  hit ch: 3 isrising:1 tc:0x4bc tf:0x13b tm:5587574703.138 ns
         [62] 6f68537d  epoch 258495357 tm 5587568640.000 ns
         [63] 810ba840  hit ch: 4 isrising:1 tc:0x040 tf:0x0ba tm:5587568961.766 ns
         [64] 8113c845  hit ch: 4 isrising:1 tc:0x045 tf:0x13c tm:5587568988.149 ns
         [65] 811288d1  hit ch: 4 isrising:1 tc:0x0d1 tf:0x128 tm:5587569687.936 ns
         [66] 810498fb  hit ch: 4 isrising:1 tc:0x0fb tf:0x049 tm:5587569895.564 ns
         [67] 8113da23  hit ch: 4 isrising:1 tc:0x223 tf:0x13d tm:5587571378.160 ns
         [68] 81066a81  hit ch: 4 isrising:1 tc:0x281 tf:0x066 tm:5587571845.872 ns
         [69] 8119fae3  hit ch: 4 isrising:1 tc:0x2e3 tf:0x19f tm:5587572339.202 ns
         [70] 81084b42  hit ch: 4 isrising:1 tc:0x342 tf:0x084 tm:5587572811.191 ns
         [71] 81095b77  hit ch: 4 isrising:1 tc:0x377 tf:0x095 tm:5587573076.372 ns
         [72] 8115fba6  hit ch: 4 isrising:1 tc:0x3a6 tf:0x15f tm:5587573313.521 ns
         [73] 8112abd3  hit ch: 4 isrising:1 tc:0x3d3 tf:0x12a tm:5587573537.957 ns
         [74] 8116fcbc  hit ch: 4 isrising:1 tc:0x4bc tf:0x16f tm:5587574703.691 ns
      *** Subsubevent size  15 id 0x8000 full 000f8000
           [76] 1006f0f4  [77] a14df70f  [78] 00000032  [79] 9cbfd319  [80] 00000000  [81] 00000000  [82] 00000000  [83] 00000000
           [84] 00000000  [85] a14df70f  [86] 00000032  [87] 9cbfd319  [88] 00000000  [89] e0ffffff  [90] 21660000
      *** Subsubevent size   1 id 0x5555 full 00015555
           [92] 00000001
~~~~~~~~~~~~~~~~
 
All options can be obtain when running "hldprint -help"



# TDC calibration #  {#trb3_tdc}

Now DABC application can be also used to calibrate data, provided by FPGA TDCs.
For this functionality code from [stream framework](https://subversion.gsi.de/go4/app/stream) is used.
Therefore DABC should be compiled together with stream - at best as [trb3 package](https://subversion.gsi.de/dabc/trb3) as described in very beginning.

All details about TDC calibration in DABC or in Go4 can be found on [TDC calibration](@ref hadaq_tdc_calibr) page.


# Usage of hadaq API in other applications

hldprint is just program with originally about 150 lines of code 
(now it is 500 due to many extra options). 
Source code located in [$DABCSYS/plugins/hadaq/hldprint.cxx](https://subversion.gsi.de/dabc/trunk/plugins/hadaq/hldprint.cxx). 
There is also example in [$DABCSYS/applications/hadaq/](https://subversion.gsi.de/dabc/trunk/applications/hadaq/) directory, which can be copied and modified for the user needs.

In simplified form access to any data source (local file, remote file or online server) looks like:

~~~~~~~~~~~~~~~~
#include "hadaq/api.h"
int main() {
   hadaq::ReadoutHandle ref = hadaq::ReadoutHandle::Connect("file.hld");
   hadaq::RawEvent* evnt = 0;
   while(evnt = ref.NextEvent(1.)) {
      // any user code here
      evnt->Dump();
   }
}
~~~~~~~~~~~~~~~~

One can use such interface in any other standalone applications.


Any comments and wishes: S.Linev(at)gsi.de
