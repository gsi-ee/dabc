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

#include "mbs/Iterator.h"

#include "dabc/logging.h"

mbs::ReadIterator::ReadIterator() :
   fFirstEvent(false),
   fEvPtr(),
   fSubPtr(),
   fRawPtr()
{
}

mbs::ReadIterator::ReadIterator(const dabc::Buffer& buf) :
   fFirstEvent(false),
   fEvPtr(),
   fSubPtr(),
   fRawPtr()
{
   Reset(buf);
}

mbs::ReadIterator::ReadIterator(const ReadIterator& src) :
   fFirstEvent(src.fFirstEvent),
   fEvPtr(src.fEvPtr),
   fSubPtr(src.fSubPtr),
   fRawPtr(src.fRawPtr)
{
}

mbs::ReadIterator& mbs::ReadIterator::operator=(const ReadIterator& src)
{
   fFirstEvent = src.fFirstEvent;
   fEvPtr = src.fEvPtr;
   fSubPtr = src.fSubPtr;
   fRawPtr = src.fRawPtr;

   return *this;
}

bool mbs::ReadIterator::Reset(const dabc::Buffer& buf)
{
   Close();

   if (buf.null()) return false;

   if (buf.GetTypeId() != mbt_MbsEvents) {
      EOUT(("Only buffer format mbt_MbsEvents is supported"));
      return false;
   }

   fEvPtr = buf;
   fFirstEvent = true;

   return true;
}

void mbs::ReadIterator::Close()
{
   fEvPtr.reset();
   fSubPtr.reset();
   fRawPtr.reset();
   fFirstEvent = false;
}

bool mbs::ReadIterator::NextEvent()
{
   if (fEvPtr.null()) return false;

   if (fFirstEvent)
      fFirstEvent = false;
   else
      fEvPtr.shift(evnt()->FullSize());

   if (fEvPtr.fullsize() < sizeof(EventHeader)) {
      fEvPtr.reset();
      return false;
   }

   if (fEvPtr.rawsize() < sizeof(EventHeader)) {
      EOUT(("Raw size less than event header - not supported !!!!"));

      fEvPtr.reset();
      return false;
   }

   if (fEvPtr.fullsize() < evnt()->FullSize()) {
      EOUT(("Error in MBS format - declared event size %u smaller than actual portion in buffer %u",
            (unsigned) evnt()->FullSize(), (unsigned) fEvPtr.fullsize()));
      fEvPtr.reset();
      return false;
   }

   fSubPtr.reset();
   fRawPtr.reset();

   return true;
}

unsigned mbs::ReadIterator::GetEventSize() const
{
   if (evnt()==0) return 0;
   unsigned res = evnt()->FullSize();
   return res < fEvPtr.fullsize() ? res : fEvPtr.fullsize();

}

dabc::Pointer mbs::ReadIterator::GetEventPointer()
{
   dabc::Pointer res;
   AssignEventPointer(res);
   return res;
}

dabc::Pointer mbs::ReadIterator::GetSubeventsPointer()
{
   dabc::Pointer res;
   AssignEventPointer(res);
   if (!res.null()) res.shift(sizeof(mbs::EventHeader));
   return res;
}


bool mbs::ReadIterator::AssignEventPointer(dabc::Pointer& ptr)
{
   if (fEvPtr.null()) {
      ptr.reset();
      return false;
   }
   ptr.reset(fEvPtr, 0, evnt()->FullSize());
   return true;
}


bool mbs::ReadIterator::NextSubEvent()
{
   if (fSubPtr.null()) {
      if (fEvPtr.null()) return false;
      if (evnt()->FullSize() < sizeof(EventHeader)) {
         EOUT(("Mbs format error - event fullsize %u too small", evnt()->FullSize()));
         return false;
      }
      fSubPtr.reset(fEvPtr, 0, evnt()->FullSize());
      fSubPtr.shift(sizeof(EventHeader));
   } else
      fSubPtr.shift(subevnt()->FullSize());

   if (fSubPtr.fullsize() < sizeof(SubeventHeader)) {
      fSubPtr.reset();
      return false;
   }

   if (subevnt()->FullSize() < sizeof(SubeventHeader)) {
      EOUT(("Mbs format error - subevent fullsize too small"));
      fSubPtr.reset();
      return false;
   }

   fRawPtr.reset(fSubPtr, 0, subevnt()->FullSize());
   fRawPtr.shift(sizeof(SubeventHeader));

   return true;
}

