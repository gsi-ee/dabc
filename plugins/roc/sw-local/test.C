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

#include "Riostream.h"
#include "TSystem.h"

#include "Control/SysCoreControl.h"
#include "Control/SysCoreData.h"
#include "Control/SysCoreDefines.h"

void test_lost(const char* boardip)
{
   SysCoreControl control;//hostIp
   int ret;
   int boardnum = control.addBoard(boardip);

   if(control.getNumberOfBoards() == 0) {
      cout << "No boards connected ... exit!" << endl;
      return;
   }
   
   if (!control.isValidBoardNum(boardnum)) {
      cout << "Cannot access board with number " << boardnum << endl;
      return;
   }

//   control[boardnum].poke(ROC_RESTART_NETWORK, 0x1, .1, .1);
//   return;
//
//
   control[boardnum].poke(ROC_CONSOLE_OUTPUT, 2);
//   control[boardnum].saveConfig();
//   control[boardnum].loadConfig();
//   control[boardnum].poke(ROC_TEMAC_PRINT, 3);
//   control[boardnum].sendConsoleCommand("help");
//   control[boardnum].poke(ROC_CONSOLE_OUTPUT, 0);
//   return;

/*

   control[boardnum].poke(ROC_CONSOLE_OUTPUT, 2);

//    gSystem->Sleep(100);

//    gSystem->Sleep(500);

//    control[boardnum].poke(ROC_TEMAC_REG0, 0x3300, 5., 5.);

    control[boardnum].poke(ROC_TEMAC_PRINT, 0);
    
//   control[boardnum].sendConsoleCommand("help");

//   long tm1 = gSystem->Now();
//   bool uploadres = control[boardnum].uploadSDfile("../../sw-host/image.bin", "image.bin");
//   bool uploadres = control[boardnum].uploadSDfile("mytest.txt");
//   bool uploadres = control[boardnum].uploadSDfile("small.txt");
//   long tm2 = gSystem->Now();
//   printf("One uses %ld msec to upload file\n", tm2-tm1);

   control[boardnum].poke(ROC_CONSOLE_OUTPUT, 0);

//   if (uploadres) control[boardnum].poke(ROC_SYSTEM_RESET, 1);

   return;
   
//   ret = control[boardnum].uploadBitfile("Debug/download.bit", 1);
//   return;
   
   

//  ret = control[boardnum].poke(ROC_SYSTEM_RESET, 1);
//  return;

//   ret = control[boardnum].poke(ROC_NUMBER, 0);


*/
   ret = control[boardnum].poke(ROC_NX_SELECT, 1);
   ret = control[boardnum].poke(ROC_NX_ACTIVE, 1);
   ret = control[boardnum].poke(ROC_SYNC_M_SCALEDOWN, 0);
   ret = control[boardnum].poke(ROC_AUX_ACTIVE, 7);

   ret = control[boardnum].poke(ROC_FIFO_RESET, 1);
   ret = control[boardnum].poke(ROC_BUFFER_FLUSH_TIMER, 100);
   

   ret = control[boardnum].poke(ROC_BURST, 0);
   ret = control[boardnum].poke(ROC_BURST_LOOPCNT, 150);

//   ret = control[boardnum].poke(ROC_BURST, 1);
//   ret = control[boardnum].poke(ROC_OCM_LOOPCNT, 2000);
   
//   ret = control[boardnum].poke(ROC_TESTPULSE_START, 1);

//   unsigned long long startTime = tick();
   
   bool res = control[boardnum].startDaq(50);
   cout << "Starting DAQ...  " << (res ? "OK" : "Fails") << endl;
   if (!res) return;

//   gSystem->Sleep(50);


//   cout << "Did sleep" << endl;

   SysCoreData data;
   long fullsz = 0;
   int printcnt = 0;
   
   long lasttm = gSystem->Now();
   
   long starttm = lasttm;
   
   bool didsuspend = false;
   
   bool didsleep = false;
   
   unsigned lastsync = 0;
   
   long numepochs = 0;
   long nummissed = 0;
   
   long totalmsgcnt = 0;
   long errmsgcnt = 0;
   long nopmsgcnt = 0;
   
   int tmcount = 0;
   
   int testtime = 30;
   
//   gBenchmark->Start("GetData");

   while (1) {      
//   while (fullsz < 10000 * 6) {  
//    while (fullsz < 50) {
     if (control[boardnum].getNextData(data, 1.)) {
         
        tmcount = 0; 
         
        if (data.isEpochMsg()) {
            numepochs++;
            nummissed += data.getMissed();
//            if (data.getMissed())
//               cout << "!!!!!!!!!!!!!!!! MISSED !!!!!!!!!!!!!!! " << (int) data.getMissed() << endl;
        } else
        if (data.isSyncMsg()) {
//           if (lastsync + 8 != data.getSyncEvNum())
//              cout << "Missed sync " << data.getSyncEvNum() - lastsync << endl;
           lastsync = data.getSyncEvNum();
        }
        
        totalmsgcnt++;
        if ((data.getMessageType()==5) || (data.getMessageType()==6)) errmsgcnt++;
        if (data.getMessageType()==0) nopmsgcnt++;
        
        if (data.isSysMsg()) {
           if (data.isStopDaqMsg()) {
              if (didsuspend) {
                 cout << "StopDaq message found" << endl; 
                 break;
              }
              
           } else 
           if (data.isStartDaqMsg()) {
              if (!didsleep) { 
                 cout << "StartDaq message found" << endl; 
                 //gSystem->Sleep(5000);
                 didsleep = true;
              }
           }   
            
        }
           
            
        if (printcnt>0) {
           data.printData(3);
           printcnt--;
        }
        
        fullsz += 6;
        
        if (fullsz % 1000 == 0) {
           if (gSystem->ProcessEvents()) break;
           
           long tm = gSystem->Now();
           
           if ((tm - starttm > (testtime - 5) * 1000) && !didsuspend) {
              didsuspend = true;
              control[boardnum].suspendDaq(); 
           }
           
           if (tm - starttm > testtime*1000) break;
           
           if (tm - lasttm > 2000) {
              printcnt = 0;
              double durat = 1000./ (tm-lasttm);
              
              printf("Data rate %5.1f KB/s Epochs %5.1f 1/s   Missed %ld   Lost %5.3f %s \n", 
                    fullsz*durat * 1e-3, numepochs*durat, nummissed, control[boardnum].getLostRate()*100., "%");

              lasttm = tm;
              fullsz = 0;
              numepochs = 0;
              nummissed = 0;
           }
        }
     } else {
        printf("timeout\n");
        if (tmcount++>3) break;
     }
   }

   res = control[boardnum].stopDaq();
   
   cout << "StopDaq done " << endl;
   
   printf("Total msg:%ld err:%ld nop:%ld percent:%6.4f\n", totalmsgcnt, errmsgcnt, nopmsgcnt, 100. / (totalmsgcnt+1) * (errmsgcnt + nopmsgcnt));

   printf("Total packets RECV:%llu LOST:%llu\n", 
         control[boardnum].totalRecvPackets(), control[boardnum].totalLostPackets());

   
//   control.deleteBoards();
//   gSystem->Sleep(1000);
//   cout << "Leave mainloop" << endl;
}


