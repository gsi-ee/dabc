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

#include "roc/Board.h"

#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/Factory.h"
#include "dabc/Device.h"

#include "roc/defines.h"
#include "roc/ReadoutModule.h"


const char* roc::xmlNumRocs         = "NumRocs";
const char* roc::xmlRocPool         = "RocPool";
const char* roc::xmlTransportWindow = "TransportWindow";
const char* roc::xmlBoardIP         = "BoardIP";
const char* roc::xmlRole            = "Role";

const char* roc::typeUdpDevice      = "roc::UdpDevice";


roc::Board::Board() :
   fRole(roleNone),
   fRocNumber(0),
   fDefaultTimeout(2.),
   fReadout(0)
{
}

roc::Board::~Board()
{
}

roc::Board* roc::Board::Connect(const char* name, BoardRole role)
{
   dabc::SetDebugLevel(0);

   if (dabc::mgr()==0)
      if (!dabc::Factory::CreateManager()) return 0;

   const char* devname = "RocUdp";

   dabc::Command* cmd = new dabc::CmdCreateDevice(typeUdpDevice, devname);
   cmd->SetStr(roc::xmlBoardIP, name);
   cmd->SetInt(roc::xmlRole, role);
   if (!dabc::mgr()->Execute(cmd)) return 0;

   dabc::Device* dev = dabc::mgr()->FindDevice(devname);
   if (dev==0) return 0;

   std::string sptr = dev->ExecuteStr("GetBoardPtr", "BoardPtr");
   void *ptr = 0;

   if (sptr.empty() || (sscanf(sptr.c_str(),"%p", &ptr) != 1)) {
      dev->DestroyProcessor();
      return 0;
   }

   roc::Board* brd = (roc::Board*) ptr;

   if (role == roleDAQ) {
      roc::ReadoutModule* m = new roc::ReadoutModule("RocReadout");
      dabc::mgr()->MakeThreadForModule(m, "ReadoutThrd");

      if (!dabc::mgr()->CreateTransport("RocReadout/Input", devname)) {
         EOUT(("Cannot connect readout module to device %s", devname));
         dabc::mgr()->DeleteModule("RocReadout");
      }

      brd->SetReadout(m);

      dabc::lgr()->SetLogLimit(1000000);
   }

   return brd;
}

bool roc::Board::Close(Board* brd)
{
   if (brd==0) return false;
   dabc::Device* dev = (dabc::Device*) brd->getdeviceptr();
   if (dev==0) return false;

   brd->SetReadout(0);
   dabc::mgr()->DeleteModule("RocReadout");

   if (!dev->Execute("Disconnect", 4.)) return false;
   dev->DestroyProcessor();
   return true;
}



void roc::Board::setROC_Number(uint32_t num)
{
   put(ROC_NUMBER, num);
}

uint32_t roc::Board::getROC_Number()
{
   uint32_t num = 0;
   get(ROC_NUMBER, num);
   return num;
}

uint32_t roc::Board::getSW_Version()
{
   return KNUT_VERSION;
}

uint32_t roc::Board::getHW_Version()
{
   uint32_t val = 0;
   get(ROC_HARDWARE_VERSION, val);
   return val;
}

void roc::Board::reset_FIFO()
{
   put(ROC_FIFO_RESET, 1);
   put(ROC_FIFO_RESET, 0);
   reset_MISS();
}

uint32_t roc::Board::getFIFO_empty()
{
   uint32_t val;
   get(ROC_NX_FIFO_EMPTY, val);
   return val;
}

uint32_t roc::Board::getFIFO_full()
{
   uint32_t val;
   get(ROC_NX_FIFO_FULL, val);
   return val;
}

uint32_t roc::Board::getMISS()
{
   uint32_t val;
   get(ROC_NX_MISS, val);
   return val;
}

void roc::Board::reset_MISS()
{
   put(ROC_NX_MISS_RESET, 1);
   put(ROC_NX_MISS_RESET, 0);
}

void roc::Board::setParityCheck(uint32_t val)
{
   put(ROC_PARITY_CHECK, val);
}

void roc::Board::RESET()
{
   put(ROC_SYSTEM_RESET, 1);
}

void roc::Board::TestPulse(uint32_t length, uint32_t number)
{
   put(ROC_TESTPULSE_LENGTH, length - 1);
   put(ROC_TESTPULSE_NUMBER, number * 2);
   put(ROC_TESTPULSE_START, 1);
}

void roc::Board::TestPulse(uint32_t delay, uint32_t length, uint32_t number)
{
   put(ROC_TESTPULSE_RESET_DELAY, delay  - 1);
   put(ROC_TESTPULSE_LENGTH, length - 1);
   put(ROC_TESTPULSE_NUMBER, number * 2);
   NX_reset();
}