unsigned mbs::ReadIterator::NumEvents(const dabc::Buffer& buf)
{
   ReadIterator iter(buf);
   unsigned cnt(0);
   while (iter.NextEvent()) cnt++;
   return cnt;
}

// ===================================================================

mbs::WriteIterator::WriteIterator() :
   fBuffer(),
   fEvPtr(),
   fSubPtr(),
   fFullSize(0)
{
}

mbs::WriteIterator::WriteIterator(const dabc::Buffer& buf) :
   fBuffer(),
   fEvPtr(),
   fSubPtr(),
   fFullSize(0)
{
   Reset(buf);
}

mbs::WriteIterator::~WriteIterator()
{
   Close();
}

bool mbs::WriteIterator::Reset(const dabc::Buffer& buf)
{
   fBuffer.Release();
   fEvPtr.reset();
   fSubPtr.reset();
   fFullSize = 0;

   if (buf.GetTotalSize() < sizeof(EventHeader) + sizeof(SubeventHeader)) {
      EOUT(("Buffer too small for just empty MBS event"));
      return false;
   }

   fBuffer = buf;
   fBuffer.SetTypeId(mbt_MbsEvents);
   fEvPtr = fBuffer;
   return true;
}

dabc::Buffer mbs::WriteIterator::Close()
{
   if (!fSubPtr.null()) FinishEvent();
   fEvPtr.reset();
   if ((fFullSize>0) && (fBuffer.GetTotalSize() >= fFullSize))
      fBuffer.SetTotalSize(fFullSize);
   fFullSize = 0;

   return fBuffer.HandOver();
}

bool mbs::WriteIterator::IsPlaceForEvent(uint32_t subeventssize)
{
   return fEvPtr.fullsize() >= (sizeof(EventHeader) + subeventssize);
}


bool mbs::WriteIterator::NewEvent(EventNumType event_number, uint32_t minsubeventssize)
{
   if (!fSubPtr.null()) {
      EOUT(("Previous event not closed"));
      return false;
   }

   if (fEvPtr.null()) {
      EOUT(("No any place for new event"));
      return false;
   }

   if (fEvPtr.fullsize() < (sizeof(EventHeader) + minsubeventssize)) {
      EOUT(("Too few place for the new event"));
      return false;
   }

   fSubPtr.reset(fEvPtr, sizeof(EventHeader));

   evnt()->Init(event_number);

   return true;
}

bool mbs::WriteIterator::NewSubevent(uint32_t minrawsize, uint8_t crate, uint16_t procid, uint8_t control)
{
   if (fSubPtr.null()) return false;

   if (fSubPtr.fullsize() < (sizeof(SubeventHeader) + minrawsize)) return false;

   subevnt()->Init(crate, procid, control);

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
   if (fSubPtr.null()) return false;

   if (fSubPtr.fullsize() < source.fullsize()) return false;

   fSubPtr.copyfrom(source);

   // we expect here, that subevent has correct length
   fSubPtr.shift(source.fullsize());

   return true;
}

bool mbs::WriteIterator::AddSubevent(mbs::SubeventHeader* sub)
{
   return AddSubevent(dabc::Pointer(sub, sub->FullSize()));
}

bool mbs::WriteIterator::FinishEvent()
{
   if (fEvPtr.null() || fSubPtr.null()) return false;

   dabc::BufferSize_t dist = fEvPtr.distance_to(fSubPtr);
   evnt()->SetFullSize(dist);
   fEvPtr.shift(dist);
   fSubPtr.reset();
   fFullSize += dist;

   return true;
}

bool mbs::WriteIterator::CopyEventFrom(const dabc::Pointer& ptr, bool finish)
{
   if (!fSubPtr.null()) {
      EOUT(("Previous event not closed"));
      return false;
   }

   if (fEvPtr.null()) {
      EOUT(("No any place for new event"));
      return false;
   }

   if (fEvPtr.fullsize() < ptr.fullsize()) {
      EOUT(("Too few place for the new event"));
      return false;
   }

   fEvPtr.copyfrom(ptr);

   fSubPtr.reset(fEvPtr, ptr.fullsize());

   if (finish) return FinishEvent();

   return true;
}
