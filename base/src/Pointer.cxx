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

#include <stdlib.h>
#include <string.h>

#include "dabc/logging.h"
#include "dabc/Exception.h"


void dabc::Pointer::long_shift(BufferSize_t sz) throw()
{
   throw dabc::Exception("Cannot shift pointer on large size, used Buffer::shift() method instead");
}

dabc::BufferSize_t dabc::Pointer::copyfrom(const Pointer& src, BufferSize_t sz) throw()
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

   if ((sz > rawsize()) || (sz > src.rawsize())) {
      throw dabc::Exception("cannot copy buffer outside current segment, use Buffer::copyfrom method");
   }

   // this is special and simple case, treat is separately
   ::memcpy(ptr(), src.ptr(), sz);
   return sz;
}

dabc::BufferSize_t dabc::Pointer::copyto(void* tgt, BufferSize_t sz) throw()
{
   if (sz > fullsize()) {
      EOUT(("Source pointer has no such much memory %lu", sz));
      sz = fullsize();
   }

   if (sz==0) return sz;

   if (sz > rawsize())
      throw dabc::Exception("Cannot copy memory outside current segment, use Buffer::copyto() method");

   ::memcpy(tgt, ptr(), sz);
    return sz;
}

dabc::BufferSize_t dabc::Pointer::copyfrom(void* src, BufferSize_t sz) throw()
{
   if (sz > fullsize()) {
      EOUT(("target pointer has no so much memory %lu", sz));
      sz = fullsize();
   }

   if (sz==0) return sz;

   // this is special and simple case, treat is separately
   if (sz > rawsize())
      throw dabc::Exception("Cannot copy memory outside current segment, use Buffer::copyfrom() method");

   ::memcpy(ptr(), src, sz);
   return sz;
}

