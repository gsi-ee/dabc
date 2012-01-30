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

#ifndef VERBS_Worker
#define VERBS_Worker

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#include <stdint.h>

struct ibv_ah;

namespace dabc {
   class Port;
   class ConnectionObject;
}

namespace verbs {

   class QueuePair;
   class ComplQueue;
   class MemoryPool;
   class Thread;
   class Device;

   class Worker : public dabc::Worker {

      friend class verbs::Thread;

      protected:

         QueuePair      *fQP;

      public:

         enum EVerbsProcEvents {
            evntVerbsSendCompl = evntFirstSystem,
            evntVerbsRecvCompl,
            evntVerbsError,
            evntVerbsLast // from this event user can specified it own events
         };

         Worker(QueuePair* qp);

         virtual ~Worker();

         virtual const char* RequiredThrdClass() const;

         void SetQP(QueuePair* qp);
         inline QueuePair* QP() const { return fQP; }
         QueuePair* TakeQP();

         virtual void ProcessEvent(const dabc::EventId&);

         virtual void VerbsProcessSendCompl(uint32_t) {}
         virtual void VerbsProcessRecvCompl(uint32_t) {}
         virtual void VerbsProcessOperError(uint32_t) {}

         void CloseQP();

         virtual const char* ClassName() const { return "verbs::Worker"; }
   };

   class WorkerRef : public dabc::WorkerRef {

      DABC_REFERENCE(WorkerRef, dabc::WorkerRef, verbs::Worker)

   };

}

#endif
