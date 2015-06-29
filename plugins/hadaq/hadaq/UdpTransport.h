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

#include "hadaq/HadaqTypeDefs.h"

#include <sched.h>

#define DEFAULT_MTU 63 * 1024

namespace hadaq {

   class DataTransport;
   class TerminalModule;

   /** \brief %Addon for socket thread to handle UDP data stream from TRB */

   class DataSocketAddon : public dabc::SocketAddon,
                           public dabc::DataInput {
      protected:

         friend class TerminalModule;  // use only to access statistic, nothing else
         friend class DataTransport;

         int                fNPort;           ///< upd port number
         dabc::Pointer      fTgtPtr;          ///< pointer used to read data
         bool               fWaitMoreData;    ///< indicate that transport waits for more data
         unsigned           fMTU;             ///< maximal size of packet expected from TRB
         double             fFlushTimeout;    ///< time when buffer will be flushed
         int                fSendCnt;         ///< counter of send buffers since last timeout active
         int                fMaxLoopCnt;      ///< maximal number of UDP packets, read at once

         uint64_t           fTotalRecvPacket;
         uint64_t           fTotalDiscardPacket;
         uint64_t           fTotalDiscard32Packet;
         uint64_t           fTotalRecvBytes;
         uint64_t           fTotalDiscardBytes;
         uint64_t           fTotalProducedBuffers;

         pid_t fPid;                        // process id
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
         DataSocketAddon(int fd, int nport, int mtu, double flush, bool debug, int maxloop);
         virtual ~DataSocketAddon();

         // this is interface from DataInput
         virtual unsigned Read_Size() { return dabc::di_DfltBufSize; }
         virtual unsigned Read_Start(dabc::Buffer& buf);
         virtual unsigned Read_Complete(dabc::Buffer& buf);
         virtual double Read_Timeout() { return 0.1; }

         void ClearCounters();

         static int OpenUdp(int nport, int rcvbuflen);

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

}

#endif
