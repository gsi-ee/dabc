# FPGA TDC calibration #      {#hadaq_tdc_calibr}

## Table of contents

- [Basics](@ref hadaq_tdc_calibr_basics)
- [TDC calibrations in stream framework](@ref hadaq_tdc_calibr_stream)
- [Run code with go4](@ref hadaq_tdc_calibr_go4)
- [Run code with DABC](@ref hadaq_tdc_calibr_dabc)


---------------------------

# Basics # {#hadaq_tdc_calibr_basics}


## That is measured by FPGA TDC?
 
Typical sub-subevebt, produced by FPGA TDC looks like (_using hldprint_):

~~~~~~~~~~~~~~~
*** Event #0xcb1e71ea fullid=0x2001 runid=0x0eb0eb32 size 536 *** 
   *** Subevent size    504 decoding 0x020011 id 0xc940 trig 0x7bb1e7e7 swapped align 4 ***
      *** Subsubevent size  48 id 0x0940 full 00300940
         [  1] 21e70000  tdc header
         [  2] 63089e85  epoch 50896517 tm 5784258560.000 ns
         [  3] 80116af8  hit  ch: 0 isrising:1 tc:0x2f8 tf:0x116 tm:5784262357.315 ns
         [  4] 63089e85  epoch 50896517 tm 5784258560.000 ns
         [  5] 8051aae9  hit  ch: 1 isrising:1 tc:0x2e9 tf:0x11a tm:-75.043 ns
         [  6] 805492f4  hit  ch: 1 isrising:0 tc:0x2f4 tf:0x149 tm:-20.554 ns tot:54.489 ns
         [  7] 63089e85  epoch 50896517 tm 5784258560.000 ns
         [  8] 808e2ae9  hit  ch: 2 isrising:1 tc:0x2e9 tf:0x0e2 tm:-74.435 ns
         [  9] 809372f3  hit  ch: 2 isrising:0 tc:0x2f3 tf:0x137 tm:-25.359 ns tot:49.076 ns
      ...
~~~~~~~~~~~~~~~
 
Such subevent starts with TDC header (not discussed here).
Then epoch counter followed by one or several hit messages.    
 
Hit message provides channel number, rising(1) or falling(0) edge code and two time values.
First is **coarse** counter (*tc* in printout), incremented with 200 MHz frequency.
Second is **fine** counter (*tf* in printout), which started with the hit detection and ends with the next 5ns period. To produce time stamp for registered hit we should use following equation:

    STAMP = (epoch*2048 + coarse)*5ns - Calibr(fine)

One should understand - larger value of **fine** counter means longer distance to the next 5ns edge and smaller value of resulted absolute time stamp.

Form of **Calibr** function individual for each channel and can vary - for instance, because of temperature change.


## Can one use liner approximation?

In general yes - one could use liner approximation for **Calibr(fine)** function. Typically such function characterized by min and max value of **fine** counter bin. Minimal fine counter bin value (typically ~30) correspond to 0 shift, maximal fine counter (around 450-500) to -5 ns shift.

Main (and probably only) advantage of linear approximation - it is simple.

But liner approximation has several drawbacks. First of al, it introduces error into stamp value in the order of ~100 ps. It could be acceptable for signals with worse resultion. But another typical effect of linear approximation - it introduces double-peak structure on the signals which does not have double peaks at all. Especially such effect can be very well seen with test pulses.



## Statistical approach for fine-counter calibration

Approach is very simple - for _every_ channel one measures many (~1e5) hits from _random_ signal and build distribution for fine-counter values. Higher value in such distribution - larger width of correspondent bin. While sum of all bins widths is 5 ns, one can very easily calculate time shift corresponding to each fine counter.

This link shows shows typical distribution of fine-counter bins in the channel:

<http://jsroot.gsi.de/latest/?nobrowser&file=../files/temp44.root&item=Histograms/TDC_C100/Ch1/TDC_C100_Ch1_RisingFine;1>

