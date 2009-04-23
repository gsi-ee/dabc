//============================================================================
// Name        : control.C
// Author      : Sergey Linev
// Version     : 1.0
// Copyright   : GSI
// Description : demonstrates the use of the roc::Board class in ROOT macro
//============================================================================

// To be able use this script, one need DABC roc plugin be compiled

// To be able run script with ACLiC, include path should be configured
// [shell] root -l
// root [0] .include $DABCSYS/include
// root [1] .x control.C+("cbmtest01")

#ifndef __CINT__
#include "Riostream.h"
#include "TBenchmark.h"
#include "roc/Board.h"
#include "roc/Defines.h"
#include "nxyter/Data.h"
#endif
// we need include only when we want to compile script

void control(const char* boardaddr = "lxg0526")
{
   if (strlen(boardaddr)==0) {
      cerr << "BoardIP cannot be defined. Exit." << endl;
      return;
   }

   roc::Board* brd = roc::Board::Connect(boardaddr, roc::roleDAQ);
   if (brd==0) {
      cerr << "Cannot connect to board " << boardaddr << endl;
      return;
   }

   cout << " Board addr:" << boardaddr << endl;

   brd->startDaq();

   gSystem->Sleep(500);

   brd->stopDaq();

   roc::Board::Close(brd);

   return;


   brd->poke(ROC_DO_TESTSETUP,1);

   brd->poke(ROC_NX_REGISTER_BASE + 0,255);
   brd->poke(ROC_NX_REGISTER_BASE + 1,255);
   brd->poke(ROC_NX_REGISTER_BASE + 2,0);
   brd->poke(ROC_NX_REGISTER_BASE + 18,35);

   brd->poke(ROC_NX_REGISTER_BASE + 32,1);

   brd->poke(ROC_TESTPULSE_RESET_DELAY, 100);
   brd->poke(ROC_TESTPULSE_LENGTH, 500);
   brd->poke(ROC_TESTPULSE_NUMBER, 10);

   brd->poke(ROC_FIFO_RESET, 1);
   brd->poke(ROC_BUFFER_FLUSH_TIMER,1000);

   brd->poke(ROC_TESTPULSE_START, 1);

   bool res = brd->startDaq();
   cout << "starting readout..." << (res ? "OK" : "Fail") << endl;
   if (!res) { roc::Board::Close(brd); return; }

   nxyter::Data data;
   long numdata = 0;

   gBenchmark->Start("GetData");

   while (brd->getNextData(data, 0.1)) {
      data.printData(7);
      numdata++;

      if (data.isStopDaqMsg()) break;

      if (numdata==100) res = brd->suspendDaq();
   }

   res = brd->stopDaq();

   cout << "Stop DAQ " << (res ? "OK" : "Fails") << endl;

   gBenchmark->Show("GetData");

   cout << numdata << " messages received" << endl;

   roc::Board::Close(brd);
}
