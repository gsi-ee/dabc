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

#include "dabc/MemoryPool.h"

#include <stdlib.h>
#include <list>

#include "dabc/logging.h"
#include "dabc/threads.h"
#include "dabc/defines.h"
#include "dabc/Configuration.h"


namespace dabc {

   class MemoryBlock {
      public:

         struct Entry {
            void         *buf;     //!< pointer on raw memory
            BufferSize_t  size;    //!< size of the block
            bool          owner;   //!< is memory should be released
            int           refcnt;  //!< usage counter - number of references on the memory
         };

         typedef Queue<unsigned, false> FreeQueue;

         Entry*    fArr;       //!< array of buffers
         unsigned  fNumber;    //!< number of buffers
         FreeQueue fFree;      //!< list of free buffers

         MemoryBlock() :
            fArr(0),
            fNumber(0),
            fFree()
         {
         }

         virtual ~MemoryBlock()
         {
            Release();
         }

         inline bool IsAnyFree() const { return !fFree.Empty(); }

         void Release()
         {
            if (fArr==0) return;

            for (unsigned n=0;n<fNumber;n++) {
               if (fArr[n].buf && fArr[n].owner) free(fArr[n].buf);
               fArr[n].buf = 0;
            }

            delete[] fArr;
            fArr = 0;
            fNumber = 0;

            fFree.Reset();
         }

         bool Allocate(unsigned number, unsigned size, unsigned align) throw()
         {
            Release();

            fArr = new Entry[number];
            fNumber = number;

            fFree.Allocate(number);

            for (unsigned n=0;n<fNumber;n++) {

               void* buf(0);
               int res = posix_memalign(&buf, align, size);

               if ((res!=0) || (buf==0)) {
                  EOUT(("Cannot allocate data for new Memory Block"));
                  throw dabc::Exception("Cannot allocate buffer");
                  return false;
               }

               fArr[n].buf = buf;
               fArr[n].size = size;
               fArr[n].owner = true;
               fArr[n].refcnt = 0;

               fFree.Push(n);
            }

            return true;
         }

         bool Assign(bool isowner, const std::vector<void*>& bufs, const std::vector<unsigned>& sizes) throw()
         {
            Release();

            fArr = new Entry[bufs.size()];
            fNumber = bufs.size();

            fFree.Allocate(bufs.size());

            for (unsigned n=0;n<fNumber;n++) {
               fArr[n].buf = bufs[n];
               fArr[n].size = sizes[n];
               fArr[n].owner = isowner;
               fArr[n].refcnt = 0;

               fFree.Push(n);
            }
            return true;
         }


   };



}

// ---------------------------------------------------------------------------------

unsigned dabc::MemoryPool::fDfltAlignment = 16;
unsigned dabc::MemoryPool::fDfltNumSegments = 8;
unsigned dabc::MemoryPool::fDfltRefCoeff = 2;
unsigned dabc::MemoryPool::fDfltBufSize = 4096;


dabc::MemoryPool::MemoryPool(const char* name, bool withmanager) :
   dabc::Worker(MakePair(name, withmanager)),
   fMem(0),
   fSeg(0),
   fAlignment(fDfltAlignment),
   fMaxNumSegments(fDfltNumSegments),
   fReqQueue(16),  // FIXME: is size 16 too big/small or should be fixed ????
   fEvntFired(false),
   fChangeCounter(0),
   fUseThread(false)
{
   DOUT3(("MemoryPool %p name %s constructor", this, GetName()));

//   if (IsName("Pool")) SetLogging(true);
}

dabc::MemoryPool::~MemoryPool()
{
   Release();

   DOUT3(("MemoryPool %p name %s destructor", this, GetName()));
}

bool dabc::MemoryPool::SetAlignment(unsigned align)
{
   LockGuard lock(ObjectMutex());
   if (fMem!=0) return false;
   fAlignment = align;
   return true;
}


unsigned dabc::MemoryPool::GetAlignment() const
{
   LockGuard lock(ObjectMutex());
   return fAlignment;
}

bool dabc::MemoryPool::SetMaxNumSegments(unsigned num)
{
   LockGuard lock(ObjectMutex());
   if (fMem!=0) return false;
   fMaxNumSegments = num;
   return true;
}


unsigned dabc::MemoryPool::GetMaxNumSegments() const
{
   LockGuard lock(ObjectMutex());
   return fMaxNumSegments;
}

