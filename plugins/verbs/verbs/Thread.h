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

#ifndef VERBS_Thread
#define VERBS_Thread

#ifndef DABC_Thread
#include "dabc/Thread.h"
#endif

#ifndef VERBS_Context
#include "verbs/Context.h"
#endif

#include <map>
#include <cstdint>

struct ibv_comp_channel;
struct ibv_wc;

#define VERBS_USING_PIPE

namespace verbs {

   class MemoryPool;

   class Worker;
   class TimeoutWorker;

   class QueuePair;
   class ComplQueue;

   /** \brief VERBS thread */


   class Thread : public dabc::Thread {

      typedef std::map<uint32_t, uint32_t>  WorkersMap;

      enum { LoopBackQueueSize = 10 };

      enum EVerbsThreadEvents {
         evntEnableCheck = evntLastThrd+1,  ///< event to enable again checking sockets for new events
         evntLastVerbsThrdEvent             ///< last event, which can be used by verbs thread
      };

      protected:
         enum EWaitStatus { wsWorking, wsWaiting, wsFired };

         ContextRef                fContext;
         struct ibv_comp_channel *fChannel{nullptr};
      #ifdef VERBS_USING_PIPE
         int                      fPipe[2] = {0,0};
      #else
         QueuePair               *fLoopBackQP{nullptr};
         MemoryPool              *fLoopBackPool{nullptr};
         int                      fLoopBackCnt{0};
         TimeoutWorker           *fTimeout{nullptr}; // use to produce timeouts
      #endif
         ComplQueue              *fMainCQ{nullptr};     // completion queue used for the whole thread to be able organize polling instead of waiting for the completion channel
         EWaitStatus              fWaitStatus{wsWorking};
         WorkersMap               fMap; // for 1000 elements one need about 50 nanosec to get value from it
         int                      fWCSize{0}; // size of array
         struct ibv_wc*           fWCs{nullptr}; // list of event completion
         long                     fFastModus{0}; // makes polling over MainCQ before starting wait - defines pooling number
         bool                     fCheckNewEvents{false}; // indicate if we should check verbs events even if there are some in the queue
      public:
         // list of all events for all kind of socket processors

         Thread(dabc::Reference parent, const std::string &name, dabc::Command cmd, ContextRef ctx);
         virtual ~Thread();

         void CloseThread();

         struct ibv_comp_channel* Channel() const { return fChannel; }

         ComplQueue* MakeCQ();

         bool IsFastModus() const { return fFastModus > 0; }

         const char* ClassName() const override { return verbs::typeThread; }

         bool CompatibleClass(const std::string &clname) const override;

         static const char* StatusStr(int code);

      protected:

         int ExecuteThreadCommand(dabc::Command cmd) override;

         bool WaitEvent(dabc::EventId& evid, double tmout) override;

         void _Fire(const dabc::EventId& evnt, int nq) override;

         void WorkersSetChanged() override;

         void ProcessExtraThreadEvent(const dabc::EventId&) override;

   };

   // ___________________________________________________________

   /** \brief %Reference on \ref verbs::Thread */

   class ThreadRef : public dabc::ThreadRef {

      DABC_REFERENCE(ThreadRef, dabc::ThreadRef, verbs::Thread)

   };


}

#endif
