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
   fPool(),
   fPoolId(0),
   fSegments(0),
   fNumSegments(0),
   fCapacity(0),
   fTypeId(0)
{
//   DOUT0(("Buffer default constructor obj %p", this));
}

dabc::Buffer::Buffer(const Buffer& src) throw() :
   fPool(),
   fPoolId(0),
   fSegments(0),
   fNumSegments(0),
   fCapacity(0),
   fTypeId(0)
{
//   DOUT0(("Buffer copy constructor obj %p", this));

   *this = *(const_cast<Buffer*>(&src));
}

dabc::Buffer::~Buffer()
{
   DOUT5(("Buffer destructor obj %p", this));

   Release();
}

dabc::BufferSize_t dabc::Buffer::GetTotalSize() const
{
   BufferSize_t sz = 0;
   for (unsigned n=0;n<fNumSegments;n++)
      sz += fSegments[n].datasize;
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

   if (nseg >= fNumSegments)
      throw dabc::Exception("Cannot happen - internal error");

   if (npos>0) {
      fSegments[nseg].datasize = npos;
      nseg++;
   }

   // we should release peaces which are no longer required

   if (nseg<fNumSegments) {
      ((MemoryPool*)fPool())->DecreaseSegmRefs(fSegments+nseg, fNumSegments - nseg);

      fNumSegments = nseg;
   }
}


dabc::Buffer& dabc::Buffer::operator=(const Buffer& src) throw()
{
   // this is central point, if pool reference is moved from source object,
   // one could move all other fields from source object and reset it
   // if one could not do this, segments list should be duplicated

//   DOUT0(("Buffer assign operator obj %p", this));

   // TODO: VERY IMPORTANT
   // one should extract all fields to container class and
   // just copy/shift this pointer to the target buffer object
   // Using ref counter, one could release container when is no longer used

   Release();

   fPool = src.fPool;

   if (src.fPool.null()) {
      fPoolId = src.fPoolId;
      fSegments = src.fSegments;
      fNumSegments = src.fNumSegments;
      fCapacity = src.fCapacity;
      fTypeId = src.fTypeId;

      Buffer* obj = const_cast<Buffer*>(&src);
      obj->fPoolId = 0;
      obj->fSegments = 0;
      obj->fNumSegments = 0;
      obj->fCapacity = 0;
      obj->fTypeId = 0;
   } else
   if (!fPool.null()) {
      ((MemoryPool*) fPool())->DuplicateBuffer(src, *this);
   }

   return *this;
}

dabc::Buffer dabc::Buffer::Duplicate() const
{
   dabc::Buffer res;

   res.fPool = fPool.Ref();

   if (!res.fPool.null())
      ((MemoryPool*) res.fPool())->DuplicateBuffer(*this, res);

   return res;
}

dabc::Buffer& dabc::Buffer::operator<<(const Buffer& src)
{
//   DOUT0(("Shift operator this %p << src %p", this, &src));

   Release();

   fPool << src.fPool;

   Buffer* obj = const_cast<Buffer*>(&src);

   fPoolId = src.fPoolId;           obj->fPoolId = 0;
   fSegments = src.fSegments;       obj->fSegments = 0;
   fNumSegments = src.fNumSegments; obj->fNumSegments = 0;
   fCapacity = src.fCapacity;       obj->fCapacity = 0;
   fTypeId = src.fTypeId;           obj->fTypeId = 0;

   return *this;
}


void dabc::Buffer::Release() throw()
{
   if (!fPool.null()) {
      // TODO: in future one could try to lock pool mutex once,
      // for a moment we do it twice when release memory, referenced by the Buffer object
      ((MemoryPool*) fPool())->ReleaseBuffer(*this);
      fPool.Release();
   }

   fPoolId = 0;
   fSegments = 0;
   fNumSegments = 0;
   fCapacity = 0;
   fTypeId = 0;
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

   if (fPool.null() || (ptr.fullsize()==0)) return res;

   while (!allowsegmented && (len > ptr.rawsize())) {
      Shift(ptr, ptr.rawsize());
      if (ptr.fullsize() < len) break;
   }

   if (ptr.fullsize() < len) return res;

   if (len==0) len = ptr.rawsize();

   res << ((MemoryPool*) fPool())->TakeEmpty();

   if (res.fPool.null()) return res;

   unsigned rescnt(0);

   while ((len > 0) && (rescnt<res.fCapacity) && (ptr.fSegm<fNumSegments)) {
      unsigned partlen = len;
      if (partlen>ptr.rawsize()) partlen = ptr.rawsize();
      if (partlen==0) break;

      res.fSegments[rescnt].id = fSegments[ptr.fSegm].id;

      res.fSegments[rescnt].buffer = ptr.ptr();

      res.fSegments[rescnt].datasize = partlen;

      len -= partlen;
      Shift(ptr, partlen);
      rescnt++;
   }

   // at this moment number of segments is 0 therefore release just mean release of pool reference
   if (len>0) { res.Release(); return res; }

   ((MemoryPool*) res.fPool())->IncreaseSegmRefs(res.fSegments, rescnt);

   res.SetTypeId(GetTypeId());
   res.fNumSegments = rescnt;

   return res;
}


bool dabc::Buffer::Append(Buffer& src) throw()
{
   return Insert(GetTotalSize(), src);
}

bool dabc::Buffer::Prepend(Buffer& src) throw()
{
   return Insert(0, src);
}

