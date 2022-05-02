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

#ifndef HADAQ_HadaqTypeDefs
#include "hadaq/HadaqTypeDefs.h"
#endif

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

#ifndef DABC_Pointer
#include "dabc/Pointer.h"
#endif

#ifndef DABC_eventsapi
#include "dabc/eventsapi.h"
#endif

namespace hadaq {

   /** \brief Read iterator for HADAQ events/subevents */

   class ReadIterator {
      protected:
         bool           fFirstEvent{false};
         dabc::Pointer  fEvPtr;
         dabc::Pointer  fSubPtr;
         dabc::Pointer  fRawPtr;
         unsigned       fBufType{dabc::mbt_Null};

      public:
         ReadIterator() {}

         ReadIterator(const dabc::Buffer& buf) { Reset(buf); }

         ReadIterator(hadaq::RawEvent *evnt)
         {
            fFirstEvent = false;
            fBufType = mbt_HadaqEvents;
            fEvPtr.reset(evnt, evnt->GetPaddedSize());
         }

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

         /** Returns size used by current event plus rest */
         unsigned remained_size() const { return fEvPtr.fullsize(); }

         hadaq::HadTu* hadtu() const { return (hadaq::HadTu*) fEvPtr(); }

         bool AssignEventPointer(dabc::Pointer& ptr);
         hadaq::RawSubevent* subevnt() const { return (hadaq::RawSubevent*) fSubPtr(); }
         void* rawdata() const { return fRawPtr(); }
         uint32_t rawdatasize() const { return fRawPtr.fullsize(); }

         /** Try to define maximal length for the raw data */
         unsigned rawdata_maxsize() const;

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
         bool NewEvent(uint32_t evtSeqNr = 0, uint32_t runNr = 0, uint32_t minsubeventsize = 0);
         bool NewSubevent(uint32_t minrawsize = 0, uint32_t trigger = 0);
         bool FinishSubEvent(uint32_t rawdatasz);

         bool AddSubevent(const dabc::Pointer &source);
         bool AddSubevent(const void *ptr, unsigned len);
         bool AddSubevent(hadaq::RawSubevent* sub)
         {
            return AddSubevent(sub, sub->GetPaddedSize());
         }
         bool AddAllSubevents(hadaq::RawEvent *evnt)
         {
            return AddSubevent(evnt->FirstSubevent(), evnt->AllSubeventsSize());
         }

         bool CopyEvent(const ReadIterator &iter);

         bool FinishEvent();

         dabc::Buffer Close();

         hadaq::RawEvent* evnt() const { return (hadaq::RawEvent*) fEvPtr(); }
         hadaq::RawSubevent* subevnt() const { return (hadaq::RawSubevent*) fSubPtr(); }
         void* rawdata() const { return subevnt() ? subevnt()->RawData() : nullptr; }
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
         EventsIterator(const std::string &name) : dabc::EventsIterator(name), fIter() {}
         virtual ~EventsIterator() {}

         bool Assign(const dabc::Buffer& buf) override { return fIter.Reset(buf); }
         void Close() override { return fIter.Close(); }

         bool NextEvent() override { return fIter.NextEvent(); };
         void *Event() override { return fIter.evnt(); }
         dabc::BufferSize_t EventSize() override { return fIter.evntsize(); }
   };

   // _______________________________________________________________________________________________

   class EventsProducer : public dabc::EventsProducer {
      protected:
         WriteIterator fIter;

      public:
         EventsProducer(const std::string &name) : dabc::EventsProducer(name), fIter() {}
         virtual ~EventsProducer() {}

         bool Assign(const dabc::Buffer& buf) override { return fIter.Reset(buf); }
         void Close() override { fIter.Close(); }

         bool GenerateEvent(uint64_t evid, uint64_t subid, uint64_t raw_size) override
         {
            (void) subid;
            if (!fIter.IsPlaceForEvent(raw_size)) return false;
            if (!fIter.NewEvent(evid)) return false;
            if (!fIter.NewSubevent(raw_size)) return false;

            if (!fIter.FinishSubEvent(raw_size)) return false;
            fIter.FinishEvent();
            return true;
         }
   };


}

#endif
