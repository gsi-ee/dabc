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

#ifndef ROC_Device
#include "roc/Device.h"
#endif

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#define UDP_PAYLOAD_OFFSET 42
#define MAX_UDP_PAYLOAD 1472
#define MESSAGES_PER_PACKET 243


namespace roc {


   struct UdpMessage
   {
      uint8_t  tag;
      uint8_t  pad[3];
      uint32_t password;
      uint32_t id;
      uint32_t address;
      uint32_t value;
   };

   struct UdpMessageFull : public UdpMessage
   {
      uint8_t rawdata[MAX_UDP_PAYLOAD - sizeof(UdpMessage)];
   };

   struct UdpDataRequest
   {
      uint32_t password;
      uint32_t reqpktid;
      uint32_t frontpktid;
      uint32_t tailpktid;
      uint32_t numresend;
   };

   struct UdpDataRequestFull : public UdpDataRequest
   {
      uint32_t resend[(MAX_UDP_PAYLOAD - sizeof(struct UdpDataRequest)) / 4];
   };

   struct UdpDataPacket
   {
      uint32_t pktid;
      uint32_t lastreqid;
      uint32_t nummsg;
   // here all messages should follow
   };

   struct UdpDataPacketFull : public UdpDataPacket
   {
      uint8_t msgs[MAX_UDP_PAYLOAD - sizeof(struct UdpDataPacket)];
   };

   struct UdpStatistic {
      uint32_t   dataRate;   // data taking rate in  B/s
      uint32_t   sendRate;   // network send rate in B/s
      uint32_t   recvRate;   // network recv rate in B/s
      uint32_t   nopRate;    // double-NOP messages 1/s
      uint32_t   frameRate;  // unrecognized frames 1/s
      uint32_t   takePerf;   // relative use of time for data taking (*100000)
      uint32_t   dispPerf;   // relative use of time for packets dispatching (*100000)
      uint32_t   sendPerf;   // relative use of time for data sending (*100000)
   };


   class UdpDevice;

   class UdpControlSocket : public dabc::SocketIOProcessor {
      friend class UdpDevice;
      protected:

         enum EUdpEvents { evntSendCtrl = evntSocketLast + 1 };

         UdpDevice*  fDev;
         UdpMessageFull   fRecvBuf;
      public:
         UdpControlSocket(UdpDevice* dev, int fd);
         virtual ~UdpControlSocket();

         virtual void ProcessEvent(dabc::EventId evnt);

         virtual bool OnRecvProvideBuffer();
         virtual void OnRecvCompleted();
   };

   class UdpDevice : public roc::Device {
      friend class UdpControlSocket;

      protected:
         bool              fConnected;
         UdpControlSocket* fCtrl;
         dabc::Condition   fCond;

         int               ctrlState_;
         UdpMessageFull    controlSend;
         unsigned          controlSendSize; // size of control data to be send
         UdpMessageFull    controlRecv;

         uint32_t currentMessagePacketId;

         UdpStatistic      brdStat;    // last available statistic block
         bool              isBrdStat;  // is block statistic contains valid data

         bool              displayConsoleOutput_; // display output, comming from ROC

         bool performCtrlLoop(double total_tmout_sec, bool show_progress);
         void processCtrlMessage(UdpMessageFull* pkt, unsigned len);
         void setBoardStat(void* rawdata, bool print);

      public:

         UdpDevice(dabc::Basic* parent, const char* name, const char* thrdname, dabc::Command* cmd);
         virtual ~UdpDevice();

         bool IsConnected() const { return fConnected; }

         virtual const char* ClassName() const { return typeUdpDevice; }

         virtual const char* RequiredThrdClass() const { return dabc::typeSocketThread; }

         virtual int CreateTransport(dabc::Command* cmd, dabc::Port* port);

         virtual bool poke(uint32_t addr, uint32_t value);
         virtual uint32_t peek(uint32_t addr);
   };

}

#endif
