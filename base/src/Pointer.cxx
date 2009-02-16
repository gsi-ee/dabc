/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#include "dabc/Pointer.h"

#include <stdlib.h>
#include <string.h>

#include "dabc/logging.h"


void dabc::Pointer::setfullsize(BufferSize_t sz)
{
   if (fFullSize<sz) return;

   fFullSize = sz;
   if (fRawSize<sz) fRawSize = sz;
}


void dabc::Pointer::long_shift(BufferSize_t sz)
{
   // do not try shift further than fullsize
   if (sz>fFullSize) sz = fFullSize;

   if (fBuf==0) {
      fRawSize -= sz;
      fFullSize -= sz;
      fPtr += sz;
      return;
   }

   while (sz>=fRawSize) {
      sz -= fRawSize;
      fFullSize -= fRawSize;

      fSegm++;
      // theoretically, one should check that fSegm is not out of range,
      // we can save us this check in the future
      if (fSegm==fBuf->NumSegments()) {
         // this is pointer on the end of the buffer,
         // to keep possibility work with distance, make a trick

         if (fPtr==0) {
            EOUT(("Internal Problem !!!!"));
            exit(1);
         }

         fPtr += fRawSize;
         fRawSize = 0;
         return;
      } else
      if (fSegm > fBuf->NumSegments()) {
         EOUT(("Segment Id is larger than exists in buffer"));
         exit(1);
         return;
      }

      fPtr = (unsigned char*) fBuf->GetDataLocation(fSegm);
      fRawSize = fBuf->GetDataSize(fSegm);
   }

   shift(sz);
}

dabc::BufferSize_t dabc::Pointer::copyfrom(const Pointer& src, BufferSize_t sz)
{
   if (sz > src.fullsize()) {
      EOUT(("source pointer has no so much memory %lu", sz));
      sz = src.fullsize();
   }

   if (sz > fullsize()) {
      EOUT(("target pointer has no so much memory %lu", sz));
      sz = fullsize();
   }

   if (sz==0) return sz;

   // this is special and simple case, treat is separately
   if ((sz <= rawsize()) && (sz <= src.rawsize())) {
      ::memcpy(ptr(), src.ptr(), sz);
      return sz;
   }

   Pointer _tgt(*this), _src(src);

   dabc::BufferSize_t restsz(sz), copysz(0);

   while (restsz>0) {
      copysz = restsz;
      if (copysz>_tgt.rawsize()) copysz = _tgt.rawsize();
      if (copysz>_src.rawsize()) copysz = _src.rawsize();

      ::memcpy(_tgt.ptr(), _src.ptr(), copysz);

      restsz -= copysz;

      _tgt.shift(copysz);
      _src.shift(copysz);
   }

   return sz;
}

dabc::BufferSize_t dabc::Pointer::copyto(void* tgt, BufferSize_t sz)
{
   if (sz > fullsize()) {
      EOUT(("Source pointer has no such much memory %lu", sz));
      sz = fullsize();
   }

   if (sz==0) return sz;

   // this is special and simple case, treat is separately
   if (sz <= rawsize()) {
      ::memcpy(tgt, ptr(), sz);
      return sz;
   }

   Pointer _src(*this);

   dabc::BufferSize_t restsz(sz), copysz(0);

   while (restsz>0) {
      copysz = restsz;
      if (copysz>_src.rawsize()) copysz = _src.rawsize();

      ::memcpy(tgt, _src.ptr(), copysz);

      restsz -= copysz;

      tgt = (char*) tgt + copysz;
      _src.shift(copysz);
   }

   return sz;
}

dabc::BufferSize_t dabc::Pointer::copyfrom(void* src, BufferSize_t sz)
{
   if (sz > fullsize()) {
      EOUT(("target pointer has no so much memory %lu", sz));
      sz = fullsize();
   }

   if (sz==0) return sz;

   // this is special and simple case, treat is separately
   if (sz <= rawsize()) {
      ::memcpy(ptr(), src, sz);
      return sz;
   }

   Pointer _tgt(*this);

   dabc::BufferSize_t restsz(sz), copysz(0);

   while (restsz>0) {
      copysz = restsz;
      if (copysz>_tgt.rawsize()) copysz = _tgt.rawsize();

      ::memcpy(_tgt.ptr(), src, copysz);

      restsz -= copysz;

      _tgt.shift(copysz);
      src = (char*) src + copysz;
   }

   return sz;
}

dabc::BufferSize_t dabc::Pointer::distance_to(const Pointer& child) const
{
   // defines distance to child pointer, means child must be inside
   // region of parent pointer

   if (child.fBuf != fBuf) {
      EOUT(("Missmatch in buffer pointers"));
      return dabc::BufferSizeError;
   }

   if (fSegm == child.fSegm)
      return (fPtr <= child.fPtr) ? child.fPtr - fPtr : dabc::BufferSizeError;

   if (fSegm > child.fSegm) {
      EOUT(("Child lies in previous segments ???"));
      return dabc::BufferSizeError;
   }

   Pointer parent(*this);

   BufferSize_t total_distance = 0;

   // potentailly, endless loop, if Buffer is changed in between
   while (parent.fSegm < child.fSegm) {
      total_distance += parent.rawsize();
      parent.shift_to_segment();
   }

   BufferSize_t last_distance = parent.distance_to(child);

   if (last_distance == dabc::BufferSizeError) return dabc::BufferSizeError;

   return total_distance + last_distance;
}
