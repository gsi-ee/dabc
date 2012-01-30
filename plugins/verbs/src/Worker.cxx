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

#include "verbs/Worker.h"

#include "dabc/Command.h"
#include "dabc/ConnectionManager.h"

#include "verbs/Device.h"
#include "verbs/QueuePair.h"
#include "verbs/ComplQueue.h"
#include "verbs/MemoryPool.h"
#include "verbs/Thread.h"

ibv_gid VerbsConnThrd_Multi_Gid =  {   {
      0xFF, 0x12, 0xA0, 0x1C,
      0xFE, 0x80, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x33, 0x44, 0x55, 0x66
    } };


verbs::Worker::Worker(QueuePair* qp) :
   dabc::Worker(0, "Worker"),
   fQP(qp)
{
}

verbs::Worker::~Worker()
{
   CloseQP();
}

const char* verbs::Worker::RequiredThrdClass() const
{
   return VERBS_THRD_CLASSNAME;
}

void verbs::Worker::SetQP(QueuePair* qp)
{
   CloseQP();
   fQP = qp;
}

void verbs::Worker::CloseQP()
{
   if (fQP!=0) {
      delete fQP;
      fQP = 0;
   }
}

verbs::QueuePair* verbs::Worker::TakeQP()
{
   QueuePair* qp = fQP;
   fQP = 0;
   return qp;
}


void verbs::Worker::ProcessEvent(const dabc::EventId& evnt)
{
   switch (evnt.GetCode()) {

      case evntVerbsSendCompl:
         VerbsProcessSendCompl(evnt.GetArg());
         break;

      case evntVerbsRecvCompl:
         VerbsProcessRecvCompl(evnt.GetArg());
         break;

      case evntVerbsError:
         VerbsProcessOperError(evnt.GetArg());
         break;

      default:
         dabc::Worker::ProcessEvent(evnt);
   }
}
