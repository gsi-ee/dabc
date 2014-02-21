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



#define DEFAULT_MTU 63 * 1024

namespace hadaq {

   class DataTransport;

   /** \brief %Addon for socket thread to handle UDP data stream from TRB */

   class DataSocketAddon : public dabc::SocketAddon,
                           public dabc::DataInput {
      protected:

         friend class DataTransport;

         int                fNPort;           ///< upd port number
         dabc::Pointer      fTgtPtr;          ///< pointer used to read data
         bool               fWaitMoreData;    ///< indicate that transport waits for more data
         unsigned           fMTU;             ///< maximal size of packet expected from TRB
         double             fFlushTimeout;    ///< time when buffer will be flushed
         int                fSendCnt;         ///< counter of send buffers since last timeout active

         uint64_t           fTotalRecvPacket;
         uint64_t           fTotalDiscardPacket;
         uint64_t           fTotalRecvMsg;
         uint64_t           fTotalDiscardMsg;
         uint64_t           fTotalRecvBytes;
         uint64_t           fTotalRecvEvents;
         uint64_t           fTotalRecvBuffers;
         uint64_t           fTotalDroppedBuffers;


         virtual void ProcessEvent(const dabc::EventId&);
         virtual double ProcessTimeout(double lastdiff);

         void MakeCallback(unsigned sz);

         /* Use codes which are valid for Read_Start */
         unsigned ReadUdp();

      public:
         DataSocketAddon(int fd, int nport, int mtu, double flush);
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
         void ClearExportedCounters();

         virtual void ProcessTimerEvent(unsigned timer);

         virtual void ProcessConnectEvent(const std::string& name, bool on);

      public:
         DataTransport(dabc::Command, const dabc::PortRef& inpport, DataSocketAddon* addon, bool observer);
         virtual ~DataTransport();

   };

}

#endif
