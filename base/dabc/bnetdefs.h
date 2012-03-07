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

#ifndef DABC_bnetdefs
#define DABC_bnetdefs

#include <stdint.h>

#ifndef DABC_Object
#include "dabc/Object.h"
#endif

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

namespace bnet {

   typedef uint64_t EventId;

   EventId DummyEventId();

   enum { MAXLID = 16 };

   struct TimeSyncMessage
   {
      int64_t msgid;
      double  master_time;
      double  slave_shift;
      double  slave_time;
      double  slave_scale;
   };

   class EventHandling : public dabc::Object {
      public:
         EventHandling(const char* name) : dabc::Object(name) {}

         virtual bool GenerateSubEvent(bnet::EventId evid, int subid, int numsubids, dabc::Buffer& buf) = 0;

         virtual bool ExtractEventId(const dabc::Buffer& buf, bnet::EventId& evid) = 0;

         virtual dabc::Buffer BuildFullEvent(bnet::EventId evid, dabc::Buffer* bufs, int numbufs) = 0;
   };

   class EventHandlingRef : public dabc::Reference {
      DABC_REFERENCE(EventHandlingRef, dabc::Reference, EventHandling)
   };
}

#endif
