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

#ifndef DABC_LocalTransport
#define DABC_LocalTransport

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#ifndef DABC_Transport
#include "dabc/Transport.h"
#endif

#ifndef DABC_BuffersQueue
#include "dabc/BuffersQueue.h"
#endif

namespace dabc {


   class LocalTransport : public Worker,
                          public Transport {

      DABC_TRANSPORT(Worker)

      protected:
         Reference                  fOther;
         BuffersQueue               fQueue;
         Mutex                     *fMutex;
         bool                       fMutexOwner;
         bool                       fMemCopy;
         bool                       fDoInput;
         bool                       fDoOutput;
         bool                       fRunning; //!< indicates that transport is running

         virtual void CleanupTransport();

         virtual void CleanupFromTransport(Object* obj);

      public:
         LocalTransport(Reference port, Mutex* mutex, bool owner, bool memcopy, bool doinp, bool doout);
         virtual ~LocalTransport();

         virtual bool ProvidesInput() { return fDoInput; }
         virtual bool ProvidesOutput() { return fDoOutput; }

         virtual bool Recv(Buffer&);
         virtual unsigned RecvQueueSize() const;
         virtual Buffer& RecvBuffer(unsigned indx) const;
         virtual bool Send(const Buffer&);
         virtual unsigned SendQueueSize();
         virtual unsigned MaxSendSegments() { return 9999; }

         static int ConnectPorts(Reference port1, Reference port2);
   };

}

#endif