void roc::Board::NX_reset()
{
   put(ROC_NX_INIT, 0);
   put(ROC_NX_RESET, 0);
   put(ROC_NX_RESET, 1);
   put(ROC_NX_INIT, 1);
   put(ROC_NX_INIT, 0);
}


void roc::Board::NX_setActive(uint32_t mask)
{
   put(ROC_NX_ACTIVE, mask & 0xf);
}

uint32_t roc::Board::NX_getActive()
{
   uint32_t mask(0);
   if (!get(ROC_NX_ACTIVE, mask)) return 0;
   return mask;
}


void roc::Board::NX_setActive(int nx1, int nx2, int nx3, int nx4)
{
   NX_setActive((nx1 ? 1 : 0) | (nx2 ? 2 : 0) | (nx3 ? 4 : 0) | (nx4 ? 8 : 0));
}


void roc::Board::setLTS_Delay(uint32_t val)
{
   put(ROC_DELAY_LTS, val);
}


void roc::Board::GPIO_setActive(uint32_t mask)
{
   put(ROC_AUX_ACTIVE, mask);
}

uint32_t roc::Board::GPIO_getActive()
{
   uint32_t mask(0);
   if (get(ROC_AUX_ACTIVE, mask)!=0) return 0;
   return mask;
}


void roc::Board::GPIO_setActive(int gpio1, int gpio2, int gpio3, int gpio4)
{
   uint32_t mask = 0;

   if (gpio1>0) mask |= 1;
   if (gpio2>0) mask |= 2;
   if (gpio3>0) mask |= 4;
   if (gpio4>0) mask |= 8;

   GPIO_setActive(mask);

}

void roc::Board::GPIO_getActive(int& gpio1, int& gpio2, int& gpio3, int& gpio4)
{
   uint32_t mask = GPIO_getActive();

   gpio1 = (mask & 1) ? 1 : 0;
   gpio2 = (mask & 2) ? 1 : 0;
   gpio3 = (mask & 4) ? 1 : 0;
   gpio4 = (mask & 8) ? 1 : 0;
}

void roc::Board::GPIO_setBAUD(uint32_t BAUD_START, uint32_t BAUD1, uint32_t BAUD2)
{
   put(ROC_SYNC_BAUD_START, BAUD_START);
   put(ROC_SYNC_BAUD1, BAUD1);
   put(ROC_SYNC_BAUD2, BAUD2);
}

void roc::Board::GPIO_setMHz(int MHz)
{
   int Baud1, Baud2, Start;
   float realMHz;

   if (MHz>65) {
      EOUT(("setMHz: %d is not a valid MHz value!\n"));
      return;
   }

   Baud1 = int(250. / float(MHz) + 0.5);
   Baud2 = int(500. / float(MHz) + 0.5) - Baud1;
   Start = Baud2 / 2;
   if (Start<=2) Start=2;

   DOUT0(("Start: %d Baud1: %d Baud2: %d\n", Start, Baud1, Baud2));

   realMHz = 250. / (((float)Baud1 + (float)Baud2) / 2);

   DOUT0(("The set Baudrate corresponds %7.3f MHz.\n", realMHz));
   if (realMHz!=MHz)
      DOUT0(("The differnece to the passed Parameter is caused by the hardware restrictions. \n"));

   GPIO_setBAUD (Start, Baud1, Baud2);
}


void roc::Board::GPIO_setScaledown(uint32_t val)
{
   put(ROC_SYNC_M_SCALEDOWN, val);
}

void roc::Board::setUseSorter(bool on)
{
   if (fReadout) fReadout->setUseSorter(on);
}


bool roc::Board::startDaq(unsigned)
{
   DOUT2(("Starting DAQ !!!!"));

   dabc::Device* dev = (dabc::Device*) getdeviceptr();

   if ((fReadout==0) || (dev==0)) return false;

   if (!dev->Execute("PrepareStartDaq", 5)) return false;

   fReadout->Start();

   return true;
}

bool roc::Board::suspendDaq()
{
   DOUT2(("Suspend DAQ !!!!"));

   dabc::Device* dev = (dabc::Device*) getdeviceptr();
   if (dev==0) return false;

   if (!dev->Execute("PrepareSuspendDaq", 5.)) return false;

   return true;
}

bool roc::Board::stopDaq()
{
   DOUT2(("Stop DAQ !!!!"));

   dabc::Device* dev = (dabc::Device*) getdeviceptr();
   if ((dev==0) || (fReadout==0)) return false;

   if (!dev->Execute("PrepareStopDaq", 5)) return false;

   fReadout->Stop();

   return true;
}

bool roc::Board::getNextData(nxyter::Data& data, double tmout)
{
   return fReadout ? fReadout->getNextData(data, tmout) : false;
}
