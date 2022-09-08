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

#include "dabc/Buffer.h"

#include "dabc/Pointer.h"
#include "dabc/MemoryPool.h"

const dabc::BufferSize_t dabc::BufferSizeError = (dabc::BufferSize_t) -1;

namespace dabc {


   /** Helper class to release memory, allocated independently from memory pool
    * Object will be deleted (with memory) when last reference in buffer disappear */
   class MemoryContainer : public Object {
      public:

         void* fPtr;

         MemoryContainer(void *ptr = nullptr) :
            Object(nullptr, "", flAutoDestroy),
            fPtr(ptr)
         {
           #ifdef DABC_EXTRA_CHECKS
             DebugObject("Memory", this, 1);
           #endif
         }

         virtual ~MemoryContainer()
         {
            if (fPtr) { std::free(fPtr); fPtr = nullptr; }
           #ifdef DABC_EXTRA_CHECKS
              DebugObject("Memory", this, -1);
           #endif
         }
   };
}


dabc::BufferContainer::~BufferContainer()
{
   dabc::MemoryPool* pool = dynamic_cast<dabc::MemoryPool*> (fPool());

//   DOUT0("~dabc::BufferContainer %p  THRD %s\n", this, dabc::mgr.CurrentThread().GetName());

   if (pool) {
      pool->DecreaseSegmRefs(fSegm, fNumSegments);
//      DOUT0("Release %u segments first id %u", fNumSegments, fSegm[0].id);
   } else {

   }

   fPool.Release();

#ifdef DABC_EXTRA_CHECKS
   DebugObject("Buffer", this, -1);
#endif

}


// ========================================================


dabc::Reference dabc::Buffer::GetPool() const
{
   return PoolPtr();
}

dabc::MemoryPool* dabc::Buffer::PoolPtr() const
{
   if (null()) return nullptr;

   return dynamic_cast<MemoryPool*> (GetObject()->fPool());
}


dabc::BufferSize_t dabc::Buffer::GetTotalSize() const
{
   BufferSize_t sz = 0;
   for (unsigned n=0;n<NumSegments();n++)
      sz += SegmentSize(n);
   return sz;
}

void dabc::Buffer::SetTotalSize(BufferSize_t len)
{
   if (len==0) {
      Release();
      return;
   }

   if (null()) return;

   BufferSize_t totalsize = GetTotalSize();
   if (len > totalsize)
      throw dabc::Exception(dabc::ex_Buffer, "Cannot extend size of the buffer by SetTotalSize method, use Append() method instead", "Buffer");

   if (len == totalsize) return;

   unsigned nseg = 0, npos = 0;
   Locate(len, nseg, npos);

   if (nseg >= NumSegments())
      throw dabc::Exception(dabc::ex_Buffer, "FATAL nseg>=NumSegments", dabc::format("Buffer numseg %u", NumSegments()));

   if (npos > 0) {
      GetObject()->fSegm[nseg].datasize = npos;
      nseg++;
   }

   // we should release peaces which are no longer required

   if (nseg<NumSegments()) {

      dabc::MemoryPool* pool = (dabc::MemoryPool*) GetObject()->fPool();

      if (pool)
         pool->DecreaseSegmRefs(Segments()+nseg, NumSegments() - nseg);

      GetObject()->fNumSegments = nseg;
   }
}


void dabc::Buffer::CutFromBegin(BufferSize_t len)
{
   if (len==0) return;

   if (len>=GetTotalSize()) {
      Release();
      return;
   }

   unsigned nseg = 0, npos = 0;
   Locate(len, nseg, npos);

   if (nseg >= NumSegments())
      throw dabc::Exception(dabc::ex_Buffer, "FATAL nseg >= NumSegments()", dabc::format("Buffer numseg %u", NumSegments()));

   // we should release segments which are no longer required
   if (nseg > 0) {

      if (PoolPtr()) PoolPtr()->DecreaseSegmRefs(Segments(), nseg);

      for (unsigned n=0;n<NumSegments()-nseg;n++)
         GetObject()->fSegm[n].copy_from(GetObject()->fSegm + n + nseg);

      GetObject()->fNumSegments -= nseg;
   }

   if (npos > 0) {
      GetObject()->fSegm[0].datasize -= npos;
      GetObject()->fSegm[0].buffer = (char*) (GetObject()->fSegm[0].buffer) + npos;
   }
}


void dabc::Buffer::Locate(BufferSize_t p, unsigned& seg_indx, unsigned& seg_shift) const
{
   seg_indx = 0;
   seg_shift = 0;

   BufferSize_t curr(0);

   while ((curr < p) && (seg_indx<NumSegments())) {
      if (curr + SegmentSize(seg_indx) <= p) {
         curr += SegmentSize(seg_indx);
         seg_indx++;
         continue;
      }

      seg_shift = p - curr;
      return;
   }
}


