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
   fBuffer(0),
   fEvPtr(),
   fSubPtr(),
   fRawPtr()
{
}

mbs::ReadIterator::ReadIterator(const dabc::Buffer& buf) :
   fBuffer(0),
   fEvPtr(),
   fSubPtr(),
   fRawPtr()
{
   Reset(buf);
}

mbs::ReadIterator::ReadIterator(const ReadIterator& src) :
   fBuffer(src.fBuffer),
   fEvPtr(src.fEvPtr),
   fSubPtr(src.fSubPtr),
   fRawPtr(src.fRawPtr)
{
}

mbs::ReadIterator& mbs::ReadIterator::operator=(const ReadIterator& src)
{
   fBuffer = src.fBuffer;
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

   fBuffer = &buf;

   return true;
}

void mbs::ReadIterator::Close()
{
   fEvPtr.reset();
   fSubPtr.reset();
   fRawPtr.reset();

   fBuffer = 0;
}

bool mbs::ReadIterator::NextEvent()
{
   if (fBuffer==0) return false;

   if (fEvPtr.null())
      fEvPtr = fBuffer->GetPointer();
   else
      fBuffer->Shift(fEvPtr, evnt()->FullSize());

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
            evnt()->FullSize(), fEvPtr.fullsize()));
      fEvPtr.reset();
      return false;
   }

   fSubPtr.reset();
   fRawPtr.reset();

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
      if (evnt()->FullSize() < sizeof(EventHeader)) {
         EOUT(("Mbs format error - event fullsize %u too small", evnt()->FullSize()));
         return false;
      }
      fSubPtr.reset(fEvPtr, evnt()->FullSize());
      fBuffer->Shift(fSubPtr, sizeof(EventHeader));
   } else
      fBuffer->Shift(fSubPtr, subevnt()->FullSize());

   if (fSubPtr.fullsize() < sizeof(SubeventHeader)) {
      fSubPtr.reset();
      return false;
   }

   if (subevnt()->FullSize() < sizeof(SubeventHeader)) {
      EOUT(("Mbs format error - subevent fullsize too small"));
      fSubPtr.reset();
      return false;
   }

   fRawPtr.reset(fSubPtr, subevnt()->FullSize());
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
   return true;
}

dabc::Buffer mbs::WriteIterator::Close()
{
   fEvPtr.reset();
   fSubPtr.reset();
   if ((fFullSize>0) && (fBuffer.GetTotalSize() >= fFullSize))
      fBuffer.SetTotalSize(fFullSize);
   fFullSize = 0;

   dabc::Buffer res;
   res << fBuffer;

   return res;
}

bool mbs::WriteIterator::IsPlaceForEvent(uint32_t subeventssize)
{
   dabc::BufferSize_t availible = 0;

   if (!fEvPtr.null()) availible = fEvPtr.fullsize();
                 else  availible = fBuffer.GetTotalSize();

   return availible >= (sizeof(EventHeader) + subeventssize);
}


bool mbs::WriteIterator::NewEvent(EventNumType event_number, uint32_t minsubeventssize)
{
   if (fBuffer.null()) return false;

   if (fEvPtr.null())
      fEvPtr = fBuffer.GetPointer();

   fSubPtr.reset();

   if (fEvPtr.fullsize() < (sizeof(EventHeader) + minsubeventssize)) {
      fEvPtr.reset();
      return false;
   }

   evnt()->Init(event_number);

   return true;
}

bool mbs::WriteIterator::NewSubevent(uint32_t minrawsize, uint8_t crate, uint16_t procid, uint8_t control)
{
   if (fEvPtr.null()) return false;

   if (fSubPtr.null()) {
      fSubPtr.reset(fEvPtr);
      fBuffer.Shift(fSubPtr, sizeof(EventHeader));
   }

   if (fSubPtr.fullsize() < (sizeof(SubeventHeader) + minrawsize)) return false;

   subevnt()->Init(crate, procid, control);

   return true;
}

bool mbs::WriteIterator::FinishSubEvent(uint32_t rawdatasz)
{
   if (fSubPtr.null()) return false;

   if (rawdatasz > maxrawdatasize()) return false;

   subevnt()->SetRawDataSize(rawdatasz);

   fBuffer.Shift(fSubPtr, subevnt()->FullSize());

   return true;
}

bool mbs::WriteIterator::AddSubevent(const dabc::Pointer& source, const dabc::Buffer* srcbuf)
{
   if (fEvPtr.null()) return false;

   if (fSubPtr.null()) {
      fSubPtr.reset(fEvPtr);
      fBuffer.Shift(fSubPtr, sizeof(EventHeader));
   }

   if (fSubPtr.fullsize() < source.fullsize()) return false;

   *((unsigned*)fSubPtr()) = 0xff00ff00;

   if (srcbuf==0)
      fBuffer.CopyFrom(fSubPtr, source);
   else
      fBuffer.CopyFrom(fSubPtr, *srcbuf, source);

   fBuffer.Shift(fSubPtr, source.fullsize());

   return true;
}

bool mbs::WriteIterator::AddSubevent(mbs::SubeventHeader* sub)
{
   dabc::Pointer ptr(sub, sub->FullSize());

   return AddSubevent(ptr);
}


bool mbs::WriteIterator::FinishEvent()
{
   if (fEvPtr.null()) return false;

   dabc::BufferSize_t dist = sizeof(EventHeader);
   if (!fSubPtr.null()) dist = fBuffer.Distance(fEvPtr, fSubPtr);
   evnt()->SetFullSize(dist);
   fFullSize += dist;
   fBuffer.Shift(fEvPtr,dist);

   return true;
}

