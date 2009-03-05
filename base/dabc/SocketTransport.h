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
#ifndef DABC_SocketTransport
#define DABC_SocketTransport

#ifndef DABC_NetworkTransport
#include "dabc/NetworkTransport.h"
#endif

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#ifndef DABC_collections
#include "dabc/collections.h"
#endif

namespace dabc {

   class SocketDevice;

   class SocketTransport : public NetworkTransport,
                           public SocketIOProcessor {

      typedef Queue<uint32_t> RecIdsQueue;

      enum ESocketTranportEvents {
            evntActivateSend = evntSocketLast,
            evntActivateRecv,
            evntActiavtePool
         };

      protected:
         // these are methods from NetworkTransport
         virtual void _SubmitSend(uint32_t recid);
         virtual void _SubmitRecv(uint32_t recid);

         virtual bool ProcessPoolRequest();

         // these are methods from SocketIOProcessor
         virtual void ProcessEvent(dabc::EventId);

         virtual void OnSendCompleted();
         virtual void OnRecvCompleted();

         virtual void OnConnectionClosed();
         virtual void OnSocketError(int errnum, const char* info);

         virtual void ErrorCloseTransport();

         bool        fIsDatagram;  // indicate if this is datagram socket and one can only user recv once per data packaet
         char*       fHeaders;
         RecIdsQueue fSendQueue;
         RecIdsQueue fRecvQueue;
         int         fRecvStatus; // 0 - idle, 1-header, 2 - front buffer from recv queue
         uint32_t    fRecvRecid; // if of the record, used for data receiving (status != 0)
         int         fSendStatus; // 0 - idle, 1 - sendinf
         uint32_t    fSendRecid; // id of the active send record


      public:
         SocketTransport(SocketDevice* dev, Port* port, bool useackn, int fd, bool isdatagram = false);
         virtual ~SocketTransport();

         virtual unsigned MaxSendSegments() { return 1000; }
   };
}

#endif
