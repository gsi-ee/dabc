//============================================================================
// Name        : SysCoreControlApp.cpp
// Author      : Stefan Mueller-Klieser
// Version     :
// Copyright   : Kirchhoff-Institut fuer Physik
// Description : demonstrates the use of the SysCoreControl class
//============================================================================

#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "SysCoreControl.h"

#include <iostream>
using namespace std;

int main(int argc, char* argv[])
{
   cout << "Data uploader for SysCoreBoard" << endl;
   cout << "Can be used for firmware, software or config upload" << endl;
    
   cout << "Usage: upload boardIp -fw|-sw|-cfg fileName [rocFileName]" << endl;
    
   if (argc<4) {
       cout << "Not all parameters are specified!!!" << endl;
       return -1;
   }

   const char* boardIp = argv[1];
   int typ = -1;
   if (!strcmp(argv[2], "-fw")) typ = 1; else
   if (!strcmp(argv[2], "-sw")) typ = 2; else
   if (!strcmp(argv[2], "-cfg")) typ = 3; else {
      cout << "Invalid type of upload " << argv[2] << endl;
      return -1;   
   }
   const char* fileName = argv[3];
   const char* tgtFileName = 0;
   if ((argc>4) && (typ==3)) tgtFileName = argv[4]; 

   SysCoreControl control;

   int boardnum = control.addBoard(boardIp);

   if(!control.isValidBoardNum(boardnum)) {
       cout << "-----   Setup could not connect to board. Exit!   -----" << endl;
       return 0;
   }

   control[boardnum].poke(ROC_CONSOLE_OUTPUT, 3);

   bool upload_res = false;

   switch(typ) {
      case 1:
         cout << "-----   Uploading frimware: " << fileName << " -------- " << endl;
         upload_res = control[boardnum].uploadBitfile(fileName, 1);
         break;
      case 2:   
         cout << "-----   Uploading software: " << fileName << " -------- " << endl;
         upload_res = control[boardnum].uploadSDfile(fileName, "image.bin");
         break;
      case 3:   
         if (tgtFileName==0) tgtFileName = "roc1.cfg";
         cout << "-----   Uploading configuration: " << fileName << " to " << tgtFileName << endl;
         upload_res = control[boardnum].uploadSDfile(fileName, tgtFileName);
      default:
         cout << "Error type !" << endl;
    }

   control[boardnum].poke(ROC_CONSOLE_OUTPUT, 0);
    
   if ((typ==2) && upload_res) {
      cout << "-----   Restart ROC -------- " << endl;
      control[boardnum].poke(ROC_SYSTEM_RESET, 1);
   }

   return 0;
}
