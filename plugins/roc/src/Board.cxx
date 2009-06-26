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

void roc::Board::ShowOperRes(const char* oper, uint32_t addr, uint32_t value, int res)
{
   DOUT2(("Roc:%u %s(0x%04x, 0x%04x) res = %d", fRocNumber, oper, addr, value, res));

   if (res!=0) {
      const char* errcode = "Unknown error code";
      switch (res) {
         case 1: errcode = "Readback failed (different value)"; break;
         case 2: errcode = "Wrong / unexisting address"; break;
         case 3: errcode = "Wrong value"; break;
         case 4: errcode = "Permission denied (to low level addresses)"; break;
         case 5: errcode = "The called function needs longer to run"; break;
         case 6: errcode = "Some slow control udp-data packets got lost"; break;
         case 7: errcode = "No Response (No Ack) Error on I2C bus"; break;
         case 8: errcode = "Operation not allowed - probably, somebody else uses ROC"; break;
         case 9: errcode = "Operation failed"; break;
      }

      DOUT0(("\033[91m Error Roc:%u %s(0x%04x, 0x%04x) res = %d %s \033[0m", fRocNumber, oper, addr, value, res, errcode));
   }
}


const char* roc::Board::VersionToStr(uint32_t ver)
{
   static char sbuf[100];
   sprintf(sbuf,"%u.%u.%u.%u", ver >> 24, (ver >> 16) & 0xff, (ver >> 8) & 0xff, ver & 0xff);
   return sbuf;
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

void roc::Board::setParityCheck(uint32_t val)
{
   put(ROC_PARITY_CHECK, val);
}

void roc::Board::RESET()
{
   put(ROC_SYSTEM_RESET, 1);
}

void roc::Board::DEBUG_MODE(uint32_t val)
{
   put(ROC_DEBUG_MODE, val);
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
   if (get(ROC_NX_ACTIVE, mask)!=0) mask = 0;
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


//-------------------------------------------------------------------------------
uint32_t roc::Board::getTHROTTLE()
{
   uint32_t mask(0);
   get(ROC_THROTTLE, mask);
   return mask;
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
uint32_t roc::Board::GPIO_getCONFIG()
{
   uint32_t mask(0);
   get(ROC_GPIO_CONFIG, mask);
   return mask;
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
void roc::Board::GPIO_setCONFIG(uint32_t mask)
{
   put(ROC_GPIO_CONFIG, mask);
}
//-------------------------------------------------------------------------------


bool roc::Board::GPIO_packCONFIG(uint32_t& mask, int gpio_nr, int additional, int throttling, int falling, int rising)
{
   if ((gpio_nr<2)||(gpio_nr>7)) return false;

   if ((gpio_nr==2)||(gpio_nr==3)) {
     rising = falling | rising;
     falling = 0;
   }

   mask = mask & (~(0xf<<(4*gpio_nr)));

   uint32_t submask = ((additional ? 8 : 0)|(throttling ? 4 : 0)|(falling ? 2: 0)|(rising ? 1 : 0));

   mask = mask | ( submask << (4*gpio_nr));

   return true;
}

bool roc::Board::GPIO_unpackCONFIG(uint32_t mask, int gpio_nr, int& additional, int& throttling, int& falling, int& rising)
{
   if ((gpio_nr<2)||(gpio_nr>7)) return false;

   uint32_t submask = (mask >> (4*gpio_nr)) & 0xf;

   additional = (submask & 8) ? 1 : 0;
   throttling = (submask & 4) ? 1 : 0;
   falling = (submask & 2) ? 1 : 0;
   rising  = (submask & 1) ? 1 : 0;

   if ((gpio_nr==2)||(gpio_nr==3)) {
     rising = falling | rising;
     falling = 0;
   }

   return true;
}

//-------------------------------------------------------------------------------
void roc::Board::GPIO_setCONFIG(int gpio_nr, int additional, int throttling, int falling, int rising)
{
   uint32_t mask = GPIO_getCONFIG();

   if (GPIO_packCONFIG(mask, gpio_nr, additional, throttling, falling, rising))
      GPIO_setCONFIG(mask);
}
//-------------------------------------------------------------------------------




//-------------------------------------------------------------------------------
void roc::Board::GPIO_setBAUD(int gpio_nr, uint32_t BAUD_START, uint32_t BAUD1, uint32_t BAUD2)
{
   if (gpio_nr==1) {
      put(ROC_SYNC1_BAUD_START, BAUD_START);
      put(ROC_SYNC1_BAUD1, BAUD1);
      put(ROC_SYNC1_BAUD2, BAUD2);
   } else if (gpio_nr==2) {
      put(ROC_SYNC2_BAUD_START, BAUD_START);
      put(ROC_SYNC2_BAUD1, BAUD1);
      put(ROC_SYNC2_BAUD2, BAUD2);
   } else if (gpio_nr==3) {
      put(ROC_SYNC3_BAUD_START, BAUD_START);
      put(ROC_SYNC3_BAUD1, BAUD1);
      put(ROC_SYNC3_BAUD2, BAUD2);
   }
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
void roc::Board::GPIO_setMHz(int gpio_nr, int MHz)
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

   GPIO_setBAUD (gpio_nr, Start, Baud1, Baud2);
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
void roc::Board::GPIO_setScaledown(uint32_t val)
{
   put(ROC_SYNC1_M_SCALEDOWN, val);
}
//-------------------------------------------------------------------------------


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
