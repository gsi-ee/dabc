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

#include "dabc/Pointer.h"

#include <cstdlib>
#include <cstring>

#include "dabc/logging.h"
#include "dabc/Exception.h"


void dabc::Pointer::long_shift(BufferSize_t len)
{
   if (fFullSize == len) {
      fPtr += len;
      fFullSize = 0;
      fRawSize = 0;
      return;
   }

   fFullSize -= rawsize();
   len -= rawsize();
   fRawSize = 0;
   fPtr = 0;
   fSegm++;

   while ((fSegm < fBuf.NumSegments()) && (fBuf.SegmentSize(fSegm) < len)) {
      len -= fBuf.SegmentSize(fSegm);
      fFullSize -= fBuf.SegmentSize(fSegm);
      fSegm++;
   }

   if (fSegm >= fBuf.NumSegments())
      throw dabc::Exception(ex_Pointer, "Pointer has invalid full length field", "Pointer");

   fPtr = (unsigned char*) fBuf.SegmentPtr(fSegm) + len;
   fRawSize = fBuf.SegmentSize(fSegm) - len;
   fFullSize -= len;
}

dabc::BufferSize_t dabc::Pointer::copyfrom(const Pointer& src, BufferSize_t sz) throw()
{
   if (sz==0) sz = src.fullsize();

   // this is the simplest case - just raw copy of original data
   if ((sz <= rawsize()) && (sz <= src.rawsize())) {
      ::memcpy(ptr(), src(), sz);
      return sz;
   }

   if (sz > src.fullsize()) {
      EOUT("source pointer has no so much memory %u", (unsigned) sz);
      sz = src.fullsize();
   }

   if (sz > fullsize()) {
      EOUT("target pointer has no so much memory %u", (unsigned) sz);
      sz = fullsize();
   }

   Pointer tgtptr(*this), srcptr(src);
   unsigned res(0);

   while (sz>0) {
      unsigned copylen = (tgtptr.rawsize() < srcptr.rawsize()) ? tgtptr.rawsize() : srcptr.rawsize();
      if (copylen>sz) copylen = sz;
      if (copylen==0) break;

      ::memcpy(tgtptr(), srcptr(), copylen);

      res+=copylen;
      sz-=copylen;
      tgtptr.shift(copylen);
      srcptr.shift(copylen);
   }

   return res;
}

dabc::BufferSize_t dabc::Pointer::copyto(Pointer& src, BufferSize_t sz) const throw()
{
   return src.copyfrom(*this, sz ? sz : fullsize());
}

dabc::BufferSize_t dabc::Pointer::copyto(void* tgt, BufferSize_t sz) const throw()
{
   if (sz==0) return 0;

   if (sz <= rawsize()) {
      ::memcpy(tgt, ptr(), sz);
      return sz;
   }

   return Pointer(tgt, sz).copyfrom(*this, sz);
}

dabc::BufferSize_t dabc::Pointer::copyfrom(const void *src, BufferSize_t sz)
{
   if (!src) return 0;

   if (sz > fullsize()) {
      EOUT("target pointer has no so much memory %lu", sz);
      sz = fullsize();
   }

   if (sz==0) return sz;

   // this is special and simple case, treat is separately
   if (sz > rawsize())
      throw dabc::Exception(ex_Pointer,"Cannot copy memory outside current segment, use Buffer::copyfrom() method", "Pointer");

   ::memcpy(ptr(), src, sz);
   return sz;
}

dabc::BufferSize_t dabc::Pointer::copyfromstr(const char* str, unsigned len) throw()
{
   return str ? copyfrom(str, len ? len : strlen(str)) : 0;
}

int dabc::Pointer::distance_to(const Pointer& child) const
{
   if (fBuf != child.fBuf)
      throw dabc::Exception(ex_Pointer,"Pointer with wrong segment id is specified", "Pointer");

   if (fSegm==child.fSegm)
      return (fPtr <= child.fPtr) ? (child.fPtr - fPtr) : -(fPtr - child.fPtr);

   if (fSegm > child.fSegm) return -child.distance_to(*this);

   Pointer left(*this);
   BufferSize_t dist(0);

   while (left.fSegm < child.fSegm) {
      dist += left.rawsize();
      left.shift(left.rawsize());
   }

   return dist + (child.fPtr - left.fPtr);
}

float dabc::Pointer::consumed_size() const
{
   BufferSize_t bufsz = fBuf.GetTotalSize();
   return (bufsz == 0) ? 0. : 1. * distance_to_ownbuf() / bufsz;
}