bool dabc::MemoryPool::_Allocate(BufferSize_t bufsize, unsigned number, unsigned refcoef) throw()
{
   if ((fMem!=0) && (fSeg!=0)) return false;

   if (bufsize*number==0) return false;

   if (refcoef==0) refcoef = 1;

   DOUT3(("POOL:%s Create num:%u X size:%u buffers align:%u", GetName(), number, bufsize, fAlignment));

   fMem = new MemoryBlock;
   fMem->Allocate(number, bufsize, fAlignment);

   DOUT3(("POOL:%s Create num:%u references with num:%u segments", GetName(), number*refcoef, fMaxNumSegments));

   fSeg = new MemoryBlock;
   fSeg->Allocate(number*refcoef, fMaxNumSegments*sizeof(MemSegment) + sizeof(dabc::Buffer::BufferRec), 16);

   fChangeCounter++;

   return true;
}

bool dabc::MemoryPool::Allocate(BufferSize_t bufsize, unsigned number, unsigned refcoef) throw()
{
   LockGuard lock(ObjectMutex());
   return _Allocate(bufsize, number, refcoef);
}

bool dabc::MemoryPool::Assign(bool isowner, const std::vector<void*>& bufs, const std::vector<unsigned>& sizes, unsigned refcoef) throw()
{
   LockGuard lock(ObjectMutex());

   if ((fMem!=0) && (fSeg!=0)) return false;

   if ((bufs.size() != sizes.size()) || (bufs.size()==0)) return false;

   if (refcoef==0) refcoef = GetDfltRefCoeff();

   fMem = new MemoryBlock;
   fMem->Assign(isowner, bufs, sizes);

   fSeg = new MemoryBlock;
   fSeg->Allocate(bufs.size()*refcoef, fMaxNumSegments*sizeof(MemSegment) + sizeof(dabc::Buffer::BufferRec), 16);

   return true;
}

bool dabc::MemoryPool::Release() throw()
{
   LockGuard lock(ObjectMutex());

   if (fMem) {
      delete fMem;
      fMem = 0;
      fChangeCounter++;
   }

   if (fSeg) {
      delete fSeg;
      fSeg = 0;
      fChangeCounter++;
   }

   return true;
}


bool dabc::MemoryPool::IsEmpty() const
{
   LockGuard lock(ObjectMutex());

   return (fMem==0) || (fSeg==0);
}

unsigned dabc::MemoryPool::GetNumBuffers() const
{
   LockGuard lock(ObjectMutex());

   return (fMem==0) ? 0 : fMem->fNumber;
}

unsigned dabc::MemoryPool::GetBufferSize(unsigned id) const
{
   LockGuard lock(ObjectMutex());

   return (fMem==0) || (id>=fMem->fNumber) ? 0 : fMem->fArr[id].size;
}

unsigned dabc::MemoryPool::GetMaxBufSize() const
{
   LockGuard lock(ObjectMutex());

   if (fMem==0) return 0;

   unsigned max(0);

   for (unsigned id=0;id<fMem->fNumber;id++)
      if (fMem->fArr[id].size > max) max = fMem->fArr[id].size;

   return max;
}

unsigned dabc::MemoryPool::GetMinBufSize() const
{
   LockGuard lock(ObjectMutex());

   if (fMem==0) return 0;

   unsigned min(0);

   for (unsigned id=0;id<fMem->fNumber;id++)
      if ((min==0) || (fMem->fArr[id].size < min)) min = fMem->fArr[id].size;

   return min;
}


void* dabc::MemoryPool::GetBufferLocation(unsigned id) const
{
   LockGuard lock(ObjectMutex());

   return (fMem==0) || (id>=fMem->fNumber) ? 0 : fMem->fArr[id].buf;
}

bool dabc::MemoryPool::_GetPoolInfo(std::vector<void*>& bufs, std::vector<unsigned>& sizes, unsigned* changecnt)
{
   if (changecnt!=0) {
      if (*changecnt == fChangeCounter) return false;
   }

   if (fMem!=0)
      for(unsigned n=0;n<fMem->fNumber;n++) {
         bufs.push_back(fMem->fArr[n].buf);
         sizes.push_back(fMem->fArr[n].size);
      }

   if (changecnt!=0) *changecnt = fChangeCounter;

   return true;
}

bool dabc::MemoryPool::GetPoolInfo(std::vector<void*>& bufs, std::vector<unsigned>& sizes)
{
   LockGuard lock(ObjectMutex());
   return _GetPoolInfo(bufs, sizes);
}


bool dabc::MemoryPool::TakeRawBuffer(unsigned& indx)
{
   LockGuard lock(ObjectMutex());
   if ((fMem==0) || !fMem->IsAnyFree()) return false;
   indx = fMem->fFree.Pop();
   return true;
}

