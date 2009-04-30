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

void control(const char* boardaddr = "cbmtest08")
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

   bool res = brd->startDaq();

   if (!res) {
      cout << "Cannot start daq " << endl;
      roc::Board::Close(brd);
      return;
   }


   uint32_t readValue, op;

   gBenchmark->Start("1000peek");
   for (op=0;op<1000;op++)
      readValue = brd->peek(0x100000);
   gBenchmark->Show("1000peek");


   nxyter::Data data;
   long numdata = 0;

   gBenchmark->Start("GetData");

   cout << "Start getting data " << endl;

   while (brd->getNextData(data, 2.5)) {
//      data.printData(7);
      numdata++;

      if (data.isStopDaqMsg()) { cout << "GET It" << endl;  break; }

      if (numdata==1000) res = brd->suspendDaq();
   }

   res = brd->stopDaq();

   cout << "Stop DAQ " << (res ? "OK" : "Fails") << " Get " << numdata << " messages" << endl;

   gBenchmark->Show("GetData");

   roc::Board::Close(brd);

}
