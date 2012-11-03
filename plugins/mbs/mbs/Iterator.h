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

#ifndef MBS_Iterator
#define MBS_Iterator

#ifndef MBS_MbsTypeDefs
#include "mbs/MbsTypeDefs.h"
#endif

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

#ifndef DABC_Pointer
#include "dabc/Pointer.h"
#endif

namespace mbs {

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

         EventHeader* evnt() const { return (EventHeader*) fEvPtr(); }
         // return actual event size, will not exceed buffers limit
         unsigned GetEventSize() const;
         bool AssignEventPointer(dabc::Pointer& ptr);

         /** Returns pointer, which assign on complete event, including header */
         dabc::Pointer GetEventPointer();

         /** Returns pointer, which assign only on subevents */
         dabc::Pointer GetSubeventsPointer();


         SubeventHeader* subevnt() const { return (SubeventHeader*) fSubPtr(); }
         void* rawdata() const { return fRawPtr(); }
         unsigned rawdatasize() const { return fRawPtr.fullsize(); }

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

         bool NewSubevent(uint32_t minrawsize = 0, uint8_t crate = 0, uint16_t procid = 0, uint8_t control = 0);
         bool FinishSubEvent(uint32_t rawdatasz);
         bool AddSubevent(const dabc::Pointer& source);
         bool AddSubevent(SubeventHeader* sub);

         bool FinishEvent();

         /** Method to copy full event from the other place.
          * ptr should contain event with full header.
          * Fot instance, mbs::ReadIterator.AssignEventPointer() could be used.
          * If finish==false, one could add more subevents. */
         bool CopyEventFrom(const dabc::Pointer& ptr, bool finish = true);

         dabc::Buffer Close();

         EventHeader* evnt() const { return (EventHeader*) fEvPtr(); }
         SubeventHeader* subevnt() const { return (SubeventHeader*) fSubPtr(); }
         void* rawdata() const { return subevnt() ? subevnt()->RawData() : 0; }
         uint32_t maxrawdatasize() const { return fSubPtr.null() ? 0 : fSubPtr.fullsize() - sizeof(SubeventHeader); }

      protected:
         dabc::Buffer   fBuffer; // here we keep buffer - mean onwership is delivered to iterator
         dabc::Pointer  fEvPtr;
         dabc::Pointer  fSubPtr;
         dabc::BufferSize_t fFullSize;
   };

}

#endif