void dabc::MemoryPool::ReleaseRawBuffer(unsigned indx)
{
   LockGuard lock(ObjectMutex());
   if (fMem) fMem->fFree.Push(indx);
}

void dabc::MemoryPool::_TakeSegmentsList(MemoryPool* pool, dabc::Buffer& buf, unsigned numsegm)
{
   // segments list too long, allocate directly by the buffer
   if ((pool==0) || (numsegm>pool->fMaxNumSegments) || !pool->fSeg->IsAnyFree()) {
      buf.AllocateRec(numsegm);
   } else {
      unsigned refid = pool->fSeg->fFree.Pop();

      if (pool->fSeg->fArr[refid].refcnt!=0)
         throw dabc::Exception("Segments are not free even is declared so");

      pool->fSeg->fArr[refid].refcnt++;

      buf.AssignRec(pool->fSeg->fArr[refid].buf, pool->fSeg->fArr[refid].size);
      buf.fRec->fPoolId = refid+1;
   }

   if (pool) {
      pool->_IncObjectRefCnt();
      buf.fRec->fPool = pool;
   }

}


dabc::Buffer dabc::MemoryPool::_TakeBuffer(BufferSize_t size, bool except, bool reserve_memory) throw()
{
   Buffer res;

//   DOUT0(("_TakeBuffer obj %p", &res));

   // if no memory is available, try to allocate it
   if ((fMem==0) || (fSeg==0)) _Allocate();

   if ((fMem==0) || (fSeg==0)) {
      if (except) throw dabc::Exception("No memory allocated in the pool");
      return res;
   }

   if (!fMem->IsAnyFree() && reserve_memory) {
      if (except) throw dabc::Exception("No any memory is available in the pool");
      return res;
   }

   if ((size==0) && reserve_memory) size = fMem->fArr[fMem->fFree.Front()].size;

   // first check if required size is available
   BufferSize_t sum(0);
   unsigned cnt(0);
   while (sum<size) {
      if (cnt>=fMem->fFree.Size()) {
         if (except) throw dabc::Exception("Cannot reserve buffer of requested size");
         return res;
      }

      unsigned id = fMem->fFree.Item(cnt);
      sum += fMem->fArr[id].size;
      cnt++;
   }

   _TakeSegmentsList(this, res, cnt);

   sum = 0;
   cnt = 0;
   MemSegment* segs = res.Segments();

   while (sum<size) {
      unsigned id = fMem->fFree.Pop();

      if (fMem->fArr[id].refcnt!=0)
         throw dabc::Exception("Buffer is not free even is declared so");

      if (cnt>=res.fRec->fCapacity)
         throw dabc::Exception("All mem segments does not fit into preallocated list");

      segs[cnt].buffer = fMem->fArr[id].buf;
      segs[cnt].datasize = fMem->fArr[id].size;
      segs[cnt].id = id;

      // Provide buffer of exactly requested size
      BufferSize_t restsize = size - sum;
      if (restsize < segs[cnt].datasize) segs[cnt].datasize = restsize;

      sum += fMem->fArr[id].size;

      // increment reference counter on the memory space
      fMem->fArr[id].refcnt++;

      cnt++;
   }

   res.fRec->fNumSegments = cnt;

   res.SetTypeId(mbt_Generic);

   return res;
}

dabc::Buffer dabc::MemoryPool::TakeBuffer(BufferSize_t size) throw()
{
   bool process_req(false);
   dabc::Buffer res;

   {
      LockGuard lock(ObjectMutex());

      if (fReqQueue.Size()>0)
         process_req = _ProcessRequests();

      res = _TakeBuffer(size, true);
   }

   if (process_req) ReplyReadyRequests();

   return res;
}

dabc::Buffer dabc::MemoryPool::TakeEmpty(unsigned capacity) throw()
{
   dabc::Buffer res;

   {
      LockGuard lock(ObjectMutex());

      if (capacity == 0) capacity = fMaxNumSegments;

      _TakeSegmentsList(this, res, capacity);
   }

   return res;
}


void dabc::MemoryPool::DuplicateBuffer(const Buffer& src, Buffer& tgt, unsigned firstseg, unsigned numsegm) throw()
{
   dabc::MemoryPool* pool = src.fRec ? src.fRec->fPool : 0;

   if (numsegm == 0) numsegm = src.NumSegments();
   if (firstseg >= src.NumSegments()) return;
   if (firstseg + numsegm > src.NumSegments())
      numsegm = src.NumSegments() - numsegm;

   LockGuard lock(pool ? pool->ObjectMutex() : 0);

   _TakeSegmentsList(pool, tgt, numsegm);

   MemSegment* src_segm = src.Segments();
   MemSegment* tgt_segm = tgt.Segments();

   for (unsigned cnt=0; cnt<numsegm; cnt++) {
      tgt_segm[cnt] = src_segm[firstseg + cnt];

      // increment reference counter on the memory space
      if (pool) pool->fMem->fArr[tgt_segm[cnt].id].refcnt++;
   }

   tgt.SetNumSegments(numsegm);

   tgt.SetTypeId(src.GetTypeId());
}

