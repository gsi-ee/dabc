#ifndef BNET_testing
#define BNET_testing

#ifndef DABC_eventsapi
#include "dabc/eventsapi.h"
#endif

namespace bnet {


   enum ETestBufferTypes {
      TestEvents   = 177   // several test events
   };


   /** \brief Iterator for test events
    *
    * Allows to iterate over test events
    */

   class TestEventsIterator : public dabc::EventsIterator {
      protected:
         dabc::Pointer fPtr;
         bool fFirstEvent{false};

      public:
         TestEventsIterator(const std::string &name) : dabc::EventsIterator(name), fPtr(), fFirstEvent(true) {}
         virtual ~TestEventsIterator() {}

         bool Assign(const dabc::Buffer& buf) override
         {
            if (buf.GetTypeId() != TestEvents) return false;
            fPtr = buf;
            fFirstEvent = true;
            return true;
         }
         void Close() override { fPtr.reset(); }

         bool NextEvent() override
         {
            if (fFirstEvent)
               fFirstEvent = false;
            else {
               unsigned sz = EventSize();
               if ((sz == 0) || (sz <= fPtr.fullsize()))
                  return false;
               fPtr.shift(sz);
            }
            return EventSize() > 0;
         }

         void* Event() override
         {
            return fPtr();
         }

         dabc::BufferSize_t EventSize() override
         {
            if (fPtr.null() || fPtr.fullsize()<3*sizeof(uint64_t)) return 0;
            uint64_t arr[3];
            fPtr.copyto(arr, sizeof(arr));
            return arr[2] + sizeof(arr);
         }
   };

   // ----------------------------------------------------------------------------------------


   /** \brief Producer of test events
    *
    * Allows to produce/generate test events in the buffer
    */

   class TestEventsProducer : public dabc::EventsProducer {
      protected:
         dabc::Buffer fBuf;
         dabc::Pointer fPtr;
      public:
         TestEventsProducer(const std::string &name) : dabc::EventsProducer(name), fBuf(), fPtr() {}
         virtual ~TestEventsProducer() {}

         bool Assign(const dabc::Buffer& buf) override
         {
            fBuf = buf;
            fPtr = buf;
            fBuf.SetTypeId(TestEvents);
            return true;
         }

         void Close() override
         {
            fBuf.SetTotalSize(fPtr.distance_to_ownbuf());
            fPtr.reset();
            fBuf.Release();
         }

         bool GenerateEvent(uint64_t evid, uint64_t subid, uint64_t raw_size) override
         {
            while (raw_size % 8 != 0) raw_size++;
            unsigned total_size = 3*sizeof(uint64_t) + raw_size;
            if (total_size > fPtr.fullsize()) return false;
            fPtr.copyfrom_shift(&evid, sizeof(evid));
            fPtr.copyfrom_shift(&subid, sizeof(subid));
            fPtr.copyfrom_shift(&raw_size, sizeof(raw_size));
            fPtr.shift(raw_size);
            return true;
         }
   };

}

#endif
