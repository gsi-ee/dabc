//============================================================================
// Name        : rocsrc/Test.cpp
// Author      : Norbert Abel
// Copyright   : Kirchhoff-Institut für Physik
//============================================================================

#include "roc/Test.h"

#include "roc/FEBs.h"

#include "roc/defines.h"
#include "dabc/logging.h"

#include "roc/UdpBoard.h"

#include <iostream>
using namespace std;

//-------------------------------------------------------------------------------
roc::Test::Test(roc::Board* board) : Calibrate(board)
{
   fC19=0;
   fC20=0;
   fFebn=0;
   fNxn[0]=0;
   fNxn[1]=0;
   fDataOK=false;
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
roc::Test::~Test()
{
}
//-------------------------------------------------------------------------------




//-------------------------------------------------------------------------------
bool roc::Test::FULL_TEST()
{
   if (ROC_HW_Test())
      if (Search_and_Connect_FEB())
         if (ADC_SPI_Test()) Full_Data_Test();

   return Summary();
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
bool roc::Test::Persistence()
{
   roc::UdpBoard* udp = dynamic_cast<roc::UdpBoard*> (fBoard);
   if (udp==0) {
      EOUT(("Cannot store - no udp channel found"));
      return false;
   }

   string input;

   DOUT0((""));
   DOUT0(("----------------------------------------------------------"));
   DOUT0(("--                      PERSISTENCE                     --"));
   DOUT0(("----------------------------------------------------------"));
   DOUT0((""));
   DOUT0(("Save measured values on ROC's SD-Card"));
   udp->saveConfig();
   DOUT0(("Data stored."));
   return true;
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
bool roc::Test::Summary()
{
   DOUT0((""));
   DOUT0(("----------------------------------------------------------"));
   DOUT0(("--                       SUMMARY                        --"));
   DOUT0(("----------------------------------------------------------"));
   DOUT0((""));

   if (fDataOK==true)
      DOUT0(("All test were successful."));
   else
      EOUT(("The test routines returned an error."));

   DOUT0(("The following FEBs were detected:"));
   DOUT0(("CON19: "));
   if (fC19==0) DOUT0(("    No Board"));
   if (fC19==1) DOUT0(("    FEB1nxC"));
   if (fC19==2) DOUT0(("    FEB2nx"));
   if (fC19==4) DOUT0(("    FEB4nx"));
   DOUT0(("CON20: "));
   if (fC20==0) DOUT0(("    No Board"));
   if (fC20==1) DOUT0(("    FEB1nxC"));
   if (fC20==2) DOUT0(("    FEB2nx"));
   if (fC19==4) DOUT0(("    FEB4nx"));
   DOUT0((""));

   if (fDataOK==true) {
      DOUT0(("Autolatency and Autodelay could be performed."));
      DOUT0(("You can find the detected values above."));
   } else
      EOUT(("Autolatency and Autodelay could NOT be performed."));
   return fDataOK;
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
bool roc::Test::Full_Data_Test()
{
   int feb, nx;
   uint32_t threshold, val;

   DOUT0((""));

   for (feb=0; feb<fFebn; feb++) {
     for (nx=0; nx<fNxn[feb]; nx++) {
         prepare(feb, nx);
         FEB(feb).ADC().setRegister(0x0D, 0x48);
         FEB(feb).ADC().setRegister(0x19, 0xA5);
         FEB(feb).ADC().setRegister(0x1A, 0x5A);
         FEB(feb).ADC().setRegister(0x1B, 0xA5);
         FEB(feb).ADC().setRegister(0x1C, 0x5A);
         FEB(feb).NX(nx).I2C().setRegister(32, 1);
         cout << endl;
         cout << "----------------------------------------------------------" << endl;
         cout << "-- Performing Full Data Test for nXYTER #" << FEB(feb).NX(nx).getNumber() << " on " << ((FEB(feb).ADC().getConnector()==CON19)?"CON19":"CON20") << endl;
         cout << "----------------------------------------------------------" << endl;
         if (verbose()) DOUT0(("Searching for a good threshold (Reg18)... "));
         for (threshold=100; threshold>0; threshold--){
             if (verbose()) std::cout << threshold << " " << std::flush;
             FEB(feb).NX(nx).I2C().setRegister(18, threshold);
             usleep(10000);
             brd().reset_FIFO();
             brd().TestPulse (2000, 5000);
             usleep(10000);
             if (brd().getFIFO_full()) break;
         }
         cout << endl;
         if (threshold==0) {
            cout << "No response to any Threshold!" << endl;
            cout << "nXYTER Data Test: FAILED!" << endl;
            return false;
         }
         brd().get(ROC_ADC_DATA, val);
         if (val!=0xAA5) {
            printf("Wrong ADC Data! Expected Testpattern 0xAA5 - Received Pattern %X.\r\n", val);
            cout << "nXYTER Data Test: FAILED!" << endl;
            return false;
         }
         FEB(feb).ADC().setRegister(0x0D, 0x0);
         FEB(feb).NX(nx).I2C().setRegister(18, threshold);
         brd().reset_FIFO();
         if (autolatency(6, 9)) {
            cout << "Unable to perform an autolatency!" << endl;
            cout << "nXYTER Data Test: FAILED!" << endl;
            return false;
         }
         if (autodelay()) {
            cout << "Unable to perform an autodelay!" << endl;
            cout << "nXYTER Data Test: FAILED!" << endl;
            return false;
         }
         FEB(feb).NX(nx).I2C().setRegister(32, 0);
     }
   }
   cout << "nXYTER Data Test: PASSED." << endl;
   fDataOK=true;
   return true;
}
//-------------------------------------------------------------------------------





//-------------------------------------------------------------------------------
bool roc::Test::ADC_SPI_Test()
{
   int feb;

   cout << endl;
   for (feb=0; feb<fFebn; feb++) {
      if (FEB(feb).ADC().getRegister(1)!=2) {
        cout << "ERROR: Read wrong ADC ID on Connector " << ((FEB(feb).ADC().getConnector()==CON19)?"CON19":"CON20") << endl << "ADC Read Check: FAILURE!" << endl;
        return false;
      }
   }
   cout << "Read from ADC SPI: PASSED." << endl;


   for (feb=0; feb<fFebn; feb++) {
      FEB(feb).ADC().setRegister(0xD, 0x48);
      if (FEB(feb).ADC().getRegister(0xD)!=0x48) {
        cout << "ERROR: Readback of ADC register 0xD failed on " << ((FEB(feb).ADC().getConnector()==CON19)?"CON19":"CON20") << endl << "ADC Write/Readback Check: FAILURE!" << endl;
        return false;
      }
      FEB(feb).ADC().setRegister(0xD, 0);
   }
   cout << "Write to & Readback from ADC SPI: PASSED." << endl;
   return true;
}
//-------------------------------------------------------------------------------





//-------------------------------------------------------------------------------
bool roc::Test::Search_and_Connect_FEB(){

   // Search for nXYTERs via I2C:
    cout << endl;
    cout << "nXYTER I2C Check..." << endl;

    if (!I2C_Test(CON19, 8)) {
       cout << "No Board detected on CON19!" << endl;
       fC19 = 0;
    } else if (!I2C_Test(CON19, 9)){
       cout << "Detected FEB1nx on CON19." << endl;
       fC19 = 1;
    } else if (!I2C_Test(CON19, 10)){
       cout << "Detected FEB2nx on CON19." << endl;
       fC19 = 2;
    } else {
       cout << "Detected FEB4nx." << endl;
       fC19 = 4;
    }

    if (fC19 != 4) {
       if (!I2C_Test(CON20, 8)) {
          cout << "No Board detected on CON20!" << endl;
          fC20 = 0;
       } else if (!I2C_Test(CON20, 9)){
          cout << "Detected FEB1nx on CON20." << endl;
          fC20 = 1;
       } else if (!I2C_Test(CON20, 10)){
          cout << "Detected FEB2nx on CON20." << endl;
          fC20 = 2;
       } else {
          cout << "ERROR: Detected FEB4nx I2C on Con20. Please exchange the Connectors!" << endl;
          return -1;
       }
    }

    if ((fC19==0)&&(fC20==0)) {
       cout << "nXYTER I2C Check: FAILED!" << endl;
       return false;
    }
    cout << "nXYTER I2C Check: PASSED." << endl;





   // Connect to the detected Boards:
   cout << endl;

   fFebn=0;
   fNxn[0]=0;
   fNxn[1]=0;

   if (fC19==1) {
      addFEB(new roc::FEB1nxC(fBoard, CON19));
      fNxn[fFebn++]=1;
   }
   if (fC19==2) {
      addFEB(new roc::FEB2nx(fBoard, CON19));
      fNxn[fFebn++]=2;
   }
   if (fC19==4) {
      addFEB(new roc::FEB4nx(fBoard));
      fNxn[fFebn++]=4;
   }

   if (fC20==1) {
      addFEB(new roc::FEB1nxC(fBoard, CON20));
      fNxn[fFebn++]=1;
   }
   if (fC20==2) {
      addFEB(new roc::FEB2nx(fBoard, CON20));
      fNxn[fFebn++]=2;
   }
   return true;
}
//-------------------------------------------------------------------------------





//-------------------------------------------------------------------------------
bool roc::Test::ROC_HW_Test()
{
   uint32_t val=0;

   DOUT0((""));

   roc::UdpBoard* udp = dynamic_cast<roc::UdpBoard*> (fBoard);
   if (udp) udp->setConsoleOutput(true);

   brd().get(ROC_HARDWARE_VERSION, val);
   if((val >= 0x01080000) || (val < 0x01070000)) {
       printf("The ROC you want to access has the wrong hardware version: %X\r\n", val);
       cout << "This C++ access class only supports boards with major version 1.7 in Software and Hardware." << endl;
       cout << "ROC Hardware Check: FAILED!" << endl;
       return false;
    }
    DOUT0(("ROC Hardware Check: PASSED."));
    return true;
}
//-------------------------------------------------------------------------------





//-------------------------------------------------------------------------------
bool roc::Test::I2C_Test(int Connector, int SlaveAddr)
{
    uint32_t val=0, err=0, err2=0;

    if (Connector==CON19) {
       brd().put(ROC_I2C1_RESET, 0);
       brd().put(ROC_I2C1_REGRESET, 0);
       brd().put(ROC_I2C1_RESET, 1);
       brd().put(ROC_I2C1_REGRESET, 1);

       brd().put(ROC_I2C1_SLAVEADDR, SlaveAddr);
       brd().put(ROC_I2C1_REGISTER, 0);
       brd().put(ROC_I2C1_DATA, 0xA5);
       brd().get(ROC_I2C1_ERROR, err);
       brd().get(ROC_I2C1_DATA, val);
       brd().get(ROC_I2C1_ERROR, err2);
    } else {
       brd().put(ROC_I2C2_RESET, 0);
       brd().put(ROC_I2C2_REGRESET, 0);
       brd().put(ROC_I2C2_RESET, 1);
       brd().put(ROC_I2C2_REGRESET, 1);

       brd().put(ROC_I2C2_SLAVEADDR, SlaveAddr);
       brd().put(ROC_I2C2_REGISTER, 0);
       brd().put(ROC_I2C2_DATA, 0xA5);
       brd().get(ROC_I2C2_ERROR, err);
       brd().get(ROC_I2C2_DATA, val);
       brd().get(ROC_I2C2_ERROR, err2);
    }

    if ((err!=0)||(val!=0xA5)||(err2!=0)) {
       return false;
    }
    return true;
}
//-------------------------------------------------------------------------------
