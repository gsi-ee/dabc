#include "mbs/Iterator.h"

#include "dabc/logging.h"

mbs::ReadIterator::ReadIterator(dabc::Buffer* buf) :
   fBuffer(0),
   fEvPtr(),
   fSubPtr(),
   fRawPtr()
{
   if (buf==0) return;

   if (buf->GetTypeId() != mbt_MbsEvs10_1) {
      EOUT(("Only buffer format mbt_MbsEvs10_1 are supported"));
      return;
   }
   fBuffer = buf;
}

mbs::ReadIterator::ReadIterator(const ReadIterator& src) :
   fBuffer(src.fBuffer),
   fEvPtr(src.fEvPtr),
   fSubPtr(src.fSubPtr),
   fRawPtr(src.fRawPtr)
{
}

bool mbs::ReadIterator::Reset(dabc::Buffer* buf)
{
   fBuffer = 0;
   fEvPtr.reset();
   fSubPtr.reset();
   fRawPtr.reset();
   if (buf==0) return false;

   if (buf->GetTypeId() != mbt_MbsEvs10_1) {
      EOUT(("Only buffer format mbt_MbsEvs10_1 is supported"));
      return false;
   }
   fBuffer = buf;
   return true;
}


bool mbs::ReadIterator::NextEvent()
{
   if (fEvPtr.null()) {
      if (fBuffer==0) return false;
      fEvPtr.reset(fBuffer);
   } else
      fEvPtr.shift(evnt()->FullSize());

   if (fEvPtr.fullsize() < sizeof(EventHeader)) {
      fEvPtr.reset();
      return false;
   }

   fSubPtr.reset();

   return true;
}

bool mbs::ReadIterator::AssignEventPointer(dabc::Pointer& ptr)
{
   if (fEvPtr.null()) {
      ptr.reset();
      return false;
   }
   ptr.reset(fEvPtr, evnt()->FullSize());
   return true;
}


bool mbs::ReadIterator::NextSubEvent()
{
   if (fSubPtr.null()) {
      if (fEvPtr.null()) return false;
      fSubPtr.reset(fEvPtr);
      fSubPtr.shift(sizeof(EventHeader));
   } else
      fSubPtr.shift(subevnt()->FullSize());

   if (fSubPtr.fullsize() < sizeof(SubeventHeader)) {
      fSubPtr.reset();
      return false;
   }

   fRawPtr.reset(fSubPtr);
   fRawPtr.shift(sizeof(SubeventHeader));

   return true;
}

// ________________________________________________________________________________

mbs::WriteIterator::WriteIterator(dabc::Buffer* buf) :
   fBuffer(buf),
   fEvPtr(),
   fSubPtr(),
   fFullSize(0)
{
   if (fBuffer==0) return;
   fBuffer->SetTypeId(mbt_MbsEvs10_1);
}

mbs::WriteIterator::~WriteIterator()
{
   Close();
}

bool mbs::WriteIterator::Reset(dabc::Buffer* buf)
{
   fBuffer = 0;
   fEvPtr.reset();
   fSubPtr.reset();
   fFullSize = 0;

   if (buf==0) return false;

   fBuffer = buf;
   fBuffer->SetTypeId(mbt_MbsEvs10_1);
   return true;
}

void mbs::WriteIterator::Close()
{
   fEvPtr.reset();
   fSubPtr.reset();
   if (fBuffer && (fFullSize>0))
      fBuffer->SetDataSize(fFullSize);
   fBuffer = 0;
   fFullSize = 0;
}

bool mbs::WriteIterator::IsPlaceForEvent(uint32_t subeventssize)
{
   dabc::BufferSize_t availible = 0;

   if (!fEvPtr.null()) availible = fEvPtr.fullsize(); else
   if (fBuffer) availible = fBuffer->GetDataSize();

   return availible >= (sizeof(EventHeader) + subeventssize);
}


bool mbs::WriteIterator::NewEvent(EventNumType event_number, uint32_t minsubeventssize)
{
   if (fBuffer == 0) return false;

   if (fEvPtr.null())
      fEvPtr.reset(fBuffer);

   fSubPtr.reset();

   if (fEvPtr.fullsize() < (sizeof(EventHeader) + minsubeventssize)) {
      fEvPtr.reset();
      return false;
   }

   evnt()->Init(event_number);

   return true;
}

bool mbs::WriteIterator::NewSubevent(uint32_t minrawsize, uint8_t crate, uint16_t procid)
{
   if (fEvPtr.null()) return false;

   if (fSubPtr.null()) {
      fSubPtr.reset(fEvPtr);
      fSubPtr.shift(sizeof(EventHeader));
   }

   if (fSubPtr.fullsize() < (sizeof(SubeventHeader) + minrawsize)) return false;

   subevnt()->Init(crate, procid);

   return true;
}

bool mbs::WriteIterator::FinishSubEvent(uint32_t rawdatasz)
{
   if (fSubPtr.null()) return false;

   if (rawdatasz > maxrawdatasize()) return false;

   subevnt()->SetRawDataSize(rawdatasz);

   fSubPtr.shift(subevnt()->FullSize());

   return true;
}

bool mbs::WriteIterator::AddSubevent(const dabc::Pointer& source)
{
   if (fEvPtr.null()) return false;

   if (fSubPtr.null()) {
      fSubPtr.reset(fEvPtr);
      fSubPtr.shift(sizeof(EventHeader));
   }

   if (fSubPtr.fullsize() < source.fullsize()) return false;

   fSubPtr.copyfrom(source, source.fullsize());

   fSubPtr.shift(source.fullsize());

   return true;
}

bool mbs::WriteIterator::FinishEvent()
{
   if (fEvPtr.null()) return false;

   dabc::BufferSize_t dist = sizeof(EventHeader);
   if (!fSubPtr.null()) dist = fEvPtr.distance_to(fSubPtr);
   evnt()->SetFullSize(dist);
   fFullSize += dist;
   fEvPtr.shift(dist);

   return true;
}
