// $Id$

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

#ifndef HADAQ_UDPTRANSPORT_H
#define HADAQ_UDPTRANSPORT_H

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

 #ifndef DABC_Pointer
#include "dabc/Pointer.h"
#endif

#ifndef DABC_DataTransport
#include "dabc/DataTransport.h"
#endif

#ifndef HADAQ_HadaqTypeDefs
#include "hadaq/HadaqTypeDefs.h"
#endif

namespace hadaq {

   class DataTransport;
   class NewTransport;
   class TerminalModule;

   struct TransportInfo {
      int                fNPort;           ///< upd port number

      uint64_t           fTotalRecvPacket;
      uint64_t           fTotalDiscardPacket;
      uint64_t           fTotalDiscard32Packet;
      uint64_t           fTotalArtificialLosts;
      uint64_t           fTotalArtificialSkip;
      uint64_t           fTotalRecvBytes;
      uint64_t           fTotalDiscardBytes;
      uint64_t           fTotalProducedBuffers;

      void ClearCounters()
      {
         fTotalRecvPacket = 0;
         fTotalDiscardPacket = 0;
         fTotalDiscard32Packet = 0;
         fTotalArtificialLosts = 0;
         fTotalArtificialSkip = 0;
         fTotalRecvBytes = 0;
         fTotalDiscardBytes = 0;
         fTotalProducedBuffers = 0;
      }

      TransportInfo(int port) : fNPort(port) { ClearCounters(); }

      std::string GetDiscardString()
      {
         std::string res = dabc::number_to_str(fTotalDiscardPacket);

         if (fTotalArtificialLosts > 0)
            res = std::string("*") + dabc::number_to_str(fTotalArtificialLosts);

         return res;
      }

      std::string GetDiscard32String()
      {

         std::string res = dabc::number_to_str(fTotalDiscard32Packet);

         if (fTotalArtificialSkip > 0)
            res = std::string("#") + dabc::number_to_str(fTotalArtificialSkip);

         return res;
      }

   };

   // ==================================================================================

   class NewAddon : public dabc::SocketAddon,
                    public TransportInfo {
      protected:

         friend class TerminalModule;  // use only to access statistic, nothing else
         friend class NewTransport;

         dabc::Pointer      fTgtPtr;          ///< pointer used to read data
         unsigned           fMTU;             ///< maximal size of packet expected from TRB
         void*              fMtuBuffer;       ///< buffer used to skip packets when no normal buffer is available
         int                fSkipCnt;         ///< counter used to control buffers skipping
         int                fSendCnt;         ///< counter of send buffers since last timeout active
         int                fMaxLoopCnt;      ///< maximal number of UDP packets, read at once
         double             fReduce;          ///< reduce filled buffer size to let reformat data later
         double             fLostRate;        ///< artificial lost of received UDP packets
         int                fLostCnt;         ///< counter used to drop buffers
         bool               fDebug;           ///< when true, produce more debug output
         bool               fRunning;         ///< is transport running
         dabc::TimeStamp    fLastProcTm;      ///< last time when udp reading was performed
         double             fMaxProcDist;     ///< maximal time between calls to BuildEvent method

         virtual void ProcessEvent(const dabc::EventId&);

         /** Light-weight command interface */
         virtual long Notify(const std::string&, int);

         /* Use codes which are valid for Read_Start */
         bool ReadUdp();

         bool CloseBuffer();

      public:
         NewAddon(int fd, int nport, int mtu, bool debug, int maxloop, double reduce, double lost);
         virtual ~NewAddon();

         bool HasBuffer() const { return !fTgtPtr.null(); }

         static int OpenUdp(const std::string &host, int nport, int rcvbuflen);
   };

   // ==================================================================================

   class NewTransport : public dabc::Transport {

      protected:

         int            fIdNumber;     ///< input port item id
         unsigned       fNumReadyBufs; ///< number of filled buffers which could be posted
         bool           fBufAssigned;  ///< if next buffer assigned
         int            fLastSendCnt;  ///< used for flushing
         dabc::TimeStamp fLastDebugTm;     ///< timer used to generate rare debugs output

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int ExecuteCommand(dabc::Command cmd);

      public:
         NewTransport(dabc::Command, const dabc::PortRef& inpport, NewAddon* addon, double flush = 1);
         virtual ~NewTransport();

         /** Methods activated by Port, when transport starts/stops. */
         virtual bool StartTransport();
         virtual bool StopTransport();

         virtual bool ProcessSend(unsigned port);
         virtual bool ProcessBuffer(unsigned pool);

         bool AssignNewBuffer(unsigned pool, NewAddon* addon = nullptr);
         void BufferReady();
         void FlushBuffer(bool onclose = false);

   };

}

#endif
