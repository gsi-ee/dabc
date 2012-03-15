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
#include "dabc/logging.h"

const dabc::BufferSize_t dabc::BufferSizeError = (dabc::BufferSize_t) -1;

dabc::Buffer::Buffer() :
   fRec(0)
{
//   DOUT0(("Buffer default constructor obj %p", this));
}

dabc::Buffer::Buffer(const Buffer& src) throw() :
   fRec(src.fRec)
{
   //   DOUT0(("Buffer copy constructor obj %p", this));

   if (fRec) fRec->increfcnt();
}

dabc::Buffer::~Buffer()
{
   DOUT5(("Buffer destructor obj %p", this));

   Release();
}

void dabc::Buffer::Release() throw()
{
   if (fRec && fRec->decrefcnt()) {
      // TODO: if record allocated by pool, use lock pool mutex
      // we need to release all segments and record itself

      bool del = (fRec->fPool==0) || (fRec->fPoolId==0);

      if (fRec->fPool)
         fRec->fPool->ReleaseBufferRec(fRec);
      else
      if (fRec->fPoolId == (unsigned) -1) {
         for (unsigned n=0;n<NumSegments();n++)
            free(SegmentPtr(n));
      }

      // now just free record memory
      if (del) free(fRec);
   }

   fRec = 0;
}

void dabc::Buffer::AssignRec(void* rec, unsigned recfullsize)
{
   if (fRec!=0) { EOUT(("Internal error - buffer MUST be empty")); exit(6); }
   if (recfullsize < sizeof(dabc::Buffer::BufferRec)) {
      EOUT(("record memory too small!!!"));
      exit(7);
   }
   memset(rec, 0, recfullsize);

   fRec = (BufferRec*) rec;

   fRec->fRefCnt = 1;            // now we are the only reference
   fRec->fCapacity = (recfullsize - sizeof(dabc::Buffer::BufferRec)) / sizeof(MemSegment);
}


void dabc::Buffer::AllocateRec(unsigned len)
{
   if (fRec!=0) { EOUT(("Internal error - buffer MUST be empty")); exit(6); }

   unsigned capacity(4);
   while ((capacity<len) && (capacity!=0)) capacity*=2;
   if (capacity==0) capacity = len;

   unsigned recfullsize = sizeof(BufferRec) + capacity*sizeof(dabc::MemSegment);

   fRec = (BufferRec*) malloc(recfullsize);

   if (fRec==0) { EOUT(("Cannot allocate record!!!")); return; }

   memset(fRec, 0, recfullsize);

   fRec->fRefCnt = 1;            // now we are the only reference
   fRec->fCapacity = capacity;   // remember allocated segments list
}


dabc::BufferSize_t dabc::Buffer::GetTotalSize() const
{
   BufferSize_t sz = 0;
   for (unsigned n=0;n<NumSegments();n++)
      sz += SegmentSize(n);
   return sz;
}

void dabc::Buffer::SetTotalSize(BufferSize_t len) throw()
{
   if (len==0) {
      Release();
      return;
   }

   if (null()) return;

   BufferSize_t totalsize = GetTotalSize();
   if (len > totalsize)
      throw dabc::Exception("Cannot extend size of the buffer by SetTotalSize method, use Append() method instead");

   if (len == totalsize) return;

   unsigned nseg(0), npos(0);
   Locate(len, nseg, npos);

   if (nseg >= NumSegments())
      throw dabc::Exception("Cannot happen - internal error");

   if (npos>0) {
      fRec->Segment(nseg)->datasize = npos;
      nseg++;
   }

   // we should release peaces which are no longer required

   if (nseg<NumSegments()) {
      if (fRec->fPool)
         fRec->fPool->DecreaseSegmRefs(Segments()+nseg, NumSegments() - nseg);

      fRec->fNumSegments = nseg;
   }
}


dabc::Buffer& dabc::Buffer::operator=(const Buffer& src) throw()
{
   // this is central point of understanding that Buffer is
   // it is just smart pointer on the structure which describes gather-list of
   // memory regions from SINGLE memory pool
   // When copy constructor or assign operator is used,
   // just pointer on such structure is duplicated.
   // Means if afterwards one object will be modified (add/remove/resize) segments,
   // all other objects will be also changed
   // Thus, copies of the same buffer object could be used only in the SINGLE thread
   // Only if buffer shifted from one thread to another, this could work.
   // If one requires to keep reference on the same memory from different threads,
   // dabc::Buffer::Duplicate() method should be used

   Release();

   fRec = src.fRec;

   if (fRec) fRec->increfcnt();

   return *this;
}

dabc::Buffer dabc::Buffer::HandOver()
{
   Buffer res;
   res.fRec = fRec;
   fRec = 0;
   return res;
}

dabc::Buffer& dabc::Buffer::operator<<(Buffer& src) throw()
{
   Release();

   fRec = src.fRec;
   src.fRec = 0;

   return *this;
}

dabc::Buffer dabc::Buffer::Duplicate() const
{
   dabc::Buffer res;

   if (fRec==0) return res;

   dabc::MemoryPool::DuplicateBuffer(*this, res);

   return res;
}

