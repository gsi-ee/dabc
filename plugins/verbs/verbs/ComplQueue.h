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

#ifndef VERBS_ComplQueue
#define VERBS_ComplQueue

#ifndef VERBS_Context
#include "verbs/Context.h"
#endif

namespace verbs {

   /** \brief Context object for completion queue operations */

   struct ComplQueueContext {
      int            events_get;
      void*          itself;
      struct ibv_cq* own_cq;
   };

   // __________________________________________________________________

   /** \brief Wrapper for IB VERBS completion queue  */

   class ComplQueue  {
      protected:

         ContextRef               fContext;

         struct ibv_cq           *f_cq;
         struct ibv_comp_channel *f_channel;
         bool                     f_ownchannel;

         ComplQueueContext        fCQContext;
         // FIXME: for a moment put here, later should be removed
         struct ibv_wc            f_wc;

      public:

         ComplQueue(ContextRef ctx, int size,
                     struct ibv_comp_channel *channel, bool use_own_channel = false);

         virtual ~ComplQueue();

         struct ibv_comp_channel *channel() const { return f_channel; }
         struct ibv_cq *cq() const { return f_cq; }
         uint64_t arg() const { return f_wc.wr_id; }

         int Poll();
         int Wait(double timeout, double fasttm = 0.);
         void AcknoledgeEvents();

         static const char *GetStrError(int err);
   };
}

#endif
