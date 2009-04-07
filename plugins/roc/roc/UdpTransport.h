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

#ifndef ROC_UdpTransport
#define ROC_UdpTransport

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#ifndef DABC_Transport
#include "dabc/Transport.h"
#endif

#ifndef DABC_collections
#include "dabc/collections.h"
#endif

namespace roc {

   class UdpDevice;

   class UdpDataSocket : public dabc::SocketIOProcessor,
                         public dabc::Transport {
      friend class UdpDevice;
      protected:
         enum EUdpEvents { evntSendReq = evntSocketLast };

         enum EDaqState { daqInit, daqStarting, daqRuns, daqStopping, daqStopped, daqFails };

         UdpDevice*         fDev;
         dabc::Mutex        fQueueMutex;
         dabc::BuffersQueue fQueue;

         EDaqState          daqState;

      public:
         UdpDataSocket(UdpDevice* dev, int fd);
         virtual ~UdpDataSocket();

         virtual void ProcessEvent(dabc::EventId evnt);

         virtual bool ProvidesInput() { return true; }
         virtual bool ProvidesOutput() { return false; }

         virtual bool Recv(dabc::Buffer* &buf);
         virtual unsigned  RecvQueueSize() const;
         virtual dabc::Buffer* RecvBuffer(unsigned indx) const;
         virtual bool Send(dabc::Buffer* buf) { return false; }
         virtual unsigned SendQueueSize() { return 0; }
         virtual unsigned MaxSendSegments() { return 0; }

         virtual void StartTransport();
         virtual void StopTransport();

   };

}

#endif
