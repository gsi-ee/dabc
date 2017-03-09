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

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
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
      uint64_t           fTotalRecvBytes;
      uint64_t           fTotalDiscardBytes;
      uint64_t           fTotalProducedBuffers;

      void ClearCounters()
      {
         fTotalRecvPacket = 0;
         fTotalDiscardPacket = 0;
         fTotalDiscard32Packet = 0;
         fTotalRecvBytes = 0;
         fTotalDiscardBytes = 0;
         fTotalProducedBuffers = 0;
      }

      TransportInfo(int port) :
         fNPort(port) { ClearCounters(); }
   };


   /** \brief %Addon for socket thread to handle UDP data stream from TRB */

   class DataSocketAddon : public dabc::SocketAddon,
                           public dabc::DataInput,
                           public TransportInfo {
      protected:

         friend class TerminalModule;  // use only to access statistic, nothing else
         friend class DataTransport;

         dabc::Pointer      fTgtPtr;          ///< pointer used to read data
         bool               fWaitMoreData;    ///< indicate that transport waits for more data
         unsigned           fMTU;             ///< maximal size of packet expected from TRB
         double             fFlushTimeout;    ///< time when buffer will be flushed
         int                fSendCnt;         ///< counter of send buffers since last timeout active
         int                fMaxLoopCnt;      ///< maximal number of UDP packets, read at once
         double             fReduce;          ///< reduce filled buffer size to let reformat data later

         bool   fDebug;                     ///< when true, produce more debug output

         virtual void ProcessEvent(const dabc::EventId&);
         virtual double ProcessTimeout(double lastdiff);

         void MakeCallback(unsigned sz);

         /* Use codes which are valid for Read_Start */
         unsigned ReadUdp();

         virtual dabc::WorkerAddon* Read_GetAddon() { return this; }

         /** Light-weight command interface, which can be used from worker */
         virtual long Notify(const std::string&, int);

      public:
         DataSocketAddon(int fd, int nport, int mtu, double flush, bool debug, int maxloop, double reduce);
         virtual ~DataSocketAddon();

         // this is interface from DataInput
         virtual unsigned Read_Size() { return dabc::di_DfltBufSize; }
         virtual unsigned Read_Start(dabc::Buffer& buf);
         virtual unsigned Read_Complete(dabc::Buffer& buf);
         virtual double Read_Timeout() { return 0.1; }
   };

   // ================================================================

   /** \brief Special HADAQ input transport
    *
    * Required to be able export different ratemeters to EPICS
    */

   class DataTransport : public dabc::InputTransport {

      protected:

         int            fIdNumber;
         bool           fWithObserver;
         std::string    fDataRateName;

         std::string GetNetmemParName(const std::string& name);
         void CreateNetmemPar(const std::string& name);
         void SetNetmemPar(const std::string& name, unsigned value);

         void RegisterExportedCounters();
         bool UpdateExportedCounters();

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int ExecuteCommand(dabc::Command cmd);

      public:
         DataTransport(dabc::Command, const dabc::PortRef& inpport, DataSocketAddon* addon, bool observer);
         virtual ~DataTransport();

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

         bool               fDebug;             ///< when true, produce more debug output

         bool               fRunning;         ///< is transport running

         virtual void ProcessEvent(const dabc::EventId&);

         /** Light-weight command interface */
         virtual long Notify(const std::string&, int);

         /* Use codes which are valid for Read_Start */
         bool ReadUdp();

         bool CloseBuffer();

      public:
         NewAddon(int fd, int nport, int mtu, bool debug, int maxloop, double reduce);
         virtual ~NewAddon();

         bool HasBuffer() const { return !fTgtPtr.null(); }

         static int OpenUdp(const std::string& host, int nport, int rcvbuflen);
   };


   class NewTransport : public dabc::Transport {

      protected:

         int            fIdNumber;
         bool           fWithObserver;
         std::string    fDataRateName;
         unsigned       fNumReadyBufs; ///< number of filled buffers which could be posted
         bool           fBufAssigned;  ///< if next buffer assigned
         int            fLastSendCnt;  ///< used for flushing

         std::string GetNetmemParName(const std::string& name);
         void CreateNetmemPar(const std::string& name);
         void SetNetmemPar(const std::string& name, unsigned value);

         void RegisterExportedCounters();
         bool UpdateExportedCounters();

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int ExecuteCommand(dabc::Command cmd);

      public:
         NewTransport(dabc::Command, const dabc::PortRef& inpport, NewAddon* addon, bool observer, double flush = 1);
         virtual ~NewTransport();

         /** Methods activated by Port, when transport starts/stops. */
         virtual bool StartTransport();
         virtual bool StopTransport();

         virtual bool ProcessSend(unsigned port);
         virtual bool ProcessBuffer(unsigned pool);

         bool AssignNewBuffer(unsigned pool, NewAddon* addon = 0);
         void BufferReady();
         void FlushBuffer(bool onclose = false);

   };


}

#endif
