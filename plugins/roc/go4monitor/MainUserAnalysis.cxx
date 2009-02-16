/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#include "Riostream.h"
#include <string.h>
#include <stdlib.h>

#include "TROOT.h"
#include "TApplication.h"
#include "snprintf.h"

#include "TGo4StepFactory.h"
#include "TGo4AnalysisStep.h"
#include "TGo4Analysis.h"
#include "TGo4AnalysisClient.h"
#include "Go4EventServerTypes.h"

TROOT go4application("GO4","Go4 user analysis");

void usage()
{
   cout << endl;
   cout << "* Roc online/offline analysis  " << endl;
   cout << "* S.Linev, GSI, Darmstadt" << endl;
   cout << "* calling:                " << endl;
   cout << "* MainUserAnalysis -server | -gui FastRoc guihost guiport | runname*.lmd " <<endl;
   cout << endl;
}

//==================  analysis main program ============================
int main(int argc, char* argv[])
{
   if(argc < 2) {
      usage(); // no argument: print usage
      return 0;
   }

   //=================  init root application loop ==========================
   TApplication theApp("Go4App", 0, 0);
   
   //================= Begin analysis setup ==================================
   // argv[0] program
   // argv[1] "-gui" when started by GUI.
   //         In this case the following args are (delivered by GUI):
   // argv[2] analysis name as specified in GUI
   // argv[3] hostname of GUI
   // argv[4] connector port of GUI
   ////////////
   // argv[1] "-server" when started as analysis server by GUI client
   
   Bool_t servermode = kFALSE; // run analysis slave as servertask
   Bool_t batchmode = kFALSE; // run analysis in batch mode
   
   char hostname[256];      // gui host name
   UInt_t iport=5000;       // port number used by GUI
   const char* lmdfilename = "/d/cbm06/cbmdata/SEP08/raw/run001/run001_*.lmd";
   
   
   strcpy(hostname,"localhost");
   
   //------ process arguments -------------------------
   
   if(strcmp(argv[1],"-gui") == 0) {  // set up arguments for GUI mode
      strncpy(hostname,argv[3], sizeof(hostname));
      iport = (argc>4) ? atoi(argv[4]) : 5000; // port of GUI server
      servermode = kFALSE;
   } else 
   if(strcmp(argv[1],"-server") == 0) {
   // set up analysis server mode started from GUI -client
      servermode = kTRUE;
   } else { 
      batchmode = kTRUE;
      lmdfilename = argv[1];
   }
   
   //------ setup the analysis -------------------------
   
   // We will use only one analysis step (factory)
   // we use only standard components
   TGo4Analysis*     analysis = TGo4Analysis::Instance();
   TGo4StepFactory*  factory  = new TGo4StepFactory("Factory");
   TGo4AnalysisStep* step     = new TGo4AnalysisStep("Analysis",factory,0,0,0);
   step->SetEventSource(new TGo4MbsFileParameter(lmdfilename));
   step->SetSourceEnabled(kTRUE);
   analysis->AddAnalysisStep(step);
   
   // tell the factory the names of processor and output event
   // both will be created by the framework later
   // Input event is by default an MBS event
   factory->DefEventProcessor("RocProc","TRocProc");// object name, class name
   factory->DefOutputEvent("RocEvent","TGo4EventElement"); // object name, class name
   
   if (batchmode) {
      cout << "**** Main :: Processing file(s) " << lmdfilename << " in batch mode" << endl;
      analysis->SetAutoSave(kTRUE);   // optional enable auto-save
      if (analysis->InitEventClasses()) {
         analysis->RunImplicitLoop(2000000000L);
         delete analysis;
         cout << "**** Main: Done!"<<endl;
      } else
         cout << "**** Main: Init event classes failed, aborting!"<<endl;   
      
      theApp.Terminate();
   } else {
      //==================== password settings for gui login (for analysis server only)
      if(servermode)  {
         analysis->SetAdministratorPassword("Rocadmin");
         analysis->SetControllerPassword("Rocctrl");
         analysis->SetObserverPassword("Rocview");
         // note: do not change go4 default passwords for analysis in client mode
         // autoconnect to gui server will not work then!!!
      }
   
      //------ start the analysis -------------------------
   
      if(servermode)  cout << "**** Main: starting analysis in server mode ..." << endl;
      else            cout << "**** Main: starting analysis in slave mode ..." << endl;
        // to start histogram server: kTRUE,"base","password"
      TGo4AnalysisClient* client = new TGo4AnalysisClient("RocAnalysis", analysis, hostname, iport, kFALSE, "", "", servermode, kFALSE);
      cout << "**** Main: created AnalysisClient Instance: "<<client->GetName()<<endl;
      //=================  start root application loop ==========================
      cout << "**** Main: Run application loop" << endl;
      theApp.Run();
   }
   
   return 0;
}
