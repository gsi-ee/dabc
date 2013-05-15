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

#ifndef VERBS_Thread
#define VERBS_Thread

#ifndef DABC_Thread
#include "dabc/Thread.h"
#endif

#ifndef VERBS_Context
#include "verbs/Context.h"
#endif

#include <map>
#include <stdint.h>

struct ibv_comp_channel;
struct ibv_wc;

#define VERBS_USING_PIPE

namespace verbs {

   class MemoryPool;

   class Worker;
   class TimeoutWorker;

   class QueuePair;
   class ComplQueue;

   class Thread : public dabc::Thread {

      typedef std::map<uint32_t, uint32_t>  WorkersMap;

      enum { LoopBackQueueSize = 10 };

      enum EVerbsThreadEvents {
         evntEnableCheck = evntLastThrd+1,  //!< event to enable again checking sockets for new events
         evntLastVerbsThrdEvent             //!< last event, which can be used by verbs thread
      };

      protected:
         enum EWaitStatus { wsWorking, wsWaiting, wsFired };

         ContextRef                fContext;
         struct ibv_comp_channel *fChannel;
      #ifdef VERBS_USING_PIPE
         int                      fPipe[2];
      #else
         QueuePair               *fLoopBackQP;
         MemoryPool              *fLoopBackPool;
         int                      fLoopBackCnt;
         TimeoutWorker           *fTimeout; // use to produce timeouts
      #endif
         ComplQueue              *fMainCQ;     // completion queue used for the whole thread to be able organize polling instead of waiting for the completion channel
         EWaitStatus              fWaitStatus;
         WorkersMap               fMap; // for 1000 elements one need about 50 nanosec to get value from it
         int                      fWCSize; // size of array
         struct ibv_wc*           fWCs; // list of event completion
         long                     fFastModus; // makes polling over MainCQ before starting wait - defines pooling number
         bool                     fCheckNewEvents; // indicate if we should check verbs events even if there are some in the queue
      public:
         // list of all events for all kind of socket processors

         Thread(dabc::Reference parent, const std::string& name, dabc::Command cmd, ContextRef ctx);
         virtual ~Thread();

         void CloseThread();

         struct ibv_comp_channel* Channel() const { return fChannel; }

         ComplQueue* MakeCQ();

         bool IsFastModus() const { return fFastModus > 0; }

         virtual const char* ClassName() const { return verbs::typeThread; }

         virtual bool CompatibleClass(const std::string& clname) const;

         static const char* StatusStr(int code);

      protected:

         virtual int ExecuteThreadCommand(dabc::Command cmd);

         virtual bool WaitEvent(dabc::EventId& evid, double tmout);

         virtual void _Fire(const dabc::EventId& evnt, int nq);

         virtual void WorkersSetChanged();

         virtual void ProcessExtraThreadEvent(const dabc::EventId&);

   };

   class ThreadRef : public dabc::ThreadRef {

      DABC_REFERENCE(ThreadRef, dabc::ThreadRef, verbs::Thread)

   };


}

#endif
