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
      Shift(ptr, ptr.rawsize());
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
      Shift(ptr, partlen);
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

      DOUT0(("Move segm %u->%u", n, n + numrequired - NumSegments()));

      segm[n + numrequired - NumSegments()] = segm[n];
      segm[n].datasize = 0;
      segm[n].id = 0;
      segm[n].buffer = 0;
   }

   MemSegment* srcsegm = ownbuf.Segments();
   // copy all segments from external buffer
   for (unsigned n=0;n<ownbuf.NumSegments();n++) {
      DOUT0(("copy segm src[%u]->tgt[%u]", n, tgtseg + n + (tgtpos>0 ? 1 : 0)));
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

void dabc::Buffer::Shift(Pointer& ptr, BufferSize_t len) const
{
   if (len < ptr.rawsize())
      ptr.shift(len);
   else
   if (len >= ptr.fullsize())
      ptr.reset();
   else {
      ptr.fFullSize -= ptr.rawsize();
      len -= ptr.rawsize();
      ptr.fRawSize = 0;
      ptr.fPtr = 0;
      ptr.fSegm++;

      while ((ptr.fSegm < NumSegments()) && (SegmentSize(ptr.fSegm) < len)) {
         len -= SegmentSize(ptr.fSegm);
         ptr.fFullSize -= SegmentSize(ptr.fSegm);
         ptr.fSegm++;
      }

      if (ptr.fSegm >= NumSegments())
         throw dabc::Exception("Pointer has invalid full length field");

      ptr.fPtr = (unsigned char*) SegmentPtr(ptr.fSegm) + len;
      ptr.fRawSize = SegmentSize(ptr.fSegm) - len;
      ptr.fFullSize -= len;
   }
}

int dabc::Buffer::Distance(const Pointer& ptr1, const Pointer& ptr2) const
{
   if (ptr1.null() || ptr2.null()) return 0;

   if ((ptr1.fSegm >= NumSegments()) || (ptr2.fSegm >= NumSegments()))
      throw dabc::Exception("Pointer with wrong segment id is specified");

   if (ptr1.fSegm > ptr2.fSegm) return -Distance(ptr2, ptr1);

   if (ptr1.fSegm==ptr2.fSegm) {
      if (ptr1.fPtr <= ptr2.fPtr) return ptr2.fPtr - ptr1.fPtr;
      return - (ptr1.fPtr - ptr2.fPtr);
   }

   unsigned nseg = ptr1.fSegm;

   // we produce first negative value,
   // but than full segment size will be accumulated in following while loop
   int sum = - (ptr1.fPtr - (unsigned char*) SegmentPtr(nseg));

   while (nseg < ptr2.fSegm) {
     sum+=SegmentSize(nseg);
     nseg++;
   }

   sum += (ptr2.fPtr - (unsigned char*) SegmentPtr(nseg));

   return sum;
}


dabc::BufferSize_t dabc::Buffer::CopyFrom(Pointer tgtptr, const Buffer& srcbuf, Pointer srcptr, BufferSize_t len) throw()
{
   BufferSize_t maxlen = tgtptr.fullsize();
   if (srcptr.fullsize()<maxlen) maxlen = srcptr.fullsize();
   if (len==0) len = maxlen; else if (len>maxlen) len = maxlen;

   BufferSize_t res(0);

   while (len>0) {
      unsigned copylen = (tgtptr.rawsize() < srcptr.rawsize()) ? tgtptr.rawsize() : srcptr.rawsize();
      if (copylen>len) copylen = len;
      if (copylen==0) break;

      ::memcpy(tgtptr(), srcptr(), copylen);

      len-=copylen;
      res+=copylen;

      Shift(tgtptr, copylen);
      srcbuf.Shift(srcptr, copylen);
   }

   return res;
}

dabc::BufferSize_t dabc::Buffer::CopyFrom(Pointer tgtptr, Pointer srcptr, BufferSize_t len) throw()
{
   if (len==0) len = srcptr.fullsize();
   if (len>tgtptr.fullsize()) len = tgtptr.fullsize();
   if (srcptr.rawsize() < len)
     throw dabc::Exception("Cannot use this CopyFrom() signature to copy from Pointer, use signature with Buffer");

   BufferSize_t res(0);

   while (len>0) {
      unsigned copylen = tgtptr.rawsize();
      if (copylen>len) copylen = len;

      if (copylen==0) break;

      ::memcpy(tgtptr(), srcptr(), copylen);

      len-=copylen;
      res+=copylen;

      Shift(tgtptr, copylen);
      srcptr.shift(copylen);
   }

   return res;
}

dabc::BufferSize_t dabc::Buffer::CopyTo(Pointer srcptr, void* ptr, BufferSize_t len) const throw()
{
   if (len>srcptr.fullsize()) len = srcptr.fullsize();

   BufferSize_t res(0);

   while (len>0) {
      unsigned copylen = srcptr.rawsize();
      if (copylen>len) copylen = len;

      if (copylen==0) break;

      ::memcpy(ptr, srcptr(), copylen);

      len-=copylen;
      res+=copylen;

      Shift(srcptr, copylen);
      ptr = (unsigned char*) ptr + copylen;
   }

   return res;
}

dabc::BufferSize_t dabc::Buffer::CopyFromStr(Pointer tgtptr, const char* src, unsigned len) throw()
{
   if (src==0) return 0;
   if (len==0) len = strlen(src);
   return CopyFrom(tgtptr, src, len);
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
