// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#ifndef DOGMA_UdpTransport
#define DOGMA_UdpTransport

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#ifndef DABC_Pointer
#include "dabc/Pointer.h"
#endif

#ifndef DABC_DataTransport
#include "dabc/DataTransport.h"
#endif

#ifndef DOGMA_TypeDefs
#include "dogma/TypeDefs.h"
#endif

namespace dogma {

   class DataTransport;
   class UdpTransport;
   class TerminalModule;

   struct TransportInfo {
      int                fNPort = 0;           ///< upd port number

      uint64_t           fTotalRecvPacket{0};
      uint64_t           fTotalDiscardPacket{0};
      uint64_t           fTotalDiscardMagic{0};
      uint64_t           fTotalArtificialSkip{0};
      uint64_t           fTotalRecvBytes{0};
      uint64_t           fTotalDiscardBytes{0};
      uint64_t           fTotalProducedBuffers{0};

      void ClearCounters()
      {
         fTotalRecvPacket = 0;
         fTotalDiscardPacket = 0;
         fTotalDiscardMagic = 0;
         fTotalArtificialSkip = 0;
         fTotalRecvBytes = 0;
         fTotalDiscardBytes = 0;
         fTotalProducedBuffers = 0;
      }

      TransportInfo(int port) : fNPort(port) { ClearCounters(); }

      std::string GetDiscardString()
      {
         return dabc::number_to_str(fTotalDiscardPacket);
      }

      std::string GetDiscardMagicString()
      {
         std::string res = dabc::number_to_str(fTotalDiscardMagic);

         if (fTotalArtificialSkip > 0)
            res += std::string("#") + dabc::number_to_str(fTotalArtificialSkip);

         return res;
      }

   };

   // ==================================================================================

   class UdpAddon : public dabc::SocketAddon,
                    public TransportInfo {
      protected:

         friend class TerminalModule;  // use only to access statistic, nothing else
         friend class UdpTransport;

         dabc::Pointer      fTgtPtr;             ///< pointer used to read data
         std::string        fHostName;           ///< host name used to create UDP socket
         int                fRecvBufLen{100000}; ///< recv buf len
         unsigned           fMTU{0};             ///< maximal size of packet expected from DOG
         void*              fMtuBuffer{nullptr}; ///< buffer used to skip packets when no normal buffer is available
         int                fSkipCnt{0};         ///< counter used to control buffers skipping
         int                fSendCnt{0};         ///< counter of send buffers since last timeout active
         int                fMaxLoopCnt{0};      ///< maximal number of UDP packets, read at once
         double             fReduce{0};          ///< reduce filled buffer size to let reformat data later
         bool               fDebug{false};       ///< when true, produce more debug output
         bool               fPrint{false};       ///< when true, produce rudimentary event print
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
         UdpAddon(int fd, const std::string &host, int nport, int rcvbuflen, int mtu, bool debug, bool print, int maxloop, double reduce);
          ~UdpAddon() override;

         bool HasBuffer() const { return !fTgtPtr.null(); }

         static int OpenUdp(const std::string &host, int nport, int rcvbuflen);
   };

   // ==================================================================================

   class UdpTransport : public dabc::Transport {

      protected:

         int            fIdNumber = 0;         ///< input port item id
         unsigned       fNumReadyBufs = 0;     ///< number of filled buffers which could be posted
         bool           fBufAssigned = false;  ///< if next buffer assigned
         int            fLastSendCnt = 0;      ///< used for flushing
         dabc::TimeStamp fLastDebugTm;        ///< timer used to generate rare debugs output

         void ProcessTimerEvent(unsigned timer) override;

         int ExecuteCommand(dabc::Command cmd) override;

      public:
         UdpTransport(dabc::Command, const dabc::PortRef& inpport, UdpAddon *addon, double flush = 1, double heartbeat = -1);
         ~UdpTransport() override;

         /** Methods activated by Port, when transport starts/stops. */
         bool StartTransport() override;
         bool StopTransport() override;

         bool ProcessSend(unsigned port) override;
         bool ProcessBuffer(unsigned pool) override;

         bool AssignNewBuffer(unsigned pool, UdpAddon *addon = nullptr);
         void BufferReady();
         void FlushBuffer(bool onclose = false);

   };

} // namespace dogma

#endif
