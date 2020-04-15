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

#ifndef DABC_eventsapi
#define DABC_eventsapi

#include <cstdint>

#ifndef DABC_Object
#include "dabc/Object.h"
#endif

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

#ifndef DABC_Pointer
#include "dabc/Pointer.h"
#endif

/** \brief Event manipulation API */

namespace dabc {

   /** \brief Iterator over events in \ref dabc::Buffer class
    *
    * \ingroup dabc_all_classes
    *
    * Allows to scan events in the buffer
    */

   class EventsIterator : public Object {
      public:
         EventsIterator(const std::string &name) : Object(name, flAutoDestroy) {}
         virtual ~EventsIterator() {}

         virtual bool Assign(const Buffer& buf) = 0;
         virtual void Close() = 0;

         virtual bool NextEvent() = 0;
         virtual void* Event() = 0;
         virtual BufferSize_t EventSize() = 0;

         unsigned NumEvents(const Buffer& buf);
   };

   class EventsIteratorRef : public Reference {
      DABC_REFERENCE(EventsIteratorRef, Reference, EventsIterator)
   };

   // ----------------------------------------------------------------------------------------


   /** \brief Producer of the events
    *
    * \ingroup dabc_all_classes
    *
    * Allows to produce/generate events in the buffer
    */

   class EventsProducer : public Object {
      public:
         EventsProducer(const std::string &name) : Object(name, flAutoDestroy) {}
         virtual ~EventsProducer() {}

         virtual bool Assign(const Buffer& buf) = 0;
         virtual void Close() = 0;

         virtual bool GenerateEvent(uint64_t evid, uint64_t subid, uint64_t raw_size) = 0;
   };


   class EventsProducerRef : public Reference {
      DABC_REFERENCE(EventsProducerRef, Reference, EventsProducer)
   };
}

#endif