![Fine counter distribution](http://dabc.gsi.de/doc/images/finecounter.jpg "Fine counter distribution")

As result from such distribution calibration function is build:

<http://jsroot.gsi.de/latest/?nobrowser&file=../files/temp44.root&item=Histograms/TDC_C100/Ch1/TDC_C100_Ch1_RisingCalibr;1>
 
![Fine counter calibration](http://dabc.gsi.de/doc/images/finecalibr.jpg "Fine counter calibration")

 
At the moment it is best-known method for calibration of fine counter. 

But it has some disadvantages. First - it typically requires special measurements to perform calibration. Potentially one could use _normal_ hits, but not allways normal measurements provide enough statistic for all channels. With less statistic precision will be much worse. 

Another problem of such calibration approach - significant size of produced calibration curves. For every channel one creates lookup table with approx 500 bins. If one uses setup with 1000 channels (not very big), every calibration set consume 1000 channels _X_ 500bins _X_ 4bytes = 2 MB of data.             


## Effect of temparature on the measurements

Effect is clear - calibration curves changing with the temperature. 
[This link compare calibration curves](http://jsroot.gsi.de/dev/index.htm?nobrowser&path=../files/&files=[temp44.root,temp35.root,temp28.root]&item=temp44.root/Histograms/TDC_C100/Ch1/TDC_C100_Ch1_RisingCalibr;1+temp35.root/_same_+temp28.root/_same_&opt=autocol,nostat), measured for three different temperatures: 28, 35 and 44 C. One could see difference upto 120 ps caused by temperature difference of about 16 grad. 
 

Such changes can be compensated if perfrom calibration frequently. In ideal case calibration should be done constantly - every time when temperature changes more than 0.5 grad. 

If calibration procedure is not performed, temperature changes will affect mean and rms values. Very rough estimation gives 10 ps change for temperature change on 1 grad. Potentially one could compensate tempreture changes without constantly calibrating TDCs channels , but at the moment it is not yet clear how. 


## Time stamp of falling edges - time-over-threshold measurement

FPGA TDC introducing time shifts for the falling edge of measured signals. Such time shift is individual for every channel and vary between 28 and 42 ns. This value should be subtracted from every fallind edge time stamp. How to measure this values?

Most simple approach - use internal pulse generator, which actiavted with trigger type 0xD. This signal has 30 ns pulse width. If one measures distance between rising and falling edges in this test pulse, one could easily estimate time shift, which could be later applyed for every falling-edge stamp.  

---------------------------


# TDC calibration with `stream` frameowrk #   {#hadaq_tdc_calibr_stream}

## That is `stream` framework? 

It is code to analyze data streams. Main features:

- It is pure C++ code, independent from [ROOT](https://root.cern.ch/) or [DABC](http://dabc.gsi.de) or [Go4](http://go4.gsi.de).
- Developed code very well integrated with ROOT, DABC and Go4.
- It can run offline (reading HLD files) or online (taking data directly from DAQ)
- It dedicated for unpacking and calibration of TRB/TDC data, means producing correct time stamps out of fine-counter values.
- Calibration also includes compensation of time-over-threshold (TOT) shifts,
  measured with internal pulser, produced with trigger type 0xD.
- Produced calibrated data can be stored in ROOT TTree,
  supporting two major formats: with absolute time stamps as double values,
  and with relative (to channel 0) time stamps as float values.
- Framework optionally provides number of histograms useful for online monitoring.
- If required, custom user code can be easily append to running application and
  access all produced data directly.

Original source code can be found in [Subversion repository](https://subversion.gsi.de/go4/app/stream/),
copy on the [github](https://github.com/linev/stream) 


## How to use `stream` framework for TRB/TDC

Normally any analysis configured in `first.C` script.
Example of such macros can be found in [applications folder](https://subversion.gsi.de/go4/app/stream/applications/).

Most simple example provided in [autotdc folder](https://subversion.gsi.de/go4/app/stream/applications/autotdc/) and looks like:

~~~~~~~~~~~~~~~{.c}
void first()
{
   base::ProcMgr::instance()->SetRawAnalysis(true);
   // base::ProcMgr::instance()->SetTriggeredAnalysis(true);

   // all new instances get this value
   base::ProcMgr::instance()->SetHistFilling(4);

   // this limits used for liner calibrations when nothing else is available
   hadaq::TdcMessage::SetFineLimits(31, 491);

   // default channel numbers and edges mask
   hadaq::TrbProcessor::SetDefaults(49, 2);

   // [min..max] range for TDC ids
   hadaq::TrbProcessor::SetTDCRange(0x900, 0x9FF);

   // [min..max] range for HUB ids
   hadaq::TrbProcessor::SetHUBRange(0x8100, 0x81FF);

   // when first argument true - TRB/TDC will be created on-the-fly
   // second parameter is function name, called after elements are created
   hadaq::HldProcessor* hld = new hadaq::HldProcessor(true, "after_create");

   // first parameter if filename  prefix for calibration files
   //     and calibration mode (empty string - no file I/O)
   // second parameter is hits count for autocalibration
   //     0 - only load calibration
   //    -1 - accumulate data and store calibrations only at the end
   //    >0 - automatic calibration after N hits in each active channel
   // third parameter is trigger type used for calibration
   //   0xD - special trigger with internal pulser, used also for TOT calibration
   //0xFFFF - all kind of trigger types will be used for calibration, no TOT calibration
   hld->ConfigureCalibration("", 100000, (1 << 0xD));

   // only accept trigger type 0x1 when storing file
   // new hadaq::HldFilter(0x1);

   // create ROOT file store
   // base::ProcMgr::instance()->CreateStore("td.root");

   // 0 - disable store
   // 1 - std::vector<hadaq::TdcMessageExt> - includes original TDC message
   // 2 - std::vector<hadaq::MessageFloat>  - compact form, without channel 0, stamp as float (relative to ch0)
   // 3 - std::vector<hadaq::MessageDouble> - compact form, with channel 0, absolute time stamp as double
   base::ProcMgr::instance()->SetStoreKind(3);
}

~~~~~~~~~~~~~~~

Idea of such configuration - all required elements are created when first HLD event is processed.


## Configure calibration

Central method is:

    hld->ConfigureCalibration("", 100000, (1 << 0xD));

Method has three parameters. 

First is file prefix for calibration functions. For every TDC separate file will be created. For instance, if prefix is "path/file_", than filename for TDC 0x954 will be "path/file_0954.cal". When file prefix is empty, no file I/O will be preformed.

Second parameter defines how often automatic calibration will be performed. It is number of counts in the channel which are required to produce calibration. Normally is such count is about 100000, reasonable range for this value 5e4 - 1e6. If value is equal to 0, no automatic calibration is performed. Special case is -1 value - statistic is accumulated for complete data set (hld file) and calibration produced at the very end.  
  
Third (optional) parameter is trigger type, used in calibration. By default all kind of triggers used in calibration (value 0xFFFF). One could specify particular type. Special case is 0xD trigger, which contains signal of internal pulse generator with 30ns wide pulse. Such signal used for fine counter calibration and for TOT calibration. 

Example shows normal configuration of automatic calibration, used for online tests. When application starts, linear calibration will be used until necessary amount of counts is accumulated. Then calibration function is produced and used for following data.  

Another approach of calibration - produce calibration on some predefined data set and use it for other data. In such case one should configre:

      hld->ConfigureCalibration("prefix_", -1, (1 << 0xD));
      
And run analysis with calibration data set. If using go4, one do:

     go4analysis -user filename.hld
     
At the end calibration will be produced for every TDC and such calibration can be used:

    hld->ConfigureCalibration("prefix_", 0, (1 << 0xD));

Now one could process any other hld file (or even same file again). Calibration will not change.

When specifying file prefix with automatic calibration, then after every calibration loop produced calibration function will be stored in the file. Like:

    hld->ConfigureCalibration("prefix_", 100000, (1 << 0xD));

This allows to reuse latest calibration function when restarting analysis - in online or offline mode.   


## Configure linear calibration

If no any other calibration is applied, linear calibration with identical min/max limits for all channels is used.
This limits can be configured with the call:

    hadaq::TdcMessage::SetFineLimits(22, 480);
    
But such linear approximation is very inprecise, while different channels can have very different maximal value.

One could enable special mode in calibarion algorithm, which search and assigns individual min/max limits for each channel. To enable such mode, one should use **10077** as number (or **10000*M+77**) of hits for automatic calibration:

    hld->ConfigureCalibration("prefix_", 100077, (1 << 0xD));
 
In such case not a real calibration function will be created, but just min/max values of fine counter will be identified and linear function for correspondent channel will be created. 
 
If individual linear calibrations should be produced base on HLD file, one should use **-77** instead of -1 argument:
 
    hld->ConfigureCalibration("prefix_", -77, (1 << 0xD));
      
This will search min/max value for each channel in whole HLD file data and create linear calibration at the end.     
    

## Enable data storage 

After unpacking and calibration, produced TDC data can be stored in ROOT TTree. For that one should use:

    base::ProcMgr::instance()->CreateStore("file.root"); 

This will create ROOT file with the TTree object.

There are three different formats of data, stored in the ROOT file, which can be configured with:

    base::ProcMgr::instance()->SetStoreKind(3);

In all cases each TDC processor creates separate branch in the TTree, where correspondent data will be stored. Each branch is _std::vector_ of special message format. Following messages are used:

| Kind  | Message | Description  |
| ----: | :-----: | :---- |
|   0   |    no   |  disable storage |
|   1   | hadaq::TdcMessageExt | message includes original hadaq::TdcMessage plus time stamp relative to channel 0  |
|   2   | hadaq::MessageFloat | message includes channel id, edge code and stamp as float value (ns) relative channel 0  |
|   3   | hadaq::MessageDouble | message includes channel id, edge code and absolute time stamp as double (s), including channel 0  |

Definition of all message types can be found in [TdcSubEvent.h](https://subversion.gsi.de/go4/app/stream/include/hadaq/TdcSubEvent.h)


## Produce monitoring histograms

Many different histograms can be accumulated to monitor quality of data and quality of TDC calibration.
First of all, one should specify histogramming level:

      base::ProcMgr::instance()->SetHistFilling(4);
 
Following levels can be specified:

| Level |  Description  |
| ----: | :---- |
|   0   | no histograms |
|   1   | only basic histograms for HLD and TRB |
|   2   | basic statistic for each TDC  |
|   3   | per-channel histograms for each TDC |
|   4   | also cross-channel references  |

For instance, when level 4 consfigured one could see histogram with reference between different channels.
This can be done in **after_create** function which looks like:

~~~~~~~~~~~~~~~{.c}
extern "C" void after_create(hadaq::HldProcessor* hld)
{
   printf("Called after all sub-components are created\n");

   if (hld==0) return;

   for (unsigned k=0;k<hld->NumberOfTRB();k++) {
      hadaq::TrbProcessor* trb = hld->GetTRB(k);
      if (trb==0) continue;
      printf("Configure %s!\n", trb->GetName());
      trb->SetPrintErrors(10);
   }

   for (unsigned k=0;k<hld->NumberOfTDC();k++) {
      hadaq::TdcProcessor* tdc = hld->GetTDC(k);
      if (tdc==0) continue;

      printf("Configure %s!\n", tdc->GetName());

      // tdc->SetUseLastHit(true);
      for (unsigned nch=2;nch<tdc->NumChannels();nch++)
         tdc->SetRefChannel(nch, 1, 0xffff, 2000,  -10., 10.);
   }
}
~~~~~~~~~~~~~~~

Here one uses channel 1 as reference for all other channels:

    tdc->SetRefChannel(nch, 1, 0xffff, 2000,  -10., 10.);
   
Third argument (0xffff) can be ID of any other TDC where reference channel could be used. Next three arguments are number of bins and min/max time (ns) for histogram where reference time will be accumulated. 

---------------------------

# Use of `stream` framework in `Go4` #    {#hadaq_tdc_calibr_go4}

[Go4](http://go4.gsi.de) is ROOT-based analyzis framework and provide many useful tools to run arbitrary code in online/offline environment. 

After configuring all environemnt variables (typically with `. trb3login`) one could directly start analysis, providing proper *first.C* file. In working directory where first.C is located just type:

    [shell] go4analysis -user filename.hld  // for offline
    [shell] go4analysis -stream daq_server:port // for online

TDC calibration, ROOT file storage, histogram monitoring works as described in previous section.


---------------------------


# TDC calibration in `DABC` # {#hadaq_tdc_calibr_dabc}

Code, developed in `stream` framework, can be run directly in DABC. DABC compiles **libDabcStream.so** library, which provides binding of `stream` code with DABC. This library should be always included in DABC xml configuration file if `stream` code should be used in DABC. If ROOT file storage should be used, one should also include **libDabcRoot.so** library for ROOT binding.   


## Run first.C after event building

Most easy way of using `stream` in DABC. In _hadaq::CombinerModule_ one configures additional output, which activates `stream` code in DABC. Like:

      <OutputPort name="Output2" url="stream://file.root?maxsize=5000&kind=3&hlevel=4"/>

This will create `file.root` file with output TTree of kind=3 (absolute double time stamps) of maximum size 5000 MB. One also can run `stream` code without writing data into ROOT file:

      <OutputPort name="Output2" url="stream://nofile?hlevel=4"/>

In such case only configured monitoring histograms will be available. Such histograms could be accessed with http interface opening in web browser address like _http://localhost:8090/_. Or same histogram can be viewed in the go4 gui starting it with command:

    [shell] go4 http://localhost:8090/   

DABC also provides web interface for controlling TDC calibration status and for start/stop file writing interactively. Such GUI can be activated when clicking of 'EventBuilder/StreamMonitor' item in the browser or just by opening page with address: _http://localhost:8090/?layout=grid2x2&item=EventBuilder/StreamMonitor_. 



## Produce HLD files with calibrated values

Such mode allows to produce HLD files with already calibrated values.

### Configuration

There is example configuration file [$DABCSYS/plugins/hadaq/app/TdcEventBuilder.xml](https://subversion.gsi.de/dabc/trunk/plugins/hadaq/app/TdcEventBuilder.xml), which shows how one could configure TRB, TDC and HUB ids for each input.
This loook like:

       <InputPort name="Input0" url="hadaq://host:10101" urlopt1="trb=0x8000&tdc=[0x3000,0x3001,0x3002,0x3003]&hub=0x8010"/>
       <InputPort name="Input1" url="hadaq://host:10102" urlopt1="trb=0x8010&tdc=[0x3010,0x3011,0x3012,0x3013]"/>
  
For each input [TDC calibration module](@ref stream::TdcCalibrationModule) will be created with name 'Input0TdcCal' for first input, 'Input1TdcCal' for second input and so on. One could specify additional parameters for such modules in common section:

    <Module name="Input*TdcCal">
       <FineMin value="31"/>
       <FineMax value="480"/>
       <NumChannels value="65"/>
       <EdgeMask value="1"/>
       <HistFilling value="3"/>
       <CalibrFile value="local"/>
       <Auto value="100000"/>
       <Replace value="true"/>
    </Module>

Comments for most parameters are provided in example file. 


### Output formats

When running, calibration modules extracts hits data, accumulate statistics and produce calibration.
Calibration module use such calibrations to calculate correct value which correspond to the **fine time** counter. For falling edge also time shift is compensated.

Result can be stored in two different ways. Either one replace **fine time**  in the hit message with calibrated value, and changes message type from `hit` to `hit2`. This corresponds to `<Replace value="true"/>` in configuration.

Or one could insert additional `calibr` message in the data stream, where calibrated value is stored. This increase size of HLD data (approx by 25%), but preserve original hits as they are. Such method is also more precise, while instead of 10 bits one could use 14 bits for storing result. Such method used when `<Replace value="false"/>` specified in configuration.        


### `hit2` message format

It is to large extend similar with original `hit` message. There are two differences. First, it has 0xa0000000 message type insted of 0x80000000. Second, 10 bits of fine counter coding time used for coding of calibrated fine time value, which should be _SUBSTRUCTED_ from coarse time value. As in original hit message, value 0x3ff (or 1023) is error.

Decoding of these 10 bits depend on the edge bit. For rising edge 5ps binning is used. For instance, value 26 means -130 ps, value 200 is -1ns. 

For falling edge lower 9 bits used to code value with 10ps binning. Means value 25 correspond to -250 ps time shift. When higher bit set, it indicates overflow of coarse counter due to time-shift compensation. In this case time for whole epoch (2048*5ns) should be substracted from global time stamp


### `calibr` message format

This message kind used to store calibrated values without change of the original data.
This is message has type 0xe0000000. Available 28 bits shared between two following hit messages: lower 14 bits correspond to first message, higher 14 bits to second hit message. Decoding depends from edge kind.
For both case value 0x3fff indicates error.

For rising edge hits time shift could be calculated with equation : value / 0x3ffe * 5ns. It corresponds approximately to 0.30519 ps binning.

For falling edge hits time shift could be calculated as: value / 0x3ffe * 50ns or approximately 3.0519 ps binning.


### Using in hldprint and analysis 

Both `hldprint` and `stream` analysis will recognize new message types and provide
time stamp without need to apply any kind of calibration. 


### Control with web interface

DABC also provides specialized web control gui, which shows DAQ and TDC calibration status.
To activate it, one should open _http://localhost:8090/?item=EventBuilder/HadaqCombiner_
or just click `EventBuilder/HadaqCombiner` in the browser. 
One could see DAQ state, configured TRB/TDC ids and progress of TDC calibration.
It is also possible to start/stop HLD file writing.

When terminal module is enabled in configuration file (default on), it also provides information about progress of TDC calibration. One could request generic state for all TDC with request:

    wget http://localhost:8090/EventBuilder/Terminal/State/value/get.json -O state.json

If calibration peformed for all TDCs, "Ready" string will be returned, otherwise "Init"
