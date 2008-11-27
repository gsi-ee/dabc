#ifndef MBS_Iterator
#define MBS_Iterator

#ifndef MBS_MbsTypeDefs
#include "mbs/MbsTypeDefs.h"
#endif

#ifndef DABC_Pointer
#include "dabc/Pointer.h"
#endif

namespace mbs {

   class ReadIterator {
      public:
         ReadIterator(dabc::Buffer* buf = 0);

         ReadIterator(const ReadIterator& src);

         bool Reset(dabc::Buffer* buf);

         bool IsOk() const { return fBuffer!=0; }

         bool NextEvent();
         bool NextSubEvent();

         EventHeader* evnt() const { return (EventHeader*) fEvPtr(); }
         bool AssignEventPointer(dabc::Pointer& ptr);
         SubeventHeader* subevnt() const { return (SubeventHeader*) fSubPtr(); }
         void* rawdata() const { return fRawPtr(); }
         uint32_t rawdatasize() const { return subevnt() ? subevnt()->RawDataSize() : 0; }

         static unsigned NumEvents(dabc::Buffer* buf);

      protected:
         dabc::Buffer*    fBuffer;
         dabc::Pointer    fEvPtr;
         dabc::Pointer    fSubPtr;
         dabc::Pointer    fRawPtr;
   };


   class WriteIterator {
      public:
         WriteIterator(dabc::Buffer* buf);
         ~WriteIterator();

         bool Reset(dabc::Buffer* buf);

         bool IsPlaceForEvent(uint32_t subeventsize);
         bool NewEvent(EventNumType event_number = 0, uint32_t subeventsize = 0);
         bool NewSubevent(uint32_t minrawsize = 0, uint8_t crate = 0, uint16_t procid = 0);
         bool FinishSubEvent(uint32_t rawdatasz);

         bool AddSubevent(const dabc::Pointer& source);
         bool FinishEvent();

         void Close();

         EventHeader* evnt() const { return (EventHeader*) fEvPtr(); }
         SubeventHeader* subevnt() const { return (SubeventHeader*) fSubPtr(); }
         void* rawdata() const { return subevnt() ? subevnt()->RawData() : 0; }
         uint32_t maxrawdatasize() const { return fSubPtr.null() ? 0 : fSubPtr.fullsize() - sizeof(SubeventHeader); }


      protected:
         dabc::Buffer*    fBuffer;
         dabc::Pointer    fEvPtr;
         dabc::Pointer    fSubPtr;
         dabc::BufferSize_t fFullSize;
   };


}







#endif
