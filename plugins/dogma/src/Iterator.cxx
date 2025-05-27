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

#include "dogma/Iterator.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"


dogma::ReadIterator::ReadIterator(const ReadIterator& src) :
   fBuffer(src.fBuffer),
   fBufType(src.fBufType),
   fFirstEvent(src.fFirstEvent),
   fEvPtr(src.fEvPtr),
   fEvPtrLen(src.fEvPtrLen),
   fSubPtr(src.fSubPtr),
   fSubPtrLen(src.fSubPtrLen),
   fRawPtr(src.fRawPtr),
   fRawPtrLen(src.fRawPtrLen)
{
}

dogma::ReadIterator& dogma::ReadIterator::operator=(const ReadIterator& src)
{
   fBuffer = src.fBuffer;
   fBufType = src.fBufType;
   fFirstEvent = src.fFirstEvent;
   fEvPtr = src.fEvPtr;
   fEvPtrLen = src.fEvPtrLen;
   fSubPtr = src.fSubPtr;
   fSubPtrLen = src.fSubPtrLen;
   fRawPtr = src.fRawPtr;
   fRawPtrLen = src.fRawPtrLen;

   return *this;
}

bool dogma::ReadIterator::Reset(const dabc::Buffer& buf)
{
   Close();

   if (buf.null())
      return false;

   if (buf.NumSegments() > 1) {
      EOUT("Cannot work with segmented buffer - exit");
      dabc::mgr.StopApplication();
      return false;
   }

   fBuffer = buf;

   fBufType = fBuffer.GetTypeId();

   if ((fBufType != mbt_DogmaEvents) && (fBufType != mbt_DogmaTransportUnit) && (fBufType != mbt_DogmaSubevents)) {
      EOUT("Buffer format %u not supported", (unsigned) fBufType);
      return false;
   }

   fFirstEvent = true;

   fEvPtr = (unsigned char *) fBuffer.SegmentPtr(0);
   fEvPtrLen = fBuffer.SegmentSize(0);

   return true;
}

bool dogma::ReadIterator::ResetOwner(dabc::Buffer& buf)
{
   Close();

   if (buf.null())
      return false;

   if (buf.NumSegments() > 1) {
      EOUT("Cannot work with segmented buffer - exit");
      dabc::mgr.StopApplication();
      return false;
   }

   fBuffer << buf;

   fBufType = fBuffer.GetTypeId();

   if ((fBufType != mbt_DogmaEvents) && (fBufType != mbt_DogmaTransportUnit) && (fBufType != mbt_DogmaSubevents)) {
      EOUT("Buffer format %u not supported", (unsigned) fBufType);
      return false;
   }

   fFirstEvent = true;

   fEvPtr = (unsigned char *) fBuffer.SegmentPtr(0);
   fEvPtrLen = fBuffer.SegmentSize(0);

   return true;
}


void dogma::ReadIterator::Close()
{
   ResetEvPtr();
   fSubPtr = nullptr;
   fSubPtrLen = 0;
   fRawPtr = nullptr;
   fRawPtrLen = 0;
   fFirstEvent = false;
}

bool dogma::ReadIterator::NextTu()
{
   if (fBufType != mbt_DogmaTransportUnit) {
      EOUT("NextTu only allowed for buffer type mbt_DogmaTransportUnit. Check your code!");
      return false;
   }

   if (!fEvPtr)
      return false;

   if (fFirstEvent)
      fFirstEvent = false;
   else
      ShiftEvPtr(tu()->GetSize());

   if (fEvPtrLen < sizeof(dogma::DogmaTu)) {
      ResetEvPtr();
      return false;
   }

   if (fEvPtrLen < tu()->GetSize()) {
      EOUT("Error in DOGMA format - declared event size %u smaller than actual portion in buffer %u",
            (unsigned) tu()->GetSize(), (unsigned) fEvPtrLen);
      ResetEvPtr();
      return false;
   }

   fSubPtr = nullptr;
   fSubPtrLen = 0;
   fRawPtr = nullptr;
   fRawPtrLen = 0;

   return true;
}

bool dogma::ReadIterator::NextEvent()
{
   if (!fEvPtr)
      return false;

   if (fBufType != mbt_DogmaEvents) {
      EOUT("NextEvent only allowed for buffer type mbt_DogmaEvents. Check your code!");
      return false;
   }

   if (fFirstEvent)
      fFirstEvent = false;
   else
      ShiftEvPtr(evnt()->GetEventLen());

   if (fEvPtrLen < sizeof(dogma::DogmaEvent)) {
      ResetEvPtr();
      return false;
   }

   if (fEvPtrLen < evnt()->GetEventLen()) {
      EOUT("Error in DOGMA format - declared event size %u bigger than actual portion in buffer %u",
            (unsigned) evnt()->GetEventLen(), (unsigned) fEvPtrLen);
      ResetEvPtr();
      return false;
   }

   fSubPtr = nullptr;
   fSubPtrLen = 0;
   fRawPtr = nullptr;
   fRawPtrLen = 0;

   return true;
}

