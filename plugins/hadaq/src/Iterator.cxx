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

#include "hadaq/Iterator.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"


hadaq::ReadIterator::ReadIterator(const ReadIterator& src) :
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

hadaq::ReadIterator& hadaq::ReadIterator::operator=(const ReadIterator& src)
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

bool hadaq::ReadIterator::Reset(const dabc::Buffer& buf)
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

   if ((fBufType != mbt_HadaqEvents) && (fBufType != mbt_HadaqTransportUnit) && (fBufType != mbt_HadaqSubevents)) {
      EOUT("Buffer format %u not supported", (unsigned) fBufType);
      return false;
   }

   fFirstEvent = true;

   fEvPtr = (unsigned char *) fBuffer.SegmentPtr(0);
   fEvPtrLen = fBuffer.SegmentSize(0);

   return true;
}

bool hadaq::ReadIterator::ResetOwner(dabc::Buffer& buf)
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

   if ((fBufType != mbt_HadaqEvents) && (fBufType != mbt_HadaqTransportUnit) && (fBufType != mbt_HadaqSubevents)) {
      EOUT("Buffer format %u not supported", (unsigned) fBufType);
      return false;
   }

   fFirstEvent = true;

   fEvPtr = (unsigned char *) fBuffer.SegmentPtr(0);
   fEvPtrLen = fBuffer.SegmentSize(0);

   return true;
}


void hadaq::ReadIterator::Close()
{
   ResetEvPtr();
   fSubPtr = nullptr;
   fSubPtrLen = 0;
   fRawPtr = nullptr;
   fRawPtrLen = 0;
   fFirstEvent = false;
}


bool hadaq::ReadIterator::NextHadTu()
{
   if (fBufType != mbt_HadaqTransportUnit) {
      EOUT("NextHadTu only allowed for buffer type mbt_HadaqTransportUnit. Check your code!");
      return false;
   }

   if (!fEvPtr)
      return false;

   if (fFirstEvent)
      fFirstEvent = false;
   else
      ShiftEvPtr(hadtu()->GetPaddedSize());

   if (fEvPtrLen < sizeof(hadaq::HadTu)) {
      EOUT("Raw size less than transport unit header - not supported !!!!");
      ResetEvPtr();
      return false;
   }

   if (fEvPtrLen < hadtu()->GetSize()) {
      EOUT("Error in HADAQ format - declared event size %u smaller than actual portion in buffer %u",
            (unsigned) hadtu()->GetSize(), (unsigned) fEvPtrLen);
      ResetEvPtr();
      return false;
   }

   fSubPtr = nullptr;
   fSubPtrLen = 0;
   fRawPtr = nullptr;
   fRawPtrLen = 0;

   return true;
}


bool hadaq::ReadIterator::NextEvent()
{
   if (!fEvPtr)
      return false;

   if (fBufType != mbt_HadaqEvents) {
      EOUT("NextEvent only allowed for buffer type mbt_HadaqEvents. Check your code!");
      return false;
   }

   if (fFirstEvent)
      fFirstEvent = false;
   else
      ShiftEvPtr(evnt()->GetPaddedSize());

   if (fEvPtrLen < sizeof(hadaq::RawEvent)) {
      EOUT("Raw size less than event header - not supported !!!!");
      ResetEvPtr();
      return false;
   }

   if (fEvPtrLen < evnt()->GetSize()) {
      EOUT("Error in HADAQ format - declared event size %u bigger than actual portion in buffer %u",
            (unsigned) evnt()->GetSize(), (unsigned) fEvPtrLen);
      ResetEvPtr();
      return false;
   }

   fSubPtr = nullptr;
   fSubPtrLen = 0;
   fRawPtr = nullptr;
   fRawPtrLen = 0;

   return true;
}

