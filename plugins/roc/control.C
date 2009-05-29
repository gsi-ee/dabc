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
#include "roc/defines.h"
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


   uint32_t readValue, op;

   gBenchmark->Start("1000gets");
   for (op=0;op<1000;op++)
      brd->get(0x100000, readValue);
   gBenchmark->Show("1000gets");

   brd->GPIO_setActive(1, 1, 0, 0);
   brd->GPIO_setScaledown(0);

//   roc::Board::Close(brd);
//   return;


   bool res = brd->startDaq();

   if (!res) {
      cout << "Cannot start daq " << endl;
      roc::Board::Close(brd);
      return;
   }

   nxyter::Data data;
   long numdata = 0;

   gBenchmark->Start("GetData");

   cout << "Start getting data " << endl;

   while (brd->getNextData(data, 5.5)) {
//      printf("%6d  ", numdata); data.printData(7);
      numdata++;

      if (data.isStartDaqMsg()) cout << "GET Start" << endl;
      if (data.isStopDaqMsg()) { cout << "GET End" << endl;  break; }
      if (data.isNopMsg()) cout << "NOP !?" << endl;

      if (numdata==100000) res = brd->suspendDaq();
   }

   res = brd->stopDaq();

   cout << "Stop DAQ " << (res ? "OK" : "Fails") << " Get " << numdata << " messages" << endl;

   gBenchmark->Show("GetData");

   roc::Board::Close(brd);

}
