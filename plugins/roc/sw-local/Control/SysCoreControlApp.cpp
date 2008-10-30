//============================================================================
// Name        : SysCoreControlApp.cpp
// Author      : Stefan Müller-Klieser
// Version     :
// Copyright   : Kirchhoff-Institut für Physik
// Description : demonstrates the use of the SysCoreControl class
//============================================================================

#include "SysCoreControl.h"

#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <iostream>
using namespace std;


//used for time measuring
unsigned long long int tick(void)
{
   struct timeval t;
   gettimeofday(&t, NULL);

   return (t.tv_sec * 1000 * 1000) + t.tv_usec;
}

int main(int numc, char* args[])
{
   cout << "Usage: SysCoreControl boardip" << endl;
   const char* boardip = "129.206.177.44";

   if (numc>1) boardip = args[1];

   cout << " BoardIp:" << boardip << endl;

   SysCoreControl control;
   int boardnum = control.addBoard(boardip);

   cout << control.getNumberOfBoards() << " SysCore boards are configured." << endl;
   if(control.getNumberOfBoards() == 0) {
      cout << "No boards connected ... exit!" << endl;
      return -1;
   }

   int ret = 1;

   ret = control[boardnum].poke(ROC_CONSOLE_OUTPUT, 1);

   ret = control[boardnum].poke(ROC_TESTPATTERN_GEN, 1);

   cout << "starting readout..." << endl;
   if (!control[boardnum].startDaq(30)) {
       cout << "Start DAQ fails" << endl;
       return -1;
   }

   unsigned long long startTime = tick();

   SysCoreData data;
   long rec = 0;

   for(int i = 0; i < 1000000; i++)
      if (control[boardnum].getNextData(data, 0.1)) {
         //data.printData(3);
         rec += 6;
      } else
         printf("timeout\n");

   if (control[boardnum].stopDaq())
       cout << "Stop DAQ done" << endl;
    else
       cout << "Stop DAQ fails" << endl;

   unsigned long long endTime = tick();
   cout << rec << " bytes received in " << endTime - startTime << " usec" << endl;

   return 0;
}