bool dogma::ReadIterator::NextSubeventsBlock()
{
   if (fBufType == mbt_DogmaTransportUnit)
      return NextTu();

   if (fBufType == mbt_DogmaEvents)
      return NextEvent();

   if (fBufType == mbt_DogmaSubevents) {
      // here only subevents
      if (!fFirstEvent) {
         fEvPtr = nullptr;
         fEvPtrLen = 0;
      }
      if (!fEvPtr)
         return false;
      fFirstEvent = false;
      fSubPtr = nullptr;
      fSubPtrLen = 0;
      fRawPtr = nullptr;
      fRawPtrLen = 0;
      return true;
   }

   return false;
}

void *dogma::ReadIterator::block() const
{
   if (fBufType == mbt_DogmaTransportUnit)
      return tu();

   if (fBufType == mbt_DogmaEvents)
      return evnt();

   return nullptr;
}

unsigned dogma::ReadIterator::blocksize() const
{
   if (fBufType == mbt_DogmaTransportUnit)
      return tusize();

   if (fBufType == mbt_DogmaEvents)
      return evntsize();

   return 0;
}


bool dogma::ReadIterator::AssignEventPointer(dabc::Pointer& ptr)
{
   if (!fEvPtr) {
      ptr.reset();
      return false;
   }
   ptr.reset(fEvPtr, evnt()->GetEventLen());
   return true;
}


bool dogma::ReadIterator::NextSubEvent()
{
   if (!fSubPtr) {
      if (!fEvPtr)
         return false;
      // this function is used both in hadtu and in event mode. Check out mode:
      dabc::BufferSize_t headsize = 0, containersize = 0;
      if (fBufType == mbt_DogmaTransportUnit) {
         // headsize = 0;
         containersize = tu()->GetSize();
      } else if (fBufType == mbt_DogmaEvents) {
         headsize = sizeof(dogma::DogmaEvent);
         containersize = evnt()->GetEventLen();
      } else if (fBufType == mbt_DogmaSubevents) {
         // headsize = 0;
         containersize = fEvPtrLen;
      } else {
         EOUT("NextSubEvent not allowed for buffer type %u. Check your code!", (unsigned) fBufType);
         return false;
      }

      if (containersize == 0)
         return false; // typical problem of artificial generated events

      if (containersize < headsize) {
         EOUT("DOGMA format error - tu container fullsize %u too small", (unsigned) containersize);
         return false;
      }
      fSubPtr = fEvPtr;
      fSubPtrLen = containersize;
      if (headsize > 0)
         ShiftSubPtr(headsize);
   } else {
      ShiftSubPtr(subevnt()->GetSize());
   }

   if (fSubPtrLen < sizeof(dogma::DogmaTu)) {
      fSubPtr = nullptr;
      fSubPtrLen = 0;
      return false;
   }

   fRawPtrLen = subevnt()->GetSize();

   if (fRawPtrLen < sizeof(dogma::DogmaTu)) {
      EOUT("Dogma format error - subevent fullsize %u too small", fRawPtrLen);
      fSubPtr = nullptr;
      fSubPtrLen = 0;
      fRawPtr = nullptr;
      fRawPtrLen = 0;
      return false;
   }

   fRawPtr = fSubPtr;

   ShiftRawPtr(sizeof(dogma::DogmaTu));

   return true;
}

unsigned dogma::ReadIterator::rawdata_maxsize() const
{
   unsigned sz0 = fEvPtrLen, sz1 = 0;
   if (fSubPtr && fEvPtr)
      sz1 = fSubPtr - fEvPtr;
   return sz0 > sz1 ? sz0 - sz1 : 0;
}

unsigned dogma::ReadIterator::NumEvents(const dabc::Buffer& buf)
{
   ReadIterator iter(buf);
   unsigned cnt = 0;
   while (iter.NextEvent())
      cnt++;
   return cnt;
}

// ===================================================================

dogma::WriteIterator::WriteIterator() :
   fBuffer(),
   fEvPtr(),
   fSubPtr(),
   fFullSize(0)
{
}

dogma::WriteIterator::WriteIterator(const dabc::Buffer& buf) :
   fBuffer(),
   fEvPtr(),
   fSubPtr(),
   fFullSize(0)
{
   Reset(buf);
}

dogma::WriteIterator::~WriteIterator()
{
   Close();
}

bool dogma::WriteIterator::Reset(const dabc::Buffer& buf)
{
   fBuffer.Release();
   fEvPtr.reset();
   fSubPtr.reset();
   fFullSize = 0;

   if (buf.GetTotalSize() < sizeof(dogma::DogmaEvent) + sizeof(dogma::DogmaTu)) {
      EOUT("Buffer too small for just empty DOGMA event");
      return false;
   }

   fBuffer = buf;

   fBuffer.SetTypeId(dogma::mbt_DogmaEvents);
   fWasStarted = false;

   return true;
}

