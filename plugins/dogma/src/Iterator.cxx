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


dogma::ReadIterator::ReadIterator(const ReadIterator& src) :
   fFirstEvent(src.fFirstEvent),
   fEvPtr(src.fEvPtr),
   fSubPtr(src.fSubPtr),
   fRawPtr(src.fRawPtr),
   fBufType(src.fBufType)
{
}

dogma::ReadIterator& dogma::ReadIterator::operator=(const ReadIterator& src)
{
   fFirstEvent = src.fFirstEvent;
   fEvPtr = src.fEvPtr;
   fSubPtr = src.fSubPtr;
   fRawPtr = src.fRawPtr;
   fBufType = src.fBufType;

   return *this;
}

bool dogma::ReadIterator::Reset(const dabc::Buffer& buf)
{
   Close();

   if (buf.null()) return false;

   fBufType = buf.GetTypeId();

   if ((fBufType != mbt_DogmaEvents) && (fBufType != mbt_DogmaTransportUnit) && (fBufType != mbt_DogmaSubevents)) {
      EOUT("Buffer format %u not supported", (unsigned) fBufType);
      return false;
   }

   fEvPtr = buf;
   fFirstEvent = true;

   return true;
}

void dogma::ReadIterator::Close()
{
   fEvPtr.reset();
   fSubPtr.reset();
   fRawPtr.reset();
   fFirstEvent = false;
   fBufType = dabc::mbt_Null;
}


bool dogma::ReadIterator::NextTu()
{
   if (fBufType != mbt_DogmaTransportUnit ) {
      EOUT("NextTu only allowed for buffer type mbt_DogmaTransportUnit. Check your code!");
      return false;
   }

   if (fEvPtr.null()) return false;

   if (fFirstEvent)
      fFirstEvent = false;
   else
      fEvPtr.shift(tu()->GetTuLen());

   if (fEvPtr.fullsize() < sizeof(dogma::DogmaTu)) {
      fEvPtr.reset();
      return false;
   }

   if (fEvPtr.rawsize() < sizeof(dogma::DogmaTu)) {
      EOUT("Raw size less than transport unit header - not supported !!!!");
      fEvPtr.reset();
      return false;
   }

   if (fEvPtr.fullsize() < tu()->GetTuLen()) {
      EOUT("Error in DOGMA format - declared event size %u smaller than actual portion in buffer %u",
            (unsigned) tu()->GetTuLen(), (unsigned) fEvPtr.fullsize());
      fEvPtr.reset();
      return false;
   }

   fSubPtr.reset();
   fRawPtr.reset();

   return true;
}


bool dogma::ReadIterator::NextEvent()
{
   if (fEvPtr.null()) return false;

   if (fBufType != mbt_DogmaEvents) {
      EOUT("NextEvent only allowed for buffer type mbt_DogmaEvents. Check your code!");
      return false;
   }

   if (fFirstEvent)
      fFirstEvent = false;
   else
      fEvPtr.shift(evnt()->GetEventLen());

   if (fEvPtr.fullsize() < sizeof(dogma::DogmaEvent)) {
      fEvPtr.reset();
      return false;
   }

   if (fEvPtr.rawsize() < sizeof(dogma::DogmaEvent)) {
      EOUT("Raw size less than event header - not supported !!!!");

      fEvPtr.reset();
      return false;
   }

   if (fEvPtr.fullsize() < evnt()->GetEventLen()) {
      EOUT("Error in DOGMA format - declared event size %u bigger than actual portion in buffer %u",
            (unsigned) evnt()->GetEventLen(), (unsigned) fEvPtr.fullsize());
      fEvPtr.reset();
      return false;
   }

   fSubPtr.reset();
   fRawPtr.reset();

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
      if (!fFirstEvent) fEvPtr.reset();
      if (fEvPtr.null()) return false;
      fFirstEvent = false;
      fSubPtr.reset();
      fRawPtr.reset();
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
   if (fEvPtr.null()) {
      ptr.reset();
      return false;
   }
   ptr.reset(fEvPtr, 0, evnt()->GetEventLen());
   return true;
}


bool dogma::ReadIterator::NextSubEvent()
{
   if (fSubPtr.null()) {
      if (fEvPtr.null()) return false;
      // this function is used both in hadtu and in event mode. Check out mode:
      dabc::BufferSize_t headsize = 0, containersize = 0;
      if (fBufType == mbt_DogmaEvents) {
         headsize = sizeof(dogma::DogmaEvent);
         containersize = evnt()->GetEventLen();
      } else if (fBufType == mbt_DogmaTransportUnit) {
         headsize = sizeof(dogma::DogmaTu);
         containersize = tu()->GetTuLen();
      } else if (fBufType == mbt_DogmaSubevents) {
         headsize = 0;
         containersize = fEvPtr.fullsize();
      } else {
         EOUT("NextSubEvent not allowed for buffer type %u. Check your code!", (unsigned) fBufType);
         return false;
      }

      if (containersize == 0) return false; // typical problem of artifical generated events

      if (containersize < headsize) {
         EOUT("DOGMA format error - tu container fullsize %u too small", (unsigned) containersize);
         return false;
      }
      fSubPtr.reset(fEvPtr, 0, containersize);
      fSubPtr.shift(headsize);
   } else {
      fSubPtr.shift(subevnt()->GetTuLen());
   }

   if (fSubPtr.fullsize() < sizeof(dogma::DogmaTu)) {
      fSubPtr.reset();
      return false;
   }

   if (subevnt()->GetTuLen() < sizeof(dogma::DogmaTu)) {
      EOUT("Hadaq format error - subevent fullsize %u too small", subevnt()->GetTuLen());
      //char* ptr = (char*) subevnt();
      //for(int i=0; i<20; ++i)
      //   printf("sub(%d)=0x%02x\n", i, (unsigned) *ptr++);

      fSubPtr.reset();
      return false;
   }

   fRawPtr.reset(fSubPtr, 0, subevnt()->GetTuLen());
   fRawPtr.shift(sizeof(dogma::DogmaTu));

   return true;
}

unsigned dogma::ReadIterator::rawdata_maxsize() const
{
   unsigned sz0 = fEvPtr.rawsize(), sz1 = 0;
   if (!fSubPtr.null()) sz1 = fEvPtr.distance_to(fSubPtr);
   return sz0 > sz1 ? sz0 - sz1 : 0;
}

unsigned dogma::ReadIterator::NumEvents(const dabc::Buffer& buf)
{
   ReadIterator iter(buf);
   unsigned cnt = 0;
   while (iter.NextEvent()) cnt++;
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

   subevnt()->SetPayloadLen(rawdatasz);

   fSubPtr.shift(subevnt()->GetTuLen());

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
   if (fEvPtr.null()) return false;

   dabc::BufferSize_t dist = sizeof(dogma::DogmaEvent);
   if (!fSubPtr.null())
      dist = fEvPtr.distance_to(fSubPtr);
   else if (fHasSubevents)
      dist = fEvPtr.fullsize(); // special case when exactly buffer was matched
   evnt()->SetPayloadLen(dist - sizeof(dogma::DogmaEvent));

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
