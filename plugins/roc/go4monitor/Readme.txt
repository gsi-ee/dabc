This is online/offline Go4 analysis of raw ROC data, produced by DABC

Requirements:

ROOT (http://root.cern.ch) and Go4 (http://go4.gsi.de) packages must be installed.
Enviroment variables ROOTSYS, GO4SYS, PATH and LD_LIBRARY_PATH should be setup.


Build the package:

Just call make in directory where analysis is checked out. This produce
MainUserAnalysis executable and libGo4UserAnalysis.so library.


Description of the package:

Files:
MainUserAnalysis - main program
TRocProc         - processor with user analysis code
TRocParam        - place for user-defined parameters
SysCoreData      - class from KNUT package 
AnalysisStart.sh - shell script to run analysis from go4 GUI via secure shell (ssh)

All classes are defined and declared in two files (*.h and *.cxx)
In MainUserAnalysis main analysis objects are created.
The data can be read from standard GSI lmd files or from running DABC stream servers.
For each event the user event function TRocProc::BuildEvent() is called.
This user event processor fills some histograms from the input event.


The processor:    TRocProc
The analysis code is in the event processor TRocProc. Members are
histograms and parameter pointers used laster in data processing.
In the constructor of TRocProc the histograms and parameters are created.
Method BuildEvent - called event by event - gets input event pointer 
from the framework. Main loop goes throw all subevents  - 
data portions from different ROCs. For each ROC number of individual 
histograms are filled.

Parameter class: TRocParam
In this class one can store parameters, and use them in all places of analysis.
Parameters can be modified from GUI.

Autosave file mechanism:
This is a file, where all analysis objects (histograms, parameters) are 
saved in the end of data analysis (by default autosave is disabled).
If enabled, at startup time the autosave file is read and all objects 
are than restored from that file.
When TGo4Analysis object is created, the autosave file is not yet loaded. 
Therefore the objects created here are overwritten by the objects 
from autosave file (if any), except histograms.
From GUI, objects are loaded from autosave file when Submit button is pressed.
One can inspect the content of the auto save file with the Go4 GUI.
Note that
GO4USERLIBRARY=/mypath/libGo4UserAnalysis.so
should be defined to enable the GUI to read the auto save file.

Creating a new class:
Provide the definition and implementation files (.h and .cxx)
Add class in LinkDef.h
Then "make all"

Use the example with lmd files:
Run go4, lunch analysis on host where lmd files are stored.
In analysis configuration set file names - one also can use "*" and "?" symbols
to process several files at once. Press Submit and start button.
While analysis is running, one can see all histograms.

Run in batch mode:
Just run analysis program like this:
   ./MainUserAnalysis 'file_name*.lmd'
where file_name is single file name or mask for several file names.
Analysis will run throw all events and produce Go4AutoSave.root file 
with filled histograms. Histograms can be dispatched either in Go4 gui
or in ROOT browser.

More info: go4.gsi.de

S.Linev@gsi.de