dabc::Buffer dabc::MemoryPool::CopyBuffer(const Buffer& src, bool except) throw()
{
   dabc::Buffer res;

   if (src.GetTotalSize()==0) return res;

   res = TakeBuffer(src.GetTotalSize());

   res.CopyFrom(src, src.GetTotalSize());

   return res;
}


void dabc::MemoryPool::IncreaseSegmRefs(MemSegment* segm, unsigned num) throw()
{
   LockGuard lock(ObjectMutex());

   if (fMem==0)
      throw dabc::Exception("Memory was not allocated in the pool");

   for (unsigned cnt=0;cnt<num;cnt++) {
      unsigned id = segm[cnt].id;
      if (id>fMem->fNumber)
         throw dabc::Exception("Wrong buffer id in the segments list of buffer");

      if (fMem->fArr[id].refcnt + 1 == 0)
         throw dabc::Exception("To many references on single segments - how it can be");

      fMem->fArr[id].refcnt++;
   }
}

void dabc::MemoryPool::DecreaseSegmRefs(MemSegment* segm, unsigned num) throw()
{
   LockGuard lock(ObjectMutex());

   if (fMem==0)
      throw dabc::Exception("Memory was not allocated in the pool");

   for (unsigned cnt=0;cnt<num;cnt++) {
      unsigned id = segm[cnt].id;
      if (id>fMem->fNumber)
         throw dabc::Exception("Wrong buffer id in the segments list of buffer");

      if (fMem->fArr[id].refcnt == 0)
         throw dabc::Exception("Reference counter of specified segment is already 0");

      fMem->fArr[id].refcnt--;
   }

}

bool dabc::MemoryPool::_ReleaseBufferRec(dabc::Buffer::BufferRec* rec)
{
   if ((rec==0) || (rec->fPool != this)) {
      EOUT(("Internal error - Wrong buffer record"));
      return false;
   }

   // simply decrement counter, no need to try destroy ourself
   _DecObjectRefCnt();

   MemSegment* segs = rec->Segments();
   unsigned size = rec->fNumSegments;

   bool isnewfree(false);

   for (unsigned cnt=0; cnt<size; cnt++) {
      unsigned id = segs[cnt].id;

      // very special case - in some cases part of segments could be disabled in that way
      // than one should not try to dec ref counter of buffer 0
      if ((id==0) && (segs[cnt].buffer==0) && (segs[cnt].datasize==0)) continue;

      if (fMem->fArr[id].refcnt == 0) throw dabc::Exception("Memory release more times as it is referenced");

      // decrement reference counter on the memory space
      fMem->fArr[id].refcnt--;

      if (fMem->fArr[id].refcnt==0) {
         fMem->fFree.Push(id);
         isnewfree = true;
      }
   }

   unsigned refid = rec->fPoolId;
   // 0 is special case - record was created by Buffer itself
   if (refid>0) {
      refid--;
      if (fSeg->fArr[refid].refcnt!=1) {
         EOUT(("HARD FAILURE refid=%u refcnt=%u", refid, (unsigned) fSeg->fArr[refid].refcnt));
         throw dabc::Exception("Segments list should referenced at this time exactly once");
      }
      fSeg->fArr[refid].refcnt--;
      fSeg->fFree.Push(refid);
   }
   return isnewfree;
}

void dabc::MemoryPool::ReleaseBufferRec(dabc::Buffer::BufferRec* rec)
{

   bool dofire(false), process_req(false);

   {
      LockGuard lock(ObjectMutex());

      bool isnewfree = _ReleaseBufferRec(rec);

      if (isnewfree && !fReqQueue.Empty()) {
         if (fUseThread) {
           if (!fEvntFired) { fEvntFired = true; dofire = true; }
         } else {
            process_req = _ProcessRequests();
         }
      }
   }

   // if requests were processed, inform all receivers outside locked area
   if (process_req) ReplyReadyRequests();

   // process of submitted requests is only allowed in pool thread (if it is assigned)
   if (dofire) FireEvent(evntProcessRequests);
}


