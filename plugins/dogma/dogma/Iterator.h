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

#ifndef DOGMA_Iterator
#define DOGMA_Iterator

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

#ifndef DABC_Pointer
#include "dabc/Pointer.h"
#endif

#ifndef DABC_eventsapi
#include "dabc/eventsapi.h"
#endif

#ifndef DOGMA_TypeDefs
#include "dogma/TypeDefs.h"
#endif

#ifndef DOGMA_defines
#include "dogma/defines.h"
#endif

namespace dogma {

   class RawIterator : public dabc::EventsIterator {
      protected:
         dabc::Pointer  fRawPtr;

      public:
         RawIterator(const std::string &name) : dabc::EventsIterator(name) {}
         ~RawIterator() override {}

         bool Assign(const dabc::Buffer& buf) override
         {
            Close();
            if (buf.null() || (buf.GetTypeId() != mbt_DogmaTransportUnit))
               return false;

            fRawPtr = buf;
            return true;
         }
         void Close() override { return fRawPtr.reset(); }

         bool NextEvent() override
         {
            auto sz = EventSize();
            if (sz >= fRawPtr.fullsize()) {
               fRawPtr.reset();
               return false;
            }
            fRawPtr.shift(sz);
            return !fRawPtr.null();
         };

         void *Event() override
         {
            if (fRawPtr.fullsize() < sizeof(DogmaTu))
               fRawPtr.reset();

            return fRawPtr();
         }

         dabc::BufferSize_t EventSize() override
         {
            auto tu = (DogmaTu *) Event();
            return tu ? tu->GetMessageSize() : 0;
         }
   };


}

#endif