dabc::Buffer dabc::Buffer::Duplicate() const
{
   dabc::Buffer res;

   if (null()) return res;

   if (PoolPtr()) PoolPtr()->IncreaseSegmRefs(Segments(), NumSegments());

   res.AllocateContainer(GetObject()->fCapacity);

   res.GetObject()->fPool = GetObject()->fPool;

   res.SetTypeId(GetTypeId());

   res.GetObject()->fNumSegments = NumSegments();

   for (unsigned n=0;n<NumSegments();n++)
      res.GetObject()->fSegm[n].copy_from(GetObject()->fSegm + n);

   return res;
}


bool dabc::Buffer::Append(Buffer& src, bool moverefs) throw()
{
   return Insert(GetTotalSize(), src, moverefs);
}

bool dabc::Buffer::Prepend(Buffer& src, bool moverefs) throw()
{
   return Insert(0, src, moverefs);
}

bool dabc::Buffer::Insert(BufferSize_t pos, Buffer& src, bool moverefs)
{
   if (src.null() || (src.GetTotalSize() == 0)) return true;

   if (null()) {
      if (moverefs)
         *this << src;
      else
         *this = src.Duplicate();
      return true;
   }

   Buffer ownbuf;

   // if we have buffer assigned to the pool and its differ from
   // source object we need deep copy to be able extend refs array
   if (PoolPtr() && (src.PoolPtr() != PoolPtr())) {

      // first new memory allocated in our pool
      ownbuf = PoolPtr()->TakeBuffer(src.GetTotalSize());

      // and second memory is copied
      ownbuf.CopyFrom(src, src.GetTotalSize());
   } else
   if (!moverefs) {
      // we just duplicate list with refs
      ownbuf = src.Duplicate();
   } else
      ownbuf = src;


   unsigned tgtseg = 0, tgtpos = 0;

   Locate(pos, tgtseg, tgtpos);

   unsigned numrequired = NumSegments() + ownbuf.NumSegments();
   if (tgtpos>0) numrequired++;

   if (numrequired > GetObject()->fCapacity)
      throw dabc::Exception(dabc::ex_Buffer, "Required number of segments less than available in the buffer", dabc::format("Buffer numseg %u", NumSegments()));

   MemSegment *segm = Segments();

   // first move complete segments to the end
   for (unsigned n = NumSegments(); n > tgtseg + (tgtpos>0 ? 1 : 0); ) {
      n--;

      // DOUT0("Move segm %u->%u", n, n + numrequired - NumSegments());

      segm[n + numrequired - NumSegments()] = segm[n];
      segm[n].datasize = 0;
      segm[n].id = 0;
      segm[n].buffer = 0;
   }

   MemSegment *srcsegm = ownbuf.Segments();
   // copy all segments from external buffer
   for (unsigned n=0;n<ownbuf.NumSegments();n++) {
      // DOUT0("copy segm src[%u]->tgt[%u]", n, tgtseg + n + (tgtpos>0 ? 1 : 0));
      segm[tgtseg + n + (tgtpos>0 ? 1 : 0)].copy_from(&(srcsegm[n]));
   }

   // in case when segment is divided on two parts
   if (tgtpos>0) {
      if(PoolPtr()) PoolPtr()->IncreaseSegmRefs(segm + tgtseg, 1);

      unsigned seg2 = tgtseg + ownbuf.NumSegments() + 1;

      segm[seg2].id = segm[tgtseg].id;
      segm[seg2].datasize = segm[tgtseg].datasize - tgtpos;
      segm[seg2].buffer = (char*) segm[tgtseg].buffer + tgtpos;

      segm[tgtseg].datasize = tgtpos;

      DOUT0("split segment %u on two parts, second is in %u", tgtseg, seg2);
   }

   // at the end
   GetObject()->fNumSegments = numrequired;

   if (PoolPtr() && (ownbuf.PoolPtr() == PoolPtr())) {
      // forget about all segments - they are moved to target
      ownbuf.GetObject()->fNumSegments = 0;
      ownbuf.Release();
   }

   if (moverefs) src.Release();

   return true;
}


std::string dabc::Buffer::AsStdString()
{
   std::string sbuf;

   if (null()) return sbuf;

   DOUT0("Num segments = %u", NumSegments());

   for (unsigned nseg=0; nseg<NumSegments(); nseg++) {
      DOUT0("Segm %u = %p %u", nseg, SegmentPtr(nseg), SegmentSize(nseg));
      sbuf.append((const char*)SegmentPtr(nseg), SegmentSize(nseg));
   }

   return sbuf;
}

