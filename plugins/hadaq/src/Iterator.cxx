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

#include "iostream"

hadaq::ReadIterator::ReadIterator() :
   fFirstEvent(false),
   fEvPtr(),
   fSubPtr(),
   fRawPtr(),
   fBufType(dabc::mbt_Null)
{
}

hadaq::ReadIterator::ReadIterator(const dabc::Buffer& buf) :
   fFirstEvent(false),
   fEvPtr(),
   fSubPtr(),
   fRawPtr(),
   fBufType(dabc::mbt_Null)
{
   Reset(buf);
}

hadaq::ReadIterator::ReadIterator(const ReadIterator& src) :
   fFirstEvent(src.fFirstEvent),
   fEvPtr(src.fEvPtr),
   fSubPtr(src.fSubPtr),
   fRawPtr(src.fRawPtr),
   fBufType(src.fBufType)
{
}

hadaq::ReadIterator& hadaq::ReadIterator::operator=(const ReadIterator& src)
{
   fFirstEvent = src.fFirstEvent;
   fEvPtr = src.fEvPtr;
   fSubPtr = src.fSubPtr;
   fRawPtr = src.fRawPtr;
   fBufType = src.fBufType;

   return *this;
}

bool hadaq::ReadIterator::Reset(const dabc::Buffer& buf)
{
   Close();

   if (buf.null()) return false;

//   DOUT0("Assign buffer size %u", (unsigned) buf.GetTotalSize());

   fBufType = buf.GetTypeId();

   if (!(fBufType == mbt_HadaqEvents || fBufType == mbt_HadaqTransportUnit) ) {
      EOUT("Only buffer format mbt_HadaqEvents or mbt_HadaqTransportUnit is supported");
      return false;
   }

   fEvPtr = buf;
   fFirstEvent = true;

   return true;
}

void hadaq::ReadIterator::Close()
{
   fEvPtr.reset();
   fSubPtr.reset();
   fRawPtr.reset();
   fFirstEvent = false;
   fBufType = dabc::mbt_Null;
}


bool hadaq::ReadIterator::NextHadTu()
{
   if (fBufType != mbt_HadaqTransportUnit ) {
      EOUT("NextHadTu only allowed for buffer type mbt_HadaqTransportUnit. Check your code!");
      return false;
   }

   if (fEvPtr.null()) return false;

   if (fFirstEvent)
      fFirstEvent = false;
   else
      fEvPtr.shift(hadtu()->GetPaddedSize());

   if (fEvPtr.fullsize() < sizeof(hadaq::HadTu)) {
      fEvPtr.reset();
      return false;
   }

   if (fEvPtr.rawsize() < sizeof(hadaq::HadTu)) {
      EOUT("Raw size less than transport unit header - not supported !!!!");
      fEvPtr.reset();
      return false;
   }

   if (fEvPtr.fullsize() < hadtu()->GetSize()) {
      EOUT("Error in HADAQ format - declared event size %u smaller than actual portion in buffer %u",
            (unsigned) hadtu()->GetSize(), (unsigned) fEvPtr.fullsize());
      fEvPtr.reset();
      return false;
   }

   fSubPtr.reset();
   fRawPtr.reset();

   return true;
}


bool hadaq::ReadIterator::NextEvent()
{
   if (fBufType != mbt_HadaqEvents ) {
      EOUT("NextEvent only allowed for buffer type mbt_HadaqEvents. Check your code!");
      return false;
   }

   if (fEvPtr.null()) return false;

   if (fFirstEvent)
      fFirstEvent = false;
   else
      fEvPtr.shift(evnt()->GetPaddedSize());

   if (fEvPtr.fullsize() < sizeof(hadaq::Event)) {
      fEvPtr.reset();
      return false;
   }

   if (fEvPtr.rawsize() < sizeof(hadaq::Event)) {
      EOUT("Raw size less than event header - not supported !!!!");

      fEvPtr.reset();
      return false;
   }

   if (fEvPtr.fullsize() < evnt()->GetSize()) {
      EOUT("Error in HADAQ format - declared event size %u bigger than actual portion in buffer %u",
            (unsigned) evnt()->GetSize(), (unsigned) fEvPtr.fullsize());
      fEvPtr.reset();
      return false;
   }

   fSubPtr.reset();
   fRawPtr.reset();

   return true;
}

bool hadaq::ReadIterator::AssignEventPointer(dabc::Pointer& ptr)
{
   if (fEvPtr.null()) {
      ptr.reset();
      return false;
   }
   ptr.reset(fEvPtr, 0, evnt()->GetPaddedSize());
   return true;
}