bool hadaq::ReadIterator::NextSubeventsBlock()
{
   if (fBufType == mbt_HadaqTransportUnit)
      return NextHadTu();

   if (fBufType == mbt_HadaqEvents)
      return NextEvent();

   if (fBufType == mbt_HadaqSubevents) {
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


bool hadaq::ReadIterator::AssignEventPointer(dabc::Pointer& ptr)
{
   if (!fEvPtr) {
      ptr.reset();
      return false;
   }
   ptr.reset(fEvPtr, evnt()->GetPaddedSize());
   return true;
}


bool hadaq::ReadIterator::NextSubEvent()
{
   if (!fSubPtr) {
      if (!fEvPtr)
         return false;
      // this function is used both in hadtu and in event mode. Check out mode:
      dabc::BufferSize_t headsize = 0, containersize = 0;
      if (fBufType == mbt_HadaqEvents) {
         headsize = sizeof(hadaq::RawEvent);
         containersize = evnt()->GetPaddedSize();
      } else if (fBufType == mbt_HadaqTransportUnit) {
         headsize = sizeof(hadaq::HadTu);
         containersize = hadtu()->GetPaddedSize();
      } else if (fBufType == mbt_HadaqSubevents) {
         // headsize = 0;
         containersize = fEvPtrLen;
      } else {
         EOUT("NextSubEvent not allowed for buffer type %u. Check your code!", (unsigned) fBufType);
         return false;
      }

      if (containersize == 0)
         return false; // typical problem of artificial generated events

      if (containersize < headsize) {
         EOUT("Hadaq format error - tu container fullsize %u too small", (unsigned) containersize);
         return false;
      }
      fSubPtr = fEvPtr;
      fSubPtrLen = containersize;
      if (headsize > 0)
         ShiftSubPtr(headsize);
   } else {
      ShiftSubPtr(subevnt()->GetPaddedSize());
   }

   if (fSubPtrLen < sizeof(hadaq::RawSubevent)) {
      fSubPtr = nullptr;
      fSubPtrLen = 0;
      return false;
   }

   if (subevnt()->GetSize() < sizeof(hadaq::RawSubevent)) {
      EOUT("Hadaq format error - subevent fullsize %u too small", subevnt()->GetSize());
      fSubPtr = nullptr;
      fSubPtrLen = 0;
      return false;
   }

   fRawPtr = fSubPtr;
   fRawPtrLen = subevnt()->GetPaddedSize();
   ShiftRawPtr(sizeof(hadaq::RawSubevent));

   return true;
}

unsigned hadaq::ReadIterator::rawdata_maxsize() const
{
   unsigned sz0 = fEvPtrLen, sz1 = 0;
   if (fSubPtr && fEvPtr)
      sz1 = fSubPtr - fEvPtr;
   return sz0 > sz1 ? sz0 - sz1 : 0;
}

unsigned hadaq::ReadIterator::NumEvents(const dabc::Buffer& buf)
{
   ReadIterator iter(buf);
   unsigned cnt = 0;
   while (iter.NextEvent())
      cnt++;
   return cnt;
}

// ===================================================================

hadaq::WriteIterator::WriteIterator() :
   fBuffer(),
   fEvPtr(),
   fSubPtr(),
   fFullSize(0)
{
}

hadaq::WriteIterator::WriteIterator(const dabc::Buffer& buf) :
   fBuffer(),
   fEvPtr(),
   fSubPtr(),
   fFullSize(0)
{
   Reset(buf);
}

hadaq::WriteIterator::~WriteIterator()
{
   Close();
}

bool hadaq::WriteIterator::Reset(const dabc::Buffer& buf)
{
   fBuffer.Release();
   fEvPtr.reset();
   fSubPtr.reset();
   fFullSize = 0;

   if (buf.GetTotalSize() < sizeof(hadaq::RawEvent) + sizeof(hadaq::RawSubevent)) {
      EOUT("Buffer too small for just empty HADAQ event");
      return false;
   }

   fBuffer = buf;

   fBuffer.SetTypeId(mbt_HadaqEvents);
   fWasStarted = false;

   return true;
}

dabc::Buffer hadaq::WriteIterator::Close()
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

bool hadaq::WriteIterator::IsPlaceForEvent(uint32_t subeventssize)
{
   dabc::BufferSize_t availible = 0;

   if (!fEvPtr.null())
      availible = fEvPtr.fullsize();
   else if (!fWasStarted)
      availible = fBuffer.GetTotalSize();

   return availible >= (sizeof(hadaq::RawEvent) + subeventssize);
}


bool  hadaq::WriteIterator::NewEvent(uint32_t evtSeqNr, uint32_t runNr, uint32_t minsubeventssize)
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

   if (fEvPtr.fullsize() < (sizeof(hadaq::RawEvent) + minsubeventssize)) {
      fEvPtr.reset();
      return false;
   }

   evnt()->Init(evtSeqNr, runNr);

   fHasSubevents = false;

   return true;
}

bool hadaq::WriteIterator::NewSubevent(uint32_t minrawsize, uint32_t trigger)
{
   if (fEvPtr.null())
      return false;

   if (fSubPtr.null())
      fSubPtr.reset(fEvPtr, sizeof(hadaq::RawEvent));

   if (fSubPtr.fullsize() < (sizeof(hadaq::RawSubevent) + minrawsize))
      return false;

   subevnt()->Init(trigger);

   return true;
}

bool hadaq::WriteIterator::FinishSubEvent(uint32_t rawdatasz)
{
   if (fSubPtr.null())
      return false;

   if (rawdatasz > maxrawdatasize())
      return false;

   subevnt()->SetSize(rawdatasz + sizeof(hadaq::RawSubevent));

   fSubPtr.shift(subevnt()->GetPaddedSize());

   fHasSubevents = true;

   return true;
}

bool hadaq::WriteIterator::AddSubevent(const dabc::Pointer& source)
{
   if (fEvPtr.null()) return false;

   if (fSubPtr.null())
      fSubPtr.reset(fEvPtr, sizeof(hadaq::RawEvent));

   if (fSubPtr.fullsize() < source.fullsize()) return false;

   fSubPtr.copyfrom(source);

   fSubPtr.shift(source.fullsize());

   fHasSubevents = true;

   return true;
}

bool hadaq::WriteIterator::AddSubevent(const void *ptr, unsigned len)
{
   if (fEvPtr.null())
      return false;

   if (fSubPtr.null())
      fSubPtr.reset(fEvPtr, sizeof(hadaq::RawEvent));

   if (fSubPtr.fullsize() < len)
      return false;

   fSubPtr.copyfrom(ptr, len);

   fSubPtr.shift(len);

   fHasSubevents = true;

   return true;
}

bool hadaq::WriteIterator::FinishEvent()
{
   if (fEvPtr.null()) return false;

   dabc::BufferSize_t dist = sizeof(hadaq::RawEvent);
   if (!fSubPtr.null())
      dist = fEvPtr.distance_to(fSubPtr);
   else if (fHasSubevents)
      dist = fEvPtr.fullsize(); // special case when exactly buffer was matched
   evnt()->SetSize(dist);

   dabc::BufferSize_t paddeddist = evnt()->GetPaddedSize();
   fFullSize += paddeddist;
   fEvPtr.shift(paddeddist);
   fHasSubevents = false;

   return true;
}

bool hadaq::WriteIterator::CopyEvent(const hadaq::ReadIterator &iter)
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
