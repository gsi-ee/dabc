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

#ifndef HADAQ_Iterator
#define HADAQ_Iterator

#include "hadaq/HadaqTypeDefs.h"
#include "dabc/Buffer.h"
#include "dabc/Pointer.h"

namespace hadaq {

   /** \brief Read iterator for HADAQ events/subevents */

   class ReadIterator {
      protected:
         bool           fFirstEvent;
         dabc::Pointer  fEvPtr;
         dabc::Pointer  fSubPtr;
         dabc::Pointer  fRawPtr;

         unsigned fBufType;

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

         /** Used for raw data from TRBs */
         bool NextHadTu();
         /** Used for ready HLD events */
         bool NextEvent();

         /** Depending from buffer type calls NextHadTu() or NextEvent() */
         bool NextSubeventsBlock();

         /** Used for sub-events iteration inside current block */
         bool NextSubEvent();

         hadaq::RawEvent* evnt() const { return (hadaq::RawEvent*) fEvPtr(); }
         unsigned evntsize() const { return evnt() ? evnt()->GetPaddedSize() : 0; }

         hadaq::HadTu* hadtu() const { return (hadaq::HadTu*) fEvPtr(); }
         bool AssignEventPointer(dabc::Pointer& ptr);
         hadaq::RawSubevent* subevnt() const { return (hadaq::RawSubevent*) fSubPtr(); }
         void* rawdata() const { return fRawPtr(); }
         uint32_t rawdatasize() const { return fRawPtr.fullsize(); }

         static unsigned NumEvents(const dabc::Buffer& buf);
   };

   // _____________________________________________________________________

   /** \brief Write iterator for HADAQ events/subevents */

   class WriteIterator {
      public:
         WriteIterator();
         WriteIterator(const dabc::Buffer& buf);
         ~WriteIterator();

         bool Reset(const dabc::Buffer& buf);

         bool IsBuffer() const { return !fBuffer.null(); }
         bool IsEmpty() const { return fFullSize == 0; }
         bool IsPlaceForEvent(uint32_t subeventsize);
         bool NewEvent(uint32_t evtSeqNr=0, uint32_t runNr=0, uint32_t minsubeventsize=0);
         bool NewSubevent(uint32_t minrawsize = 0, uint32_t trigger = 0);
         bool FinishSubEvent(uint32_t rawdatasz);

         bool AddSubevent(const dabc::Pointer& source);
         bool AddSubevent(hadaq::RawSubevent* sub);
         bool FinishEvent();

         dabc::Buffer Close();

         hadaq::RawEvent* evnt() const { return (hadaq::RawEvent*) fEvPtr(); }
         hadaq::RawSubevent* subevnt() const { return (hadaq::RawSubevent*) fSubPtr(); }
         void* rawdata() const { return subevnt() ? subevnt()->RawData() : 0; }
         uint32_t maxrawdatasize() const { return fSubPtr.null() ? 0 : fSubPtr.fullsize() - sizeof(hadaq::RawSubevent); }

      protected:
         dabc::Buffer   fBuffer; // here we keep buffer - mean ownership is delivered to iterator
         dabc::Pointer  fEvPtr;
         dabc::Pointer  fSubPtr;
         dabc::BufferSize_t fFullSize;
   };

   // _______________________________________________________________________________________________

   class EventsIterator : public dabc::EventsIterator {
      protected:
         ReadIterator fIter;

      public:
         EventsIterator(const std::string& name) : dabc::EventsIterator(name), fIter() {}
         virtual ~EventsIterator() {}

         virtual bool Assign(const dabc::Buffer& buf) { return fIter.Reset(buf); }
         virtual void Close() { return fIter.Close(); }

         virtual bool NextEvent() { return fIter.NextEvent(); };
         virtual void* Event() { return fIter.evnt(); }
         virtual dabc::BufferSize_t EventSize() { return fIter.evntsize(); }
   };

}

#endif
