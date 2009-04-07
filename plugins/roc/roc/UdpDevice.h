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

#ifndef ROC_UdpDevice
#define ROC_UdpDevice

#ifndef DABC_Device
#include "dabc/Device.h"
#endif

#ifndef ROC_UdpBoard
#include "roc/UdpBoard.h"
#endif

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#ifndef ROC_UdpTransport
#include "roc/UdpTransport.h"
#endif

namespace roc {

   class UdpDevice;

   class UdpControlSocket : public dabc::SocketIOProcessor {
      friend class UdpDevice;
      protected:

         enum ECtrlState { ctrlReady, ctrlLocked, ctrlWaitReply, ctrlGotReply, ctrlTimedout };

         enum EUdpEvents { evntDoCtrl = evntSocketLast,  };

         UdpDevice*      fDev;

         dabc::Mutex     fControlMutex;
         ECtrlState      fCtrlState;
         dabc::Condition fControlCond;
         dabc::Command*  fControlCmd;
         UdpMessageFull  fControlSend;
         unsigned        fControlSendSize;
         UdpMessageFull  fControlRecv;
         UdpMessageFull *fControlRes;
         uint32_t        fPacketCounter;

         double          fTotalTmoutSec;
         bool            fShowProgress;
         bool            fFastMode;
         int             fLoopCnt;

      public:
         UdpControlSocket(UdpDevice* dev, int fd);
         virtual ~UdpControlSocket();

         virtual double ProcessTimeout(double last_diff);

         virtual void ProcessEvent(dabc::EventId evnt);

         bool completeLoop(bool res);

         bool lockCtrlLoop();

         bool startCtrlLoop(dabc::Command* cmd);

         bool doCtrlLoop();
   };


   class UdpDevice : public dabc::Device,
                     public roc::UdpBoard {

      enum EUdpDeviceEvents { eventCheckUdpCmds = eventDeviceLast,
                              eventUdpDeviceLast };


      friend class UdpControlSocket;
      friend class UdpDataSocket;

      protected:
         bool              fConnected;

         std::string       fRocIp;

         dabc::CommandsQueue  fUdpCmds;

         int               fCtrlPort;
         UdpControlSocket *fCtrlCh;

         int               fDataPort;
         UdpDataSocket    *fDataCh;

         UdpMessageFull    controlRecv;

         UdpStatistic      brdStat;    // last available statistic block
         bool              isBrdStat;  // is block statistic contains valid data

         bool              displayConsoleOutput_; // display output, coming from ROC


         void processCtrlMessage(UdpMessageFull* pkt, unsigned len);
         void setBoardStat(void* rawdata, bool print);

         bool pokeRawData(uint32_t address, const void* rawdata, uint32_t rawdatelen, double tmout = 2.);
         int parseBitfileHeader(char* pBuffer, unsigned int nLen);
         uint8_t calcBinXOR(const char* filename);
         bool uploadDataToRoc(char* buf, unsigned len);

         virtual bool initialise(BoardRole role);
         virtual void* getdeviceptr() { return this; }

         virtual int ExecuteCommand(dabc::Command* cmd);

         virtual void ProcessEvent(dabc::EventId evnt);

         void ProcessNextUdpCommand();

         void preparePeek(uint32_t addr, double tmout);
         void preparePoke(uint32_t addr, uint32_t value, double tmout);

      public:

         UdpDevice(dabc::Basic* parent, const char* name, const char* thrdname, dabc::Command* cmd);
         virtual ~UdpDevice();

         bool IsConnected() const { return fConnected; }

         virtual const char* ClassName() const { return typeUdpDevice; }

         virtual const char* RequiredThrdClass() const { return dabc::typeSocketThread; }


         virtual int CreateTransport(dabc::Command* cmd, dabc::Port* port);

         // interface part of roc::UdpBoard

         virtual bool poke(uint32_t addr, uint32_t value, double tmout = 5.);
         virtual uint32_t peek(uint32_t addr, double tmout = 5.);

         virtual bool sendConsoleCommand(const char* cmd);

         virtual bool uploadBitfile(const char* filename, int position);

         virtual bool uploadSDfile(const char* filename, const char* sdfilename = 0);

         virtual bool saveConfig(const char* filename = 0);
         virtual bool loadConfig(const char* filename = 0);
   };

}

#endif