bool hadaq::ReadIterator::NextSubEvent()
{
   if (fSubPtr.null()) {
      if (fEvPtr.null()) return false;
      // this function is used both in hadtu and in event mode. Check out mode:
      dabc::BufferSize_t headsize=0;
      size_t containersize = 0;
      if (fBufType == mbt_HadaqEvents) {
         headsize = sizeof(hadaq::Event);
         containersize = evnt()->GetPaddedSize();
      } else if (fBufType == mbt_HadaqTransportUnit) {
         headsize = sizeof(hadaq::HadTu);
         containersize = hadtu()->GetPaddedSize();
      } else {
         EOUT("NextSubEvent only allowed for buffer type mbt_HadaqEvents or mbt_HadaqTransportUnit. Check your code!");
         return false;
      }

      if (containersize < headsize) {
         EOUT("Hadaq format error - tu container fullsize %u too small", containersize);
         return false;
      }
      fSubPtr.reset(fEvPtr, 0, containersize);

      fSubPtr.shift(headsize);
   } else
      fSubPtr.shift(subevnt()->GetPaddedSize());

   if (fSubPtr.fullsize() < sizeof(hadaq::Subevent)) {
      fSubPtr.reset();
      return false;
   }

   if (subevnt()->GetSize() < sizeof(hadaq::Subevent)) {
      EOUT("Hadaq format error - subevent fullsize %u too small", subevnt()->GetSize());
      char* ptr=(char*) subevnt();
      for(int i=0; i<20; ++i)
      {
         std::cout <<"sub("<<i<<")=0x"<< (std::hex)<< *ptr ;
         std::cout <<(std::dec)<< std::endl;
      }


      fSubPtr.reset();
      return false;
   }

   fRawPtr.reset(fSubPtr, 0, subevnt()->GetPaddedSize());
   fRawPtr.shift(sizeof(hadaq::Subevent));

   return true;
}

unsigned hadaq::ReadIterator::NumEvents(const dabc::Buffer& buf)
{
   ReadIterator iter(buf);
   unsigned cnt(0);
   while (iter.NextEvent()) cnt++;
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

   if (buf.GetTotalSize() < sizeof(hadaq::Event) + sizeof(hadaq::Subevent)) {
      EOUT("Buffer too small for just empty HADAQ event");
      return false;
   }

   fBuffer = buf;
   fBuffer.SetTypeId(mbt_HadaqEvents);
   return true;
}

dabc::Buffer hadaq::WriteIterator::Close()
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

bool hadaq::WriteIterator::IsPlaceForEvent(uint32_t subeventssize)
{
   dabc::BufferSize_t availible = 0;

   if (!fEvPtr.null()) availible = fEvPtr.fullsize();
   else  availible = fBuffer.GetTotalSize();

   return availible >= (sizeof(hadaq::Event) + subeventssize);
}


bool  hadaq::WriteIterator::NewEvent(EventNumType evtSeqNr, RunId runNr, uint32_t minsubeventssize)

{
   // TODO: add arguments to set other event header fields
   if (fBuffer.null()) return false;

   if (fEvPtr.null())  fEvPtr = fBuffer;

   fSubPtr.reset();

   if (fEvPtr.fullsize() < (sizeof(hadaq::Event) + minsubeventssize)) {
      fEvPtr.reset();
      return false;
   }

   evnt()->Init(evtSeqNr, runNr);

   return true;
}

bool hadaq::WriteIterator::NewSubevent(uint32_t minrawsize, uint32_t trigger)
{
   if (fEvPtr.null()) return false;

   if (fSubPtr.null())
      fSubPtr.reset(fEvPtr, sizeof(hadaq::Event));

   if (fSubPtr.fullsize() < (sizeof(hadaq::Subevent) + minrawsize)) return false;

   subevnt()->Init(trigger);

   return true;
}

bool hadaq::WriteIterator::FinishSubEvent(uint32_t rawdatasz)
{
   if (fSubPtr.null()) return false;

   if (rawdatasz > maxrawdatasize()) return false;

   subevnt()->SetSize(rawdatasz);

   fSubPtr.shift(subevnt()->GetPaddedSize());

   return true;
}

bool hadaq::WriteIterator::AddSubevent(const dabc::Pointer& source)
{
   if (fEvPtr.null()) return false;

   if (fSubPtr.null())
      fSubPtr.reset(fEvPtr, sizeof(hadaq::Event));

   if (fSubPtr.fullsize() < source.fullsize()) return false;

   fSubPtr.copyfrom(source);

   fSubPtr.shift(source.fullsize());

   return true;
}

bool hadaq::WriteIterator::AddSubevent(hadaq::Subevent* sub)
{
   return AddSubevent(dabc::Pointer(sub, sub->GetPaddedSize()));
}

bool hadaq::WriteIterator::FinishEvent()
{
   if (fEvPtr.null()) return false;

   dabc::BufferSize_t dist = sizeof(hadaq::Event);
   if (!fSubPtr.null()) dist = fEvPtr.distance_to(fSubPtr);
   evnt()->SetSize(dist);
   dabc::BufferSize_t paddeddist=evnt()->GetPaddedSize();
   fFullSize += paddeddist;
   fEvPtr.shift(paddeddist);

   return true;
}