dabc::BufferSize_t dabc::Buffer::CopyFrom(const Buffer& srcbuf, BufferSize_t len) throw()
{
   return Pointer(*this).copyfrom(Pointer(srcbuf), len);
}


dabc::BufferSize_t dabc::Buffer::CopyFromStr(const char* src, unsigned len) throw()
{
   return Pointer(*this).copyfromstr(src, len);
}

dabc::BufferSize_t dabc::Buffer::CopyTo(void* ptr, BufferSize_t len) const throw()
{
   return Pointer(*this).copyto(ptr, len);
}

dabc::Buffer dabc::Buffer::GetNextPart(Pointer& ptr, BufferSize_t len, bool allowsegmented) throw()
{
   dabc::Buffer res;

   if (ptr.fullsize()<len) return res;

   while (!allowsegmented && (len > ptr.rawsize())) {
      ptr.shift(ptr.rawsize());
      if (ptr.fullsize() < len) break;
   }

   if (ptr.fullsize() < len) return res;

   unsigned firstseg = 0, lastseg = 0;
   void* firstptr = nullptr;
   BufferSize_t firstlen = 0, lastlen = 0;

   while ((len>0) && (ptr.fSegm<NumSegments())) {
      unsigned partlen(len);
      if (partlen>ptr.rawsize()) partlen = ptr.rawsize();
      if (partlen == 0) break;

      if (!firstptr) {
         firstptr = ptr.ptr();
         firstlen = partlen;
         firstseg = ptr.fSegm;
      }

      lastseg = ptr.fSegm;
      lastlen = partlen;

      len -= partlen;
      ptr.shift(partlen);
   }

   if (len > 0) {
      EOUT("Internal problem - not full length covered");
      return res;
   }

   res.AllocateContainer(GetObject()->fCapacity);

   MemoryPool* pool = PoolPtr();

   if (pool) {
      pool->IncreaseSegmRefs(Segments() + firstseg, lastseg-firstseg+1);

      res.GetObject()->fPool.SetObject(pool);
   }

   for (unsigned n=firstseg;n<=lastseg;n++) {

      res.Segments()[n-firstseg].copy_from(Segments() + n);
      if (n==firstseg) {
         res.Segments()->buffer = firstptr;
         res.Segments()->datasize = firstlen;
      }
      if (n==lastseg)
         res.Segments()[n-firstseg].datasize = lastlen;
   }

   res.GetObject()->fNumSegments = lastseg - firstseg + 1;

//   DOUT0("GetNextPart returns %u", res.GetTotalSize());

   return res;
}

dabc::Buffer dabc::Buffer::CreateBuffer(BufferSize_t sz) throw()
{
   void *rawbuf = (sz > 0) ? std::malloc(sz) : nullptr;
   return CreateBuffer(rawbuf, sz, true);
}

dabc::Buffer dabc::Buffer::CreateBuffer(const void* ptr, unsigned size, bool owner, bool makecopy) throw()
{
   dabc::Buffer res;

   res.AllocateContainer(8);

   if (owner) {
      res.GetObject()->fPool = new MemoryContainer((void*)ptr);
   } else if (makecopy) {
      void *newptr = std::malloc(size);
      if (!newptr) {
         EOUT("Failed to allocate buffer of size %u", size);
         return res;
      }
      memcpy(newptr, ptr, size);
      res.GetObject()->fPool = new MemoryContainer((void*)newptr);
      ptr = newptr;
   }

   res.GetObject()->fNumSegments = 1;
   res.GetObject()->fSegm[0].buffer = (void*) ptr;
   res.GetObject()->fSegm[0].datasize = size;

   return res;
}


bool dabc::Buffer::CanSafelyChange() const
{
   MemoryPool* pool = PoolPtr();
   if (!null() && (pool!=0))
      return pool->IsSingleSegmRefs(GetObject()->fSegm, GetObject()->fNumSegments);
   return true;

}



void dabc::Buffer::AllocateContainer(unsigned capacity)
{
   Release();

   unsigned obj_size = sizeof(BufferContainer) + sizeof(MemSegment) * capacity;

   void* area = std::malloc(obj_size);

   BufferContainer* cont = new (area) BufferContainer;

   cont->fCapacity = capacity;
   cont->fSegm = (MemSegment*) ((char*) area + sizeof(BufferContainer));

   memset(cont->fSegm, 0, sizeof(MemSegment) * capacity);

   *this = cont;
}