void test_get(const char* boardip)
{
   SysCoreControl control;//hostIp
   int ret;
   int boardnum = control.addBoard(boardip);

   if(control.getNumberOfBoards() == 0) {
      cout << "No boards connected ... exit!" << endl;
      return;
   }
   
   if (!control.isValidBoardNum(boardnum)) {
      cout << "Cannot access board with number " << boardnum << endl;
      return;
   }
   
//   control[boardnum].setUseSorter(true);
   
   ret = control[boardnum].poke(ROC_SYNC_M_SCALEDOWN, 0);
   ret = control[boardnum].poke(ROC_AUX_ACTIVE, 1);
   ret = control[boardnum].poke(ROC_BUFFER_FLUSH_TIMER, 1000);
   
   bool res = control[boardnum].startDaq(50);
   cout << "Starting DAQ...  " << (res ? "OK" : "Fails") << endl;
   if (!res) return;

   SysCoreData data;
   long totalcnt = 0;
   
   long starttm = gSystem->Now();

   while  (control[boardnum].getNextData(data, 1.)) {
      totalcnt++; 
      
      long now = gSystem->Now();
      if ((now - starttm) > 5000) break;
   }

   res = control[boardnum].stopDaq();
   
   cout << "StopDaq done " << endl;
   
   printf("Total cnt:%ld\n", totalcnt);
}


void test(const char* boardip = "lxi009")
{
   if (strlen(boardip)==0) {
      cerr << "board ip is not defined. Exit." << endl;
      return;  
   }   
   
   cout << " BoardIp:" << boardip << endl;
   
   test_lost(boardip);
   
//   test_get(boardip);
   
}