dabc::Buffer dabc::MemoryPool::TakeBufferReq(MemoryPoolRequester* req, BufferSize_t size) throw()
{
   dabc::Buffer res;

   if (req==0) return res;

   bool process_req(false);

   {

      LockGuard lock(ObjectMutex());

      if ((req->pool!=0) && (req->pool!=this)) {
         EOUT(("Requester is used for other memory pool"));
         throw dabc::Exception("Requester is used for other memory pool");
         return res;
      }

      if (!req->buf.null()) {
         res << req->buf;
         req->pool = 0;

         if ((size>0) && (size>res.GetTotalSize())) {
            throw dabc::Exception("Requested buffer size is bigger than already prepared buffer");
            return res;
         }

         return res;
      }

      if (fReqQueue.Size()>0)
         process_req = _ProcessRequests();

      res = _TakeBuffer(size, true);

      if (res.null()) {
         req->bufsize = size;

         if (req->pool==0) {
            req->pool = this;
            fReqQueue.Push(req);
         }
      }
   }

   if (process_req) ReplyReadyRequests();

   return res;
}

void dabc::MemoryPool::RemoveRequester(MemoryPoolRequester* req)
{
   if (req==0) return;

   dabc::Buffer buf;

   if (req->pool==this) {
      LockGuard lock(ObjectMutex());
      req->pool = 0;
      fReqQueue.Remove(req);
      buf << req->buf;
      if (!req->buf.null()) {
         EOUT(("Internal error - buffer MUST be empty"));
         exit(765);
      }
   }

   buf.Release();
}

bool dabc::MemoryPool::_ProcessRequests()
{
   // method return true when at least one request is processed
   // in this case one should call ReplyReadyRequests without the locked mutex

   bool res = false;

   for(unsigned n=0; n<fReqQueue.Size(); n++) {
      MemoryPoolRequester* req = fReqQueue.Item(n);

      if (req->buf.null())
         req->buf = _TakeBuffer(req->bufsize, false);

      if (!req->buf.null()) res = true;
   }

   return res;
}

void dabc::MemoryPool::ReplyReadyRequests()
{
   MemoryPoolRequester* req(0);

   do {
      // give signal to requester that buffer is ready
      // principal here - make call outside locked mutex
      if (req!=0)
         if (req->ProcessPoolRequest()) req = 0;

      if (req!=0) req->buf.Release();

      req = 0;

      LockGuard guard(ObjectMutex());

      for(unsigned n=0; n<fReqQueue.Size(); n++)
         if (!fReqQueue.Item(n)->buf.null()) {
            req = fReqQueue.Item(n);
            req->pool = 0;
            fReqQueue.RemoveItem(n);
            break;
         }
   } while (req!=0);
}


void dabc::MemoryPool::ProcessEvent(const EventId& ev)
{

   if (ev.GetCode() == evntProcessRequests) {

      bool process_req(false);

      {
         LockGuard guard(ObjectMutex());

         fEvntFired = false;

         process_req = _ProcessRequests();
      }

      if (process_req) ReplyReadyRequests();

      return;
   }

   dabc::Worker::ProcessEvent(ev);
}

bool dabc::MemoryPool::CheckChangeCounter(unsigned &cnt)
{
   bool res = cnt!=fChangeCounter;

   cnt = fChangeCounter;

   return res;
}

bool dabc::MemoryPool::Reconstruct(Command cmd)
{
   unsigned buffersize = Cfg(xmlBufferSize, cmd).AsUInt(GetDfltBufSize());

//   dabc::SetDebugLevel(2);
   unsigned numbuffers = Cfg(xmlNumBuffers, cmd).AsUInt();
//   dabc::SetDebugLevel(0);
   unsigned refcoeff = Cfg(xmlRefCoeff, cmd).AsUInt(GetDfltRefCoeff());
   unsigned numsegm = Cfg(xmlNumSegments, cmd).AsUInt(GetDfltNumSegments());
   unsigned align = Cfg(xmlAlignment, cmd).AsUInt(GetDfltAlignment());

   DOUT2(("POOL:%s bufsize:%u X num:%u", GetName(), buffersize, numbuffers));

   if (align) SetAlignment(align);
   if (numsegm) SetMaxNumSegments(numsegm);

   return Allocate(buffersize, numbuffers, refcoeff);
}

double dabc::MemoryPool::GetUsedRatio() const
{
   LockGuard lock(ObjectMutex());

   if (fMem==0) return 0.;

   double sum1(0.), sum2(0.);
   for(unsigned n=0;n<fMem->fNumber;n++) {
      sum1 += fMem->fArr[n].size;
      if (fMem->fArr[n].refcnt>0)
         sum2 += fMem->fArr[n].size;
   }

   return sum1>0. ? sum2/sum1 : 0.;

}