dabc::Buffer dogma::WriteIterator::Close()
{
   fEvPtr.reset();
   fSubPtr.reset();
   if ((fFullSize > 0) && (fBuffer.GetTotalSize() >= fFullSize))
      fBuffer.SetTotalSize(fFullSize);
   fFullSize = 0;

   dabc::Buffer res;
   res << fBuffer;
   return res;
}

bool dogma::WriteIterator::IsPlaceForEvent(uint32_t subeventssize)
{
   dabc::BufferSize_t availible = 0;

   if (!fEvPtr.null())
      availible = fEvPtr.fullsize();
   else if (!fWasStarted)
      availible = fBuffer.GetTotalSize();

   return availible >= (sizeof(dogma::DogmaEvent) + subeventssize);
}


bool dogma::WriteIterator::NewEvent(uint32_t seqid, uint32_t trig_type, uint32_t trig_number, uint32_t minsubeventssize)
{
   // TODO: add arguments to set other event header fields
   if (fBuffer.null()) return false;

   if (fEvPtr.null()) {
      if (fWasStarted)
         return false;
      fEvPtr = fBuffer;
      fWasStarted = true;
   }

   fSubPtr.reset();

   if (fEvPtr.fullsize() < (sizeof(dogma::DogmaEvent) + minsubeventssize)) {
      fEvPtr.reset();
      return false;
   }

   evnt()->Init(seqid, trig_type, trig_number);

   fHasSubevents = false;

   return true;
}

bool dogma::WriteIterator::NewSubevent(uint32_t minrawsize, uint32_t type_number)
{
   if (fEvPtr.null())
      return false;

   if (fSubPtr.null())
      fSubPtr.reset(fEvPtr, sizeof(dogma::DogmaEvent));

   if (fSubPtr.fullsize() < (sizeof(dogma::DogmaTu) + minrawsize))
      return false;

   subevnt()->Init(type_number);

   return true;
}

bool dogma::WriteIterator::FinishSubEvent(uint32_t rawdatasz)
{
   if (fSubPtr.null())
      return false;

   if (rawdatasz > maxrawdatasize())
      return false;

   if (rawdatasz % 4 != 0)
      EOUT("Failure to set payload length 0x%x", (unsigned) rawdatasz);

   subevnt()->SetPayloadLen(rawdatasz / 4);

   fSubPtr.shift(subevnt()->GetSize());

   fHasSubevents = true;

   return true;
}

bool dogma::WriteIterator::AddSubevent(const dabc::Pointer& source)
{
   if (fEvPtr.null()) return false;

   if (fSubPtr.null())
      fSubPtr.reset(fEvPtr, sizeof(dogma::DogmaEvent));

   if (fSubPtr.fullsize() < source.fullsize()) return false;

   fSubPtr.copyfrom(source);

   fSubPtr.shift(source.fullsize());

   fHasSubevents = true;

   return true;
}

bool dogma::WriteIterator::AddSubevent(const void *ptr, unsigned len)
{
   if (fEvPtr.null())
      return false;

   if (fSubPtr.null())
      fSubPtr.reset(fEvPtr, sizeof(dogma::DogmaEvent));

   if (fSubPtr.fullsize() < len)
      return false;

   fSubPtr.copyfrom(ptr, len);

   fSubPtr.shift(len);

   fHasSubevents = true;

   return true;
}

bool dogma::WriteIterator::FinishEvent()
{
   if (fEvPtr.null())
      return false;

   dabc::BufferSize_t dist = sizeof(dogma::DogmaEvent);
   if (!fSubPtr.null())
      dist = fEvPtr.distance_to(fSubPtr);
   else if (fHasSubevents)
      dist = fEvPtr.fullsize(); // special case when exactly buffer was matched

   if (dist < sizeof(dogma::DogmaEvent)) {
      EOUT("DOGMA event size too small %u", dist);
      dist = sizeof(dogma::DogmaEvent);
   }

   dist -= sizeof(dogma::DogmaEvent);

   if (dist % 4 != 0)
      EOUT("DOGMA event paylod 0x%x is not 4 bytes aligned", (unsigned) dist);

   evnt()->SetPayloadLen(dist / 4);

   dabc::BufferSize_t paddeddist = evnt()->GetEventLen();
   fFullSize += paddeddist;
   fEvPtr.shift(paddeddist);
   fHasSubevents = false;

   return true;
}

bool dogma::WriteIterator::CopyEvent(const dogma::ReadIterator &iter)
{
   if (fEvPtr.null()) {
      if (fWasStarted) return false;
      fEvPtr = fBuffer;
      fWasStarted = true;
   }

   auto size = iter.evntsize();

   if (!iter.evnt() || (size == 0) || (fEvPtr.fullsize() < size)) return false;

   fEvPtr.copyfrom(iter.evnt(), size);
   fFullSize += size;
   fEvPtr.shift(size);

   return true;
}
