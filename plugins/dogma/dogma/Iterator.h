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

   /** \brief Read iterator for HADAQ events/subevents */

   class ReadIterator {
      protected:
         bool           fFirstEvent = false;
         dabc::Pointer  fEvPtr;
         dabc::Pointer  fSubPtr;
         dabc::Pointer  fRawPtr;
         unsigned       fBufType = dabc::mbt_Null;

      public:
         ReadIterator() {}

         ReadIterator(const dabc::Buffer& buf) { Reset(buf); }

         ReadIterator(dogma::DogmaEvent *evnt)
         {
            fFirstEvent = false;
            fBufType = mbt_DogmaEvents;
            fEvPtr.reset(evnt, evnt->GetEventLen());
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

         bool IsTu() const;

         /** Used for raw data from TRBs */
         bool NextTu();

         dogma::DogmaTu* tu() const { return (dogma::DogmaTu*) fEvPtr(); }
         unsigned tusize() const { return tu() ? tu()->GetSize() : 0; }

         /** Used for ready HLD events */
         bool NextEvent();

         bool IsEvent() const;

         dogma::DogmaEvent* evnt() const { return (dogma::DogmaEvent*) fEvPtr(); }
         unsigned evntsize() const { return evnt() ? evnt()->GetEventLen() : 0; }

         /** Depending from buffer type calls NextHadTu() or NextEvent() */
         bool NextSubeventsBlock();

         void *block() const;
         unsigned blocksize() const;

         /** Used for sub-events iteration inside current block */
         bool NextSubEvent();

         /** Returns size used by current event plus rest */
         unsigned remained_size() const { return fEvPtr.fullsize(); }

         bool AssignEventPointer(dabc::Pointer& ptr);
         dogma::DogmaTu* subevnt() const { return (dogma::DogmaTu*) fSubPtr(); }
         void* rawdata() const { return fRawPtr(); }
         uint32_t rawdatasize() const { return fRawPtr.fullsize(); }

         /** Try to define maximal length for the raw data */
         unsigned rawdata_maxsize() const;

         static unsigned NumEvents(const dabc::Buffer& buf);
   };


   // _____________________________________________________________________

   /** \brief Write iterator for DOGMA events/subevents */

   class WriteIterator {
      public:
         WriteIterator();
         WriteIterator(const dabc::Buffer& buf);
         ~WriteIterator();

         bool Reset(const dabc::Buffer& buf);

         bool IsBuffer() const { return !fBuffer.null(); }
         bool IsEmpty() const { return fFullSize == 0; }
         bool IsPlaceForEvent(uint32_t subeventsize);
         bool NewEvent(uint32_t seqid = 0, uint32_t trig_type = 0, uint32_t trig_number = 0, uint32_t minsubeventssize = 0);
         bool NewSubevent(uint32_t minrawsize = 0, uint32_t type_number = 0);
         bool FinishSubEvent(uint32_t rawdatasz);

         bool AddSubevent(const dabc::Pointer &source);
         bool AddSubevent(const void *ptr, unsigned len);
         bool AddSubevent(dogma::DogmaTu* sub)
         {
            return AddSubevent(sub, sub->GetSize());
         }
         bool AddAllSubevents(dogma::DogmaEvent *evnt)
         {
            return AddSubevent(evnt->FirstSubevent(), evnt->GetPayloadLen());
         }

         bool CopyEvent(const ReadIterator &iter);

         bool FinishEvent();

         dabc::Buffer Close();

         dogma::DogmaEvent* evnt() const { return (dogma::DogmaEvent*) fEvPtr(); }
         dogma::DogmaTu* subevnt() const { return (dogma::DogmaTu*) fSubPtr(); }
         void* rawdata() const { return subevnt() ? subevnt()->RawData() : nullptr; }
         uint32_t maxrawdatasize() const { return fSubPtr.null() ? 0 : fSubPtr.fullsize() - sizeof(dogma::DogmaTu); }

      protected:
         dabc::Buffer   fBuffer; // here we keep buffer - mean ownership is delivered to iterator
         dabc::Pointer  fEvPtr;
         dabc::Pointer  fSubPtr;
         dabc::BufferSize_t fFullSize;
         bool fWasStarted{false};  // indicates if events writing was started after buffer assign
         bool fHasSubevents{false};  // indicates if any subevent was provided
   };

   // _______________________________________________________________________________________________

      class RawIterator : public dabc::EventsIterator {
      protected:
         ReadIterator fIter;

      public:
         RawIterator(const std::string &name) : dabc::EventsIterator(name) {}
         ~RawIterator() override {}

         bool Assign(const dabc::Buffer& buf) override { return fIter.Reset(buf); }
         void Close() override { return fIter.Close(); }

         bool NextEvent() override { return fIter.NextSubeventsBlock(); }
         void *Event() override { return fIter.block(); }
         dabc::BufferSize_t EventSize() override { return fIter.blocksize(); }
         unsigned EventKind() const override { return fIter.IsTu() ? 1 : (fIter.IsEvent() ? 2 : 0); }
   };

}

#endif