void dabc::Buffer::Locate(BufferSize_t p, unsigned& seg_indx, unsigned& seg_shift) const
{
   seg_indx = 0;
   seg_shift = 0;

   BufferSize_t curr(0);

   while ((curr < p) && (seg_indx<fNumSegments)) {
      if (curr + fSegments[seg_indx].datasize <= p) {
         curr += fSegments[seg_indx].datasize;
         seg_indx++;
         continue;
      }

      seg_shift = p - curr;
      return;
   }
}


bool dabc::Buffer::Insert(BufferSize_t pos, Buffer& src) throw()
{
   if (src.null()) return true;

   if (null()) {
      dabc::Buffer::operator=(src);
      return true;
   }

   if (!fPool.null() &&  (fPool()!= src.fPool())) {
      Buffer buf2 = ((MemoryPool*)fPool())->CopyBuffer(src);
      buf2.SetTransient(true);
      return dabc::Buffer::Insert(pos, buf2);
   }

   unsigned tgtseg(0), tgtpos(0);

   Locate(pos, tgtseg, tgtpos);

   unsigned numrequired = fNumSegments + src.fNumSegments;
   if (tgtpos>0) numrequired++;

   if (numrequired > fCapacity)
      throw dabc::Exception("Required number of segments less than available in the buffer");

   if (!src.IsTransient())
      ((MemoryPool*)src.fPool())->IncreaseSegmRefs(src.fSegments, src.fNumSegments);

   // first move complete segments to the end
   for (unsigned n=fNumSegments; n>tgtseg + (tgtpos>0 ? 1 : 0); ) {
      n--;

      DOUT0(("Move segm %u->%u", n, n + numrequired - fNumSegments));

      fSegments[n + numrequired - fNumSegments] = fSegments[n];
      fSegments[n].datasize = 0;
      fSegments[n].id = 0;
      fSegments[n].buffer = 0;
   }

   // copy all segments from external buffer
   for (unsigned n=0;n<src.fNumSegments;n++) {
      DOUT0(("copy segm src[%u]->tgt[%u]", n, tgtseg + n + (tgtpos>0 ? 1 : 0)));
      fSegments[tgtseg + n + (tgtpos>0 ? 1 : 0)] = src.fSegments[n];
   }

   // in case when segment is divided on two parts
   if (tgtpos>0) {
      ((MemoryPool*)fPool())->IncreaseSegmRefs(fSegments + tgtseg, 1);

      unsigned seg2 = tgtseg + src.fNumSegments + 1;

      fSegments[seg2].id = fSegments[tgtseg].id;
      fSegments[seg2].datasize = fSegments[tgtseg].datasize - tgtpos;
      fSegments[seg2].buffer = (char*) fSegments[tgtseg].buffer + tgtpos;

      fSegments[tgtseg].datasize = tgtpos;

      DOUT0(("split segment %u on two parts, second is in %u", tgtseg, seg2));

   }

   // at the end
   fNumSegments = numrequired;

   if (src.IsTransient()) {
      // forget about all segments - they are moved to target
      src.fNumSegments = 0;
      src.Release();
   }

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

      while ((ptr.fSegm < NumSegments()) && (fSegments[ptr.fSegm].datasize < len)) {
         len -= fSegments[ptr.fSegm].datasize;
         ptr.fFullSize -= fSegments[ptr.fSegm].datasize;
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
   int sum = - (ptr1.fPtr - (unsigned char*) fSegments[nseg].buffer);

   while (nseg < ptr2.fSegm) {
     sum+=fSegments[nseg].datasize;
     nseg++;
   }

   sum += (ptr2.fPtr - (unsigned char*) fSegments[nseg].buffer);

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

   DOUT0(("Num segments = %u", fNumSegments));

   for (unsigned nseg=0; nseg<fNumSegments; nseg++) {
      DOUT0(("Segm %u = %p %u", nseg, fSegments[nseg].buffer, fSegments[nseg].datasize));
      sbuf.append((const char*)fSegments[nseg].buffer, fSegments[nseg].datasize);
   }

   return sbuf;
}



dabc::Buffer dabc::Buffer::CreateBuffer(BufferSize_t sz, unsigned numrefs, unsigned numsegm) throw()
{
   dabc::Buffer res;

   if (sz==0) return res;

   MemoryPool* pool1 = new MemoryPool("name", false);
   pool1->SetMaxNumSegments(numsegm);

   pool1->Allocate(sz, 1, numrefs);

   res << pool1->TakeBuffer(sz);

   // at the moment when reference will be released, pool should be deleted
   res.fPool.SetOwner(true);

   return res;
}

dabc::Buffer dabc::Buffer::CreateBuffer(const void* ptr, unsigned sz, bool owner, unsigned numrefs, unsigned numsegm) throw()
{
   dabc::Buffer res;

   if ((sz==0) || (ptr==0)) return res;

   MemoryPool* pool1 = new MemoryPool("name", false);
   pool1->SetMaxNumSegments(numsegm);

   std::vector<void*> ptrs; ptrs.push_back((void*)ptr);
   std::vector<unsigned> sizes; sizes.push_back(sz);

   pool1->Assign(owner, ptrs, sizes, numrefs);

   res << pool1->TakeBuffer(sz);

   // at the moment when last reference will be released, pool should be deleted
   res.fPool.SetOwner(true);

   return res;
}
