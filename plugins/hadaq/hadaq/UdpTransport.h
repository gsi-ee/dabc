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
      int                fNPort{0};           ///< upd port number

      uint64_t           fTotalRecvPacket{0};
      uint64_t           fTotalDiscardPacket{0};
      uint64_t           fTotalDiscard32Packet{0};
      uint64_t           fTotalArtificialLosts{0};
      uint64_t           fTotalArtificialSkip{0};
      uint64_t           fTotalRecvBytes{0};
      uint64_t           fTotalDiscardBytes{0};
      uint64_t           fTotalProducedBuffers{0};

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

         dabc::Pointer      fTgtPtr;             ///< pointer used to read data
         unsigned           fMTU{0};             ///< maximal size of packet expected from TRB
         void*              fMtuBuffer{nullptr}; ///< buffer used to skip packets when no normal buffer is available
         int                fSkipCnt{0};         ///< counter used to control buffers skipping
         int                fSendCnt{0};         ///< counter of send buffers since last timeout active
         int                fMaxLoopCnt{0};      ///< maximal number of UDP packets, read at once
         double             fReduce{0};          ///< reduce filled buffer size to let reformat data later
         double             fLostRate{0};        ///< artificial lost of received UDP packets
         int                fLostCnt{0};         ///< counter used to drop buffers
         bool               fDebug{false};       ///< when true, produce more debug output
         bool               fRunning{false};     ///< is transport running
         dabc::TimeStamp    fLastProcTm;         ///< last time when udp reading was performed
         double             fMaxProcDist{0};     ///< maximal time between calls to BuildEvent method

         void ProcessEvent(const dabc::EventId&) override;

         /** Light-weight command interface */
         long Notify(const std::string&, int) override;

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

         int            fIdNumber{0};         ///< input port item id
         unsigned       fNumReadyBufs{0};     ///< number of filled buffers which could be posted
         bool           fBufAssigned{false};  ///< if next buffer assigned
         int            fLastSendCnt{0};      ///< used for flushing
         dabc::TimeStamp fLastDebugTm;        ///< timer used to generate rare debugs output

         void ProcessTimerEvent(unsigned timer) override;

         int ExecuteCommand(dabc::Command cmd) override;

      public:
         NewTransport(dabc::Command, const dabc::PortRef& inpport, NewAddon* addon, double flush = 1, double heartbeat = -1);
         virtual ~NewTransport();

         /** Methods activated by Port, when transport starts/stops. */
         bool StartTransport() override;
         bool StopTransport() override;

         bool ProcessSend(unsigned port) override;
         bool ProcessBuffer(unsigned pool) override;

         bool AssignNewBuffer(unsigned pool, NewAddon* addon = nullptr);
         void BufferReady();
         void FlushBuffer(bool onclose = false);

   };

}

#endif
