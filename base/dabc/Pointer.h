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

#ifndef DABC_Pointer
#define DABC_Pointer

#include <stdint.h>

namespace dabc {

   typedef uint32_t BufferSize_t;


   class Buffer;

   class Pointer {

      friend class Buffer;

      protected:
         unsigned          fSegm;  // segment id
         unsigned char*    fPtr;   // pointer on the raw buffer
         BufferSize_t      fRawSize;  // size of contiguous memory, pointed by fPtr
         BufferSize_t      fFullSize; // full size of memory from pointer till the end

         void long_shift(BufferSize_t sz) throw();

      public:
         inline Pointer() :
            fSegm(0),
            fPtr(0),
            fRawSize(0),
            fFullSize(0)
         {
         }

         inline Pointer(const void* buf, BufferSize_t sz) :
            fSegm(0),
            fPtr((unsigned char*) buf),
            fRawSize(sz),
            fFullSize(sz)
         {
         }

         inline Pointer(const Pointer& src) :
            fSegm(src.fSegm),
            fPtr(src.fPtr),
            fRawSize(src.fRawSize),
            fFullSize(src.fFullSize)
         {
         }

         inline Pointer& operator=(const Pointer& src)
         {
            reset(src);
            return *this;
         }

         inline void reset(void* buf, BufferSize_t sz)
         {
            fSegm = 0;
            fPtr = (unsigned char*) buf;
            fRawSize = sz;
            fFullSize = sz;
         }

         inline void reset(const Pointer& src, BufferSize_t fullsz = 0)
         {
            fSegm = src.fSegm;
            fPtr = src.fPtr;
            fRawSize = src.fRawSize;
            fFullSize = src.fFullSize;
            if (fullsz>0) setfullsize(fullsz);
         }

         inline void reset()
         {
            fSegm = 0;
            fPtr = 0;
            fRawSize = 0;
            fFullSize = 0;
         }

         inline void* operator()() const { return fPtr; }
         inline void* ptr() const { return fPtr; }
         inline bool null() const { return (fPtr==0) || (fullsize()==0); }
         inline BufferSize_t rawsize() const { return fRawSize; }
         inline BufferSize_t fullsize() const { return fFullSize; }
         inline void shift(BufferSize_t sz) throw()
         {
            if (sz<fRawSize) {
               fPtr += sz;
               fRawSize -= sz;
               fFullSize -= sz;
            } else
            if (sz>=fFullSize)
               reset();
            else
               long_shift(sz);
         }

         void setfullsize(BufferSize_t sz)
         {
            if (fFullSize>=sz) {
               fFullSize = sz;
               if (fRawSize<sz) fRawSize = sz;
            }
         }

         inline void operator+=(BufferSize_t sz) throw() { shift(sz); }

         BufferSize_t copyfrom(const Pointer& src, BufferSize_t sz) throw();

         BufferSize_t copyto(void* tgt, BufferSize_t sz) throw();

         BufferSize_t copyfrom(void* src, BufferSize_t sz) throw();

         BufferSize_t distance_to(const Pointer& child) const;

         unsigned segmid() const { return fSegm; }
   };

}


#endif