void dabc::Buffer::Release(Mutex* m) throw()
{
   if ((m==0) || null()) {
      Release();
   } else {
      Buffer b;
      {
         LockGuard lock(m);
         b << *this;
      }
      b.Release();
   }
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

   unsigned firstseg(0), lastseg(0);
   void* firstptr(0);
   BufferSize_t firstlen(0), lastlen(0);

   while ((len>0) && (ptr.fSegm<NumSegments())) {
      unsigned partlen(len);
      if (partlen>ptr.rawsize()) partlen = ptr.rawsize();
      if (partlen==0) break;

      if (firstptr==0) {
         firstptr = ptr.ptr();
         firstlen = partlen;
         firstseg = ptr.fSegm;
      }

      lastseg = ptr.fSegm;
      lastlen = partlen;

      len -= partlen;
      ptr.shift(partlen);
   }

   if (len>0) {
      EOUT(("Internal problem - not full length covered"));
      return res;
   }

   dabc::MemoryPool::DuplicateBuffer(*this, res, firstseg, lastseg-firstseg+1);

   if (res.null()) return res;

   res.Segments()[0].buffer = firstptr;
   res.Segments()[0].datasize = firstlen;

   if (lastseg>firstseg)
      res.Segments()[lastseg-firstseg].datasize = lastlen;

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


bool dabc::Buffer::Insert(BufferSize_t pos, Buffer& src, bool moverefs) throw()
{
   if (src.null()) return true;

   if (null()) {
      if (moverefs) {
         dabc::Buffer::operator=(src);
         src.Release();
      } else
         *this = src.Duplicate();
      return true;
   }

   Buffer ownbuf;

   // if we have buffer assigned to the pool and its differ from
   // source object we need deep copy to be able extend refs array
   if ((fRec->fPool!=0) && (src.fRec->fPool!=fRec->fPool)) {
      ownbuf = fRec->fPool->CopyBuffer(src);
   } else
   if (!moverefs)
      ownbuf = src.Duplicate();
   else
      ownbuf = src;


   unsigned tgtseg(0), tgtpos(0);

   Locate(pos, tgtseg, tgtpos);

   unsigned numrequired = NumSegments() + ownbuf.NumSegments();
   if (tgtpos>0) numrequired++;

   if (numrequired > fRec->fCapacity)
      throw dabc::Exception("Required number of segments less than available in the buffer");

   MemSegment* segm = Segments();

   // first move complete segments to the end
   for (unsigned n=NumSegments(); n>tgtseg + (tgtpos>0 ? 1 : 0); ) {
      n--;

      // DOUT0(("Move segm %u->%u", n, n + numrequired - NumSegments()));

      segm[n + numrequired - NumSegments()] = segm[n];
      segm[n].datasize = 0;
      segm[n].id = 0;
      segm[n].buffer = 0;
   }

   MemSegment* srcsegm = ownbuf.Segments();
   // copy all segments from external buffer
   for (unsigned n=0;n<ownbuf.NumSegments();n++) {
      // DOUT0(("copy segm src[%u]->tgt[%u]", n, tgtseg + n + (tgtpos>0 ? 1 : 0)));
      segm[tgtseg + n + (tgtpos>0 ? 1 : 0)] = srcsegm[n];
   }

   // in case when segment is divided on two parts
   if (tgtpos>0) {
      if(fRec->fPool) fRec->fPool->IncreaseSegmRefs(segm + tgtseg, 1);

      unsigned seg2 = tgtseg + ownbuf.NumSegments() + 1;

      segm[seg2].id = segm[tgtseg].id;
      segm[seg2].datasize = segm[tgtseg].datasize - tgtpos;
      segm[seg2].buffer = (char*) segm[tgtseg].buffer + tgtpos;

      segm[tgtseg].datasize = tgtpos;

      DOUT0(("split segment %u on two parts, second is in %u", tgtseg, seg2));
   }

   // at the end
   fRec->fNumSegments = numrequired;

   if (fRec->fPool && (ownbuf.fRec->fPool == fRec->fPool)) {
      // forget about all segments - they are moved to target
      ownbuf.fRec->fNumSegments = 0;
      ownbuf.Release();
   }

   if (moverefs) src.Release();

   return true;
}

dabc::Pointer dabc::Buffer::GetPointer(BufferSize_t pos, BufferSize_t len) const
{
   return Pointer(*this, pos, len);
}

void dabc::Buffer::Shift(Pointer& ptr, BufferSize_t len) const
{
   ptr.shift(len);
}


dabc::BufferSize_t dabc::Buffer::CopyTo(void* ptr, BufferSize_t len) const throw()
{
   return Pointer(*this).copyto(ptr, len);
}

std::string dabc::Buffer::AsStdString()
{
   std::string sbuf;

   if (null()) return sbuf;

   DOUT0(("Num segments = %u", NumSegments()));

   for (unsigned nseg=0; nseg<NumSegments(); nseg++) {
      DOUT0(("Segm %u = %p %u", nseg, SegmentPtr(nseg), SegmentSize(nseg)));
      sbuf.append((const char*)SegmentPtr(nseg), SegmentSize(nseg));
   }

   return sbuf;
}

dabc::Buffer dabc::Buffer::CreateBuffer(BufferSize_t sz) throw()
{
   return CreateBuffer(malloc(sz), sz, true);
}

dabc::Buffer dabc::Buffer::CreateBuffer(const void* ptr, unsigned sz, bool owner) throw()
{
   dabc::Buffer res;

   if ((ptr==0) || (sz==0)) return res;

   res.AllocateRec(4);

   MemSegment* segm = res.Segments();

   segm->id = 0;
   segm->datasize = sz;
   segm->buffer = (void*) ptr;

   res.fRec->fNumSegments = 1;

   if (owner) res.fRec->fPoolId = (unsigned) -1; // indicate that we are owner of the memory

   return res;
}

dabc::BufferSize_t dabc::Buffer::CopyFrom(const Buffer& srcbuf, BufferSize_t len) throw()
{
   return Pointer(*this).copyfrom(Pointer(srcbuf), len);
}


dabc::BufferSize_t dabc::Buffer::CopyFromStr(const char* src, unsigned len) throw()
{
   return Pointer(*this).copyfromstr(src, len);
}

