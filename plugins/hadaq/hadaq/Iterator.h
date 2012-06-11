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

#ifndef HADAQ_Iterator
#define HADAQ_Iterator

#ifndef HADAQ_HadaqTypeDefs
#include "hadaq/HadaqTypeDefs.h"
#endif

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

#ifndef DABC_Pointer
#include "dabc/Pointer.h"
#endif

namespace hadaq {

   class ReadIterator {
      protected:
         bool           fFirstEvent;
         dabc::Pointer  fEvPtr;
         dabc::Pointer  fSubPtr;
         dabc::Pointer  fRawPtr;

      public:
         ReadIterator();

         ReadIterator(const dabc::Buffer& buf);

         ReadIterator(const ReadIterator& src);

         ReadIterator& operator=(const ReadIterator& src);

         ~ReadIterator() { Close(); }

         /** Initialize iterator on the beginning of the buffer, buffer instance should exists until
          * end of iterator usage */
         bool Reset(const dabc::Buffer& buf);

         /** Reset iterator - forget pointer on buffer */
         bool Reset() { Close(); return true; }

         void Close();

         bool IsData() const { return !fEvPtr.null(); }

         bool NextEvent();
         bool NextSubEvent();

         hadaq::Event* evnt() const { return (hadaq::Event*) fEvPtr(); }
         bool AssignEventPointer(dabc::Pointer& ptr);
         hadaq::Subevent* subevnt() const { return (hadaq::Subevent*) fSubPtr(); }
         void* rawdata() const { return fRawPtr(); }
         uint32_t rawdatasize() const { return fRawPtr.fullsize(); }

         static unsigned NumEvents(const dabc::Buffer& buf);
   };

   class WriteIterator {
      public:
         WriteIterator();
         WriteIterator(const dabc::Buffer& buf);
         ~WriteIterator();

         bool Reset(const dabc::Buffer& buf);

         bool IsBuffer() const { return !fBuffer.null(); }
         bool IsEmpty() const { return fFullSize == 0; }
         bool IsPlaceForEvent(uint32_t subeventsize);
         bool NewEvent(EventNumType event_number = 0, uint32_t subeventsize = 0);
         bool NewSubevent(uint32_t minrawsize = 0, uint32_t trigger = 0);
         bool FinishSubEvent(uint32_t rawdatasz);

         bool AddSubevent(const dabc::Pointer& source);
         bool AddSubevent(hadaq::Subevent* sub);
         bool FinishEvent();

         dabc::Buffer Close();

         hadaq::Event* evnt() const { return (hadaq::Event*) fEvPtr(); }
         hadaq::Subevent* subevnt() const { return (hadaq::Subevent*) fSubPtr(); }
         void* rawdata() const { return subevnt() ? subevnt()->RawData() : 0; }
         uint32_t maxrawdatasize() const { return fSubPtr.null() ? 0 : fSubPtr.fullsize() - sizeof(hadaq::Subevent); }

      protected:
         dabc::Buffer   fBuffer; // here we keep buffer - mean onwership is delivered to iterator
         dabc::Pointer  fEvPtr;
         dabc::Pointer  fSubPtr;
         dabc::BufferSize_t fFullSize;
   };

}

#endif
