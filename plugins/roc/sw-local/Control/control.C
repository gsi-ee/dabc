//============================================================================
// Name        : control.C
// Author      : Sergey Linev
// Version     :
// Copyright   : GSI
// Description : demonstrates the use of the SysCoreControl class in ROOT macro
//============================================================================

// To be able use this script, one need libKnut.so be compiled
// At the time, when libKnut.so is compiling, ROOT should be already 
// installed and correctly configured - means ROOTSYS must be defined
// and LD_LIBRARY_PATH should has in its list $ROOTSYS/lib directory.
// After libKnut.so compiled, one should configure LD_LIBRARY_PATH for it like:
// [shell] export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`
// Run simulator on some remote node with command:
// [shell] ./SysCoreSimulator
// or use available ROC board. Use simulator or ROC IP address when running script:
// [shell] root -l 'control.C("140.181.96.121")'
// To run script in compiled mode (with ACLiC), one should add + after script name
// [shell] root -l 'control.C+("140.181.96.121")'
// If script should be run in directory other when ../Control, include path must be configured:
// [shell] root -l
// root [0] .include /home/user/knut/Control
// root [1] .x control.C+("140.181.96.121") 

#include "SysCoreDefines.h"

// we need include only when we want to compile script
//#include "Riostream.h"
//#include "TString.h"
//#include "TBenchmark.h"
//#include "SysCoreData.h"
//#include "SysCoreControl.h"

void control(TString boardip = "140.181.115.40")
{
   if (boardip.Length()==0) {
      cerr << "BoardIP cannot be defined. Exit." << endl;
      return;  
   }   
   
   cout << " BoardIp:" << boardip << endl;
   
   SysCoreControl control(13131, 13132);
   int boardnum, ret;
   boardnum = control.addBoard(boardip.Data());

   cout << control.getNumberOfBoards() << " SysCore boards are configured." << endl;
   if(control.getNumberOfBoards() == 0)
   {
      cout << "No boards connected ... exit!" << endl;
      return;
   }   

   ret = control[boardnum].poke(ROC_DO_TESTSETUP,1);
   
   ret = control[boardnum].poke(ROC_NX_REGISTER_BASE + 0,255);
   ret = control[boardnum].poke(ROC_NX_REGISTER_BASE + 1,255);
   ret = control[boardnum].poke(ROC_NX_REGISTER_BASE + 2,0);
   ret = control[boardnum].poke(ROC_NX_REGISTER_BASE + 18,35);
   
   ret = control[boardnum].poke(ROC_NX_REGISTER_BASE + 32,1);
   
   ret = control[boardnum].poke(ROC_TESTPULSE_RESET_DELAY, 100);
   ret = control[boardnum].poke(ROC_TESTPULSE_LENGTH, 500);
   ret = control[boardnum].poke(ROC_TESTPULSE_NUMBER, 10);
   
   ret = control[boardnum].poke(ROC_FIFO_RESET, 1);
   ret = control[boardnum].poke(ROC_BUFFER_FLUSH_TIMER,1000);

   ret = control[boardnum].poke(ROC_TESTPULSE_START, 1);
    
   bool res = control[boardnum].startDaq(30);
   cout << "starting readout..." << (res ? "OK" : "Fail") << endl;
   if (!res) return;
   
   SysCoreData data;
   long numdata = 0;
   
   gBenchmark->Start("GetData");
   
   while (control[boardnum].getNextData(data, 0.1)) {
      data.printData(7);
      numdata++;
         
      if (data.isStopDaqMsg()) break;
         
      if (numdata==100) res = control[boardnum].suspendDaq();
   }
   
   res = control[boardnum].stopDaq();
   
   cout << "Stop DAQ " << (res ? "OK" : "Fails") << endl;

   gBenchmark->Show("GetData");

   cout << numdata << " messages received" << endl;
}
