#include "mbs/MbsTypeDefs.h"

#include "dabc/logging.h"

#include <string.h>

const char* mbs::xmlFileName          = "FileName";
const char* mbs::xmlSizeLimit         = "SizeLimit";
const char* mbs::xmlFirstMultiple     = "FirstMultiple";
const char* mbs::xmlNumMultiple       = "NumMultiple";



mbs::ReadIterator::ReadIterator(dabc::Buffer* buf) :
   fBuffer(buf),
   fEvPtr(),
   fSubPtr(),
   fRawPtr()
{
   if (fBuffer==0) return;

   if (fBuffer->GetTypeId() != mbt_MbsEvs10_1) {
      EOUT(("Only buffer format mbt_MbsEvs10_1 are supported"));
      fBuffer = 0;
   }
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

void mbs::WriteIterator::Close()
{
   fEvPtr.reset();
   fSubPtr.reset();
   if (fBuffer && (fFullSize>0))
      fBuffer->SetDataSize(fFullSize);
   fBuffer = 0;
   fFullSize = 0;
}

bool mbs::WriteIterator::NewEvent(uint32_t event_number, uint32_t mineventsize)
{
   if (fBuffer == 0) return false;

   if (fEvPtr.null()) {
      fEvPtr.reset(fBuffer);
   } else {
      dabc::BufferSize_t dist = sizeof(EventHeader);
      if (!fSubPtr.null()) dist = fEvPtr.distance_to(fSubPtr);
      evnt()->SetFullSize(dist);
      fFullSize+=dist;
      fEvPtr.shift(dist);
   }

   fSubPtr.reset();

   if (fEvPtr.fullsize() < (sizeof(EventHeader) + mineventsize)) {
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

// __________________________________________________________________________________________________




#define MAX_BUF_LGTH 32768
#define MIN_BUF_LGTH 512

bool mbs::sMbsBufferHeader::Check()
{
   if (iType == MBS_TYPE(10, 1))
      return (((BufferLength()>>16)== 0) && (BufferLength() > 0)) &&
             (iMaxWords>0)&&
             (h_begin<2)&&
             (h_begin>=0)&&
             (h_end<2)&&
             (h_end>=0)&&
             (i_used <= (int16_t) (MAX_BUF_LGTH - sizeof(sMbsBufferHeader))/2) &&
             (i_used>0)&&
             ((iEndian==1)||
              (iEndian==0)||
          /* above, because some old lsm file forgot to set this bit, so it's zero */
              (iEndian==0x1000000L));

   if (iType == MBS_TYPE(100, 1))
      return (BufferLength() > 0) && IsCorrectEndian() && (iUsed==0);

   return false;
}

void mbs::sMbsBufferHeader::Swap()
{
   SwapMbsData(this, sizeof(sMbsBufferHeader));
}

int mbs::sMbsBufferHeader::BufferLength()
{
   // returns size of buffer (including header size) how it should be read from the file
   // 0 idicates error

   switch (iType) {
      // new buffer type
      case MBS_TYPE(100, 1):
         return sizeof(mbs::sMbsBufferHeader) + iUsedWords * 2;

      // old buffer type
      case MBS_TYPE(10, 1): {
         int res = iMaxWords * 2;
         if (res % 512 > 0) res += sizeof(mbs::sMbsBufferHeader);
         return res;
      }
      // new file header
      case MBS_TYPE(3000, 1):
         return sizeof(mbs::sMbsBufferHeader) + iUsedWords * 2;

      // old file header
      case MBS_TYPE(2000, 1): {
         int res = iMaxWords * 2;
         if (res % 512 > 0) res += sizeof(mbs::sMbsBufferHeader);
         return res;
      }

      default:
         EOUT(("Uncknown buffer type %d-%d", iType % 0x10000, iType / 0x10000));
   }

   return 0;
}

unsigned mbs::sMbsBufferHeader::UsedBufferLength()
{
   switch (iType) {
      // new buffer type
      case MBS_TYPE(100,1):
         return sizeof(mbs::sMbsBufferHeader) + iUsedWords * 2;

      // old buffer type
      case MBS_TYPE(10,1):
         return sizeof(mbs::sMbsBufferHeader) + i_used * 2;

      default:
         EOUT(("Uncknown buffer type %d-%d", i_type, i_subtype));
   }

   return 0;
}

void mbs::sMbsBufferHeader::SetUsedLength(int len)
{
   switch (iType) {
      // new buffer type
      case MBS_TYPE(100,1):
         iUsedWords = (len - sizeof(mbs::sMbsBufferHeader)) / 2;
         break;

      // old buffer type
      case MBS_TYPE(10,1):
         i_used = (len - sizeof(mbs::sMbsBufferHeader)) / 2;
         break;

      default:
         EOUT(("Uncknown buffer type %d-%d", i_type, i_subtype));
   }
}

// _______________________________________________________________

void mbs::eMbs101EventHeader::Dump101Event()
{
   if ((iType != MBS_TYPE(10,1)) && (iType != MBS_TYPE(100,1)) ) {
      EOUT(("Wrong type event type %d-%d", i_type, i_subtype));
      return;
   }

   DOUT1(("  Event %d (%d-%d)  size %d", iCount, i_type, i_subtype, DataSize()));

   char* buf = (char*) this;

   char* end_buf = buf + DataSize();

   buf += sizeof(mbs::eMbs101EventHeader);

   int cnt = 0;

   while ((buf + sizeof(mbs::eMbs101SubeventHeader)) <= end_buf) {
      if (cnt++>10) {
          DOUT1(("    ..."));
          break;
      }
      mbs::eMbs101SubeventHeader* subev = (mbs::eMbs101SubeventHeader*) buf;

      DOUT1(("    Subevent %d size %d", subev->iProcId, subev->DataSize() - sizeof(eMbs101SubeventHeader)));

      buf += subev->DataSize();
   }
}

void mbs::eMbs101EventHeader::CopyFrom(const eMbs101EventHeader& src)
{
   iDummy = src.iDummy;
   iTrigger = src.iTrigger;
   iCount = src.iCount;

   iType = src.iType;
   iMaxWords = src.iMaxWords;
}

// _______________________________________________________________


void mbs::IterateEvent(void* evhdr)
{
   if (evhdr==0) return;

   ((mbs::eMbs101EventHeader*) evhdr)->Dump101Event();
}

void mbs::SwapMbsData(void* data, int bytessize)
{
   if (data==0) return;
   int sz = bytessize / 4;

   uint32_t* d = (uint32_t*) data;

   for (int i=0; i<sz; i++) {
      *d = bswap_32(*d);
      d++;
   }
}

void mbs::IterateBuffer(void* buf, bool reset)
{
   sMbsBufferHeader* bufhdr  = (sMbsBufferHeader*) buf;

   if (bufhdr->UsedBufferLength() < sizeof(sMbsBufferHeader)) {
      EOUT(("This is not MBS buffer. Halt"));
      return;
   }

   unsigned totalsize = bufhdr->UsedBufferLength() - sizeof(sMbsBufferHeader);

   char* tmp = (char*) buf + sizeof(sMbsBufferHeader);

   DOUT1(("Iterate in buffer size:%d typ:%d-%d h_begin:%d h_end:%d",
          bufhdr->UsedBufferLength(), bufhdr->i_type, bufhdr->i_subtype,
          bufhdr->h_begin, bufhdr->h_end));

   static char rest_buffer[10000];
   static int rest_buffer_size = 0;

   if (reset) rest_buffer_size = 0;

   if (bufhdr->h_end>0) {
      if (rest_buffer_size==0) {
         EOUT(("No rest of buffer fragment"));
         DOUT1(( "No rest of buffer fragment"));
         return;
      }
      eMbsEventHeader* evnt = (eMbsEventHeader*) tmp;
      int add_size = evnt->DataSize() - sizeof(eMbsEventHeader);

      DOUT1(("Add fragment %d", add_size));

      memcpy(&(rest_buffer[rest_buffer_size]),
               tmp + sizeof(eMbsEventHeader), add_size);

      eMbsEventHeader* evnt0 = (eMbsEventHeader*) rest_buffer;
      evnt0->iMaxWords += add_size/2;

      ((eMbs101EventHeader*) evnt0)->Dump101Event();

      rest_buffer_size = 0;

      tmp += evnt->DataSize();
      totalsize -= evnt->DataSize();
   }

   int cnt = 0;

   while (totalsize>0) {
      if (totalsize<sizeof(eMbsEventHeader)) {
         EOUT(("Too small peace of buffer %d is remained empty", totalsize));
         break;
      }

      if (cnt > 10000) {
         DOUT1(("  ..."));
         break;
      }

      eMbsEventHeader* evnt = (eMbsEventHeader*) tmp;

      unsigned eventsize = evnt->DataSize();

      if (eventsize > totalsize) {
         EOUT(("Missmatch between event size %d and rest size in buffer %d", eventsize, totalsize));
         break;
      }

      if ((eventsize==totalsize) && (bufhdr->h_begin>0)) {
         DOUT1(("Keep fragment %d", eventsize));
         rest_buffer_size = eventsize;
         memcpy(rest_buffer, tmp, eventsize);
         break;
      }

      if (evnt->iType == MBS_TYPE(10, 1)) {
         eMbs101EventHeader* evnt101 = (eMbs101EventHeader*) evnt;
         if ((cnt<3) || (eventsize==totalsize))
            evnt101->Dump101Event();
      }

      cnt++;

      tmp += eventsize;
      totalsize -= eventsize;
   }
}


bool mbs::FirstEvent(const dabc::Pointer& bufptr, dabc::Pointer& evptr, eMbs101EventHeader* &evhdr, void* tmp)
{
   // locate at the first event of buffer, after buffer header
   // buffer header should be not segment

   if (bufptr.rawsize() < sizeof(sMbsBufferHeader)) {
      EOUT(("Cannot map buffer header on segemented area"));
      return false;
   }

   sMbsBufferHeader* bufhdr = (sMbsBufferHeader*) bufptr();

   if (bufhdr->iType != MBS_TYPE(100, 1)) {
      EOUT(("Only 100-1 buffer type is supported"));
      return false;
   }

   evptr.reset(bufptr, bufhdr->UsedBufferLength());

   evptr.shift(sizeof(sMbsBufferHeader));

   evhdr = 0;

   return mbs::NextEvent(evptr, evhdr, tmp);
}

bool mbs::GetEventHeader(dabc::Pointer& evptr, eMbs101EventHeader* &evhdr, void* tmp)
{
   // extracts pointer on the event header
   // if data is segmented, header first copyed to the temporary area

   evhdr = 0;

   return mbs::NextEvent(evptr, evhdr, tmp);
}

bool mbs::NextEvent(dabc::Pointer& evptr, eMbs101EventHeader* &evhdr, void* tmp)
{
   if (evptr.fullsize()==0) {
      EOUT(("No more events in the buffer"));
      return false;
   }

   if (evhdr) {
      dabc::BufferSize_t evlen = (dabc::BufferSize_t) evhdr->DataSize();
      evhdr = 0;
      if (evlen >= evptr.fullsize()) return false;
      evptr.shift(evlen);
   }

   if (evptr.rawsize() >= sizeof(eMbs101EventHeader))
      evhdr = (eMbs101EventHeader*) evptr();
   else {
      if (tmp==0) {
         EOUT(("Cannot map event header on segmented area"));
         return false;
      }
      evptr.copyto(tmp, sizeof(eMbs101EventHeader));
      evhdr = (eMbs101EventHeader*) tmp;
   }

   if (evhdr->iType != MBS_TYPE(10, 1)) {
      EOUT(("Only 10-1 event type is supported"));
      return false;
   }

   return true;
}

bool mbs::FirstSubEvent(const dabc::Pointer& evptr, eMbs101EventHeader* evhdr, dabc::Pointer& subevptr, mbs::eMbs101SubeventHeader* &subevhdr, void* tmp)
{
   if (evhdr==0) return false;

   subevptr.reset(evptr, (dabc::BufferSize_t) evhdr->DataSize());

   subevptr.shift(sizeof(eMbs101EventHeader));

   subevhdr = 0;

   return mbs::NextSubEvent(subevptr, subevhdr, tmp);
}

bool mbs::NextSubEvent(dabc::Pointer& subevptr, eMbs101SubeventHeader* &subevhdr, void* tmp)
{
   // subevlen must contain correct length of previous subevent
   if (subevhdr) {
      dabc::BufferSize_t subevlen = (dabc::BufferSize_t) subevhdr->DataSize();
      subevhdr = 0;
      if (subevlen >= subevptr.fullsize()) return false;
      subevptr.shift(subevlen);
   }

   if (subevptr.rawsize() >= sizeof(eMbs101SubeventHeader))
      subevhdr = (eMbs101SubeventHeader*) subevptr.ptr();
   else {
      if (tmp==0) {
         EOUT(("Cannot map subevent header on segmented memory"));
         return false;
      }
      subevptr.copyto(tmp, sizeof(eMbs101SubeventHeader));
      subevhdr = (eMbs101SubeventHeader*) tmp;
   }

   if (subevhdr->iType != MBS_TYPE(10, 1)) {
      EOUT(("Only 10-1 subevent type is supported"));
      return false;
   }

   return true;
}

bool mbs::StartBuffer(dabc::Buffer* buf, dabc::Pointer& evptr, sMbsBufferHeader* &bufhdr, void* tmp, bool newformat)
{
   dabc::Pointer bufptr(buf);

   bool res = mbs::StartBuffer(bufptr, evptr, bufhdr, tmp, newformat);
   if (res)
      buf->SetTypeId(newformat ? mbs::mbt_Mbs100_1 : mbs::mbt_Mbs10_1);

   return res;
}

bool mbs::StartBuffer(dabc::Pointer& bufptr, dabc::Pointer& evptr, sMbsBufferHeader* &bufhdr, void* tmp, bool newformat)
{
   if (bufptr.fullsize() < sizeof(sMbsBufferHeader)) {
//      EOUT(("Buffer too small"));
      return false;
   }

   if (bufptr.rawsize() < sizeof(sMbsBufferHeader))
      bufhdr = (sMbsBufferHeader*) tmp;
   else
      bufhdr = (sMbsBufferHeader*) bufptr();

   if (bufhdr==0) return false;

   // set here some buffer default values

   bufhdr->iType = newformat ? MBS_TYPE(100, 1) : MBS_TYPE(10, 1);
   bufhdr->SetEndian();
   bufhdr->iNumEvents = 0;
   bufhdr->iBufferId = 0;
   // this is required for old 10-1 format
   bufhdr->iMaxWords = (bufptr.fullsize() - sizeof(sMbsBufferHeader)) / 2;

   evptr.reset(bufptr);

   evptr.shift(sizeof(sMbsBufferHeader));

   return true;
}

bool mbs::StartEvent(dabc::Pointer& evptr, dabc::Pointer& subevptr, eMbs101EventHeader* &evhdr, void* tmp)
{
   if (evptr.fullsize() < sizeof(eMbs101EventHeader)) {
//      EOUT(("Buffer too small"));
      return false;
   }

   if (evptr.rawsize() < sizeof(eMbs101EventHeader))
      evhdr = (eMbs101EventHeader*) tmp;
   else
      evhdr = (eMbs101EventHeader*) evptr();

   if (evhdr==0) return false;

   evhdr->iType = MBS_TYPE(10, 1);

   // set here some event default values

   subevptr.reset(evptr);

   subevptr.shift(sizeof(eMbs101EventHeader));

   return true;
}

bool mbs::StartSubEvent(dabc::Pointer& subevptr, dabc::Pointer& subevdata, eMbs101SubeventHeader* &subevhdr, void* tmp)
{
   if (subevptr.fullsize() < sizeof(eMbs101SubeventHeader)) {
//      EOUT(("Buffer too small"));
      return false;
   }

   if (subevptr.rawsize() < sizeof(eMbs101SubeventHeader))
      subevhdr = (eMbs101SubeventHeader*) tmp;
   else
      subevhdr = (eMbs101SubeventHeader*) subevptr();

   if (subevhdr==0) return false;

   subevhdr->iType = MBS_TYPE(10, 1);

   // set here some subevent default values
   subevdata.reset(subevptr);
   subevdata.shift(sizeof(eMbs101SubeventHeader));

   return true;
}

bool mbs::FinishSubEvent(dabc::Pointer& subevptr, dabc::Pointer& subevdata, eMbs101SubeventHeader* subevhdr)
{
   dabc::BufferSize_t len = subevptr.distance_to(subevdata);

   if (len == dabc::BufferSizeError) return false;

   subevhdr->SetDataSize(len);

   // copy header information from the external header buffer
   if (subevhdr != subevptr())
      subevptr.copyfrom(subevhdr, sizeof(eMbs101SubeventHeader));

   subevptr.shift(len);

   return true;
}

dabc::BufferSize_t mbs::FinishEvent(dabc::Pointer& evptr, dabc::Pointer& subevptr, eMbs101EventHeader* evhdr, sMbsBufferHeader* bufhdr)
{
   dabc::BufferSize_t len = evptr.distance_to(subevptr);

   if (len == dabc::BufferSizeError) return dabc::BufferSizeError;

   evhdr->SetDataSize(len);

   // copy header information from the external header buffer
   if (evhdr != evptr())
      evptr.copyfrom(evhdr, sizeof(eMbs101EventHeader));

   evptr.shift(len);

   if (bufhdr!=0) bufhdr->iNumEvents++;

   return len;
}

dabc::BufferSize_t mbs::FinishBuffer(dabc::Pointer& bufptr, dabc::Pointer& evptr, sMbsBufferHeader* bufhdr)
{
   dabc::BufferSize_t len = bufptr.distance_to(evptr);

   if (len == dabc::BufferSizeError) return len;

   bufhdr->SetUsedLength(len);

   // copy header information from the external header buffer
   if (bufhdr != bufptr())
      bufptr.copyfrom(bufhdr, sizeof(sMbsBufferHeader));

   bufptr.shift(len);

   return len;
}

bool mbs::FinishBuffer(dabc::Buffer* buf, dabc::Pointer& evptr, sMbsBufferHeader* bufhdr)
{
   dabc::Pointer bufptr(buf);

   dabc::BufferSize_t fulllen = FinishBuffer(bufptr, evptr, bufhdr);

   if (fulllen == dabc::BufferSizeError) return false;

   if (buf->GetTypeId() == mbs::mbt_Mbs100_1)
      buf->SetDataSize(fulllen);

   return true;
}
