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

   /** Structure to keep event/time frame identifier
    * 64-bit unsigned value is used. There are following special values:
    *   0 - null(), no event - no data expected
    *  */

   struct EventId {
      protected:

      uint64_t id;

      public:

      EventId() : id(0) {}

      EventId(const EventId& src) : id(src.id) {}

      EventId(uint64_t src) : id(src) {}

      EventId& operator=(const EventId& src) { id = src.id; return *this; }

      EventId& operator=(uint64_t src) { id = src; return *this; }

      inline operator uint64_t() const { return id; }
      inline void Set(uint64_t _id) { id = _id; }
      inline uint64_t Get() const { return id; }

      inline bool null() const { return id==0; }
      inline void SetNull() { id = 0; }

      inline void SetCtrl() { id = (uint64_t) -1; }
      inline bool ctrl() const { return id == (uint64_t) -1; }

//      inline bool operator<(const EventId& src) const { return id < src.id; }

//      inline bool operator>(const EventId& src) const { return id > src.id; }

//      inline bool operator==(const EventId& src) const { return id == src.id; }

//      inline uint64_t operator()() const { return id; }

      inline EventId& operator++(int) { id++; return *this; }


   };

   // TODO: should be a parameter, not a constant
   enum { MAXLID = 16 };

   struct TimeSyncMessage
   {
      int64_t msgid;
      double  master_time;
      double  slave_recv;
      double  slave_send;
      double  slave_shift;
      double  slave_scale;
   };

   class EventHandling : public dabc::Object {
      public:
         EventHandling(const char* name) : dabc::Object(name) {}

         virtual bool GenerateSubEvent(const bnet::EventId& evid, int subid, int numsubids, dabc::Buffer& buf) = 0;

         virtual bool ExtractEventId(const dabc::Buffer& buf, bnet::EventId& evid) = 0;

         virtual dabc::Buffer BuildFullEvent(const bnet::EventId& evid, dabc::Buffer* bufs, int numbufs) = 0;
   };

   class EventHandlingRef : public dabc::Reference {
      DABC_REFERENCE(EventHandlingRef, dabc::Reference, EventHandling)
   };
}

#endif
