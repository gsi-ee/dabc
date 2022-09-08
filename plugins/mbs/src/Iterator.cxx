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
      EOUT("Only buffer format mbt_MbsEvents is supported");
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

bool mbs::ReadIterator::IsLastEvent() const
{
   if (fEvPtr.null()) return true;

   return fEvPtr.fullsize() <= evnt()->FullSize();
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
      EOUT("Raw size less than event header - not supported !!!!");

      fEvPtr.reset();
      return false;
   }

   if (fEvPtr.fullsize() < evnt()->FullSize()) {
      EOUT("Error in MBS format - declared event size %u bigger than actual portion in the buffer %u",
            (unsigned) evnt()->FullSize(), (unsigned) fEvPtr.fullsize());
      fEvPtr.reset();
      return false;
   }

   fSubPtr.reset();
   fRawPtr.reset();

   return true;
}

unsigned mbs::ReadIterator::GetEventSize() const
{
   if (!evnt()) return 0;
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
         EOUT("Mbs format error - event fullsize %u too small", evnt()->FullSize());
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
      EOUT("Mbs format error - subevent fullsize too small");
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
   unsigned cnt = 0;
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

mbs::WriteIterator::WriteIterator(dabc::Buffer& buf) :
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

bool mbs::WriteIterator::Reset(dabc::Buffer& buf)
{
   fBuffer.Release();
   fEvPtr.reset();
   fSubPtr.reset();
   fFullSize = 0;

   if (buf.GetTotalSize() < sizeof(EventHeader) + sizeof(SubeventHeader)) {
      EOUT("Buffer too small for just empty MBS event");
      return false;
   }

   fBuffer << buf;
   fBuffer.SetTypeId(mbt_MbsEvents);
   fEvPtr = fBuffer;
   return true;
}

dabc::Buffer mbs::WriteIterator::Close()
{
   if (!fSubData.null()) FinishSubEvent();
   if (!fSubPtr.null()) FinishEvent();
   unsigned full_size = fEvPtr.distance_to_ownbuf();
   if (full_size != fFullSize)
      EOUT("Sizes mismatch");

   fEvPtr.reset();

   if ((fFullSize>0) && (fBuffer.GetTotalSize() >= fFullSize))
      fBuffer.SetTotalSize(fFullSize);
   fFullSize = 0;

   dabc::Buffer res;
   res << fBuffer;
   return res;
}

dabc::Buffer mbs::WriteIterator::CloseAndTransfer(dabc::Buffer& newbuf)
{
   if (newbuf.null()) return Close();

   if (!IsAnyIncompleteData()) {
      dabc::Buffer oldbuf = Close();
      Reset(newbuf);
      return oldbuf;
   }


   if (!fEvPtr.is_same_buf(fSubPtr)) EOUT("Place 111");

   unsigned subptr_shift = fEvPtr.distance_to(fSubPtr);
   unsigned copy_size = subptr_shift;
   unsigned subdata_shift = 0;

   if (!fSubData.null()) {
      if (!fSubData.is_same_buf(fSubPtr)) EOUT("Place 222");
      subdata_shift = fSubPtr.distance_to(fSubData);
      copy_size += subdata_shift;
   }

   // this is copy of uncompleted data
   dabc::Pointer(newbuf).copyfrom(fEvPtr, copy_size);

   // forgot about event as it was
   DiscardEvent();

   // close iterator and fill ready data into the buffer
   dabc::Buffer oldbuf = Close();

   Reset(newbuf);
   fSubPtr.reset(fEvPtr, subptr_shift);
   if (subdata_shift>0) fSubData.reset(fSubPtr, subdata_shift);

   dabc::Buffer res;
   res << fBuffer;
   return res;
}


bool mbs::WriteIterator::IsPlaceForEvent(uint32_t subeventssize, bool add_subev_header)
{
   return fEvPtr.fullsize() >= (sizeof(EventHeader) + subeventssize + (add_subev_header ? sizeof(SubeventHeader) : 0));
}


bool mbs::WriteIterator::NewEvent(EventNumType event_number, uint32_t minsubeventssize)
{

   if (!fSubPtr.null()) {
      EOUT("Previous event not closed");
      return false;
   }

   if (fEvPtr.null()) {
      EOUT("No any place for new event");
      return false;
   }

   if (fEvPtr.fullsize() < (sizeof(EventHeader) + minsubeventssize)) {
      EOUT("Too few place for the new event");
      return false;
   }

   fSubPtr.reset(fEvPtr, sizeof(EventHeader));

   evnt()->Init(event_number);

   return true;
}

bool mbs::WriteIterator::NewSubevent(uint32_t minrawsize, uint8_t crate, uint16_t procid, uint8_t control)
{
   if (fSubPtr.null()) return false;

   if (!fSubData.null()) {
      EOUT("Previous subevent not yet finished!!!");
      return false;
   }

   if (fSubPtr.fullsize() < (sizeof(SubeventHeader) + minrawsize)) return false;

   subevnt()->Init(crate, procid, control);

   fSubData.reset(fSubPtr, sizeof(SubeventHeader));

   return true;
}

bool mbs::WriteIterator::NewSubevent2(uint32_t fullid)
{
   if (fSubPtr.null()) return false;

   if (!fSubData.null()) {
      EOUT("Previous subevent not yet finished!!!");
      return false;
   }

   subevnt()->InitFull(fullid);

   fSubData.reset(fSubPtr, sizeof(SubeventHeader));

   return true;
}



bool mbs::WriteIterator::IsPlaceForRawData(unsigned len) const
{
   return len <= fSubData.fullsize();
}


bool mbs::WriteIterator::AddRawData(const void* src, unsigned len)
{
   if (fSubData.null() || (len > fSubPtr.fullsize())) return false;
   fSubData.copyfrom_shift(src, len);
   return true;
}

bool mbs::WriteIterator::FinishSubEvent(uint32_t rawdatasz)
{
//   DOUT0("Subptr %s subdata %s", DBOOL(fSubPtr.null()), DBOOL(fSubData.null()));

   if (fSubPtr.null()) return false;

   unsigned filled_rawsize = 0;
   if (fSubData.null()) {
      // maximum size was filled in such case
      filled_rawsize = maxrawdatasize();
   } else {
      if (!fSubData.is_same_buf(fSubPtr)) EOUT("Place 333");
      filled_rawsize = fSubPtr.distance_to(fSubData) - sizeof(SubeventHeader);
   }

   if (rawdatasz==0) rawdatasz = filled_rawsize;

   if (rawdatasz > maxrawdatasize()) {
      EOUT("So big raw data size %u not possible, maximum is %u", rawdatasz, maxrawdatasize());
      rawdatasz  = maxrawdatasize();
   }

   subevnt()->SetRawDataSize(rawdatasz);

   fSubPtr.shift(subevnt()->FullSize());

   fSubData.reset();

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
   return AddSubevent( dabc::Pointer(sub, sub->FullSize()) );
}

bool mbs::WriteIterator::FinishEvent()
{
   if (fEvPtr.null()) return false;

   dabc::BufferSize_t fullsize = fEvPtr.fullsize();

   if (!fSubPtr.null()) {
      if (!fEvPtr.is_same_buf(fSubPtr)) EOUT("Place 444");
      fullsize = fEvPtr.distance_to(fSubPtr);
   }

   evnt()->SetFullSize(fullsize);
   fEvPtr.shift(fullsize);

   fSubPtr.reset();
   fFullSize += fullsize;

   return true;
}

void mbs::WriteIterator::DiscardEvent()
{
   fSubData.reset();
   fSubPtr.reset();
}


bool mbs::WriteIterator::CopyEventFrom(const dabc::Pointer& ptr, bool finish)
{
   if (!fSubPtr.null()) {
      EOUT("Previous event not closed");
      return false;
   }

   if (fEvPtr.null()) {
      EOUT("No any place for new event");
      return false;
   }

   if (fEvPtr.fullsize() < ptr.fullsize()) {
      EOUT("Too few place for the new event");
      return false;
   }

   fEvPtr.copyfrom(ptr);

   fSubPtr.reset(fEvPtr, ptr.fullsize());

   if (finish) return FinishEvent();

   return true;
}

