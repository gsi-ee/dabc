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

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

namespace dabc {

   /** \brief Manipulator with \ref dabc::Buffer class
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    * \ref dabc::Buffer can contain segmented lists of memory regions.
    * \ref dabc::Pointer class provides method which let work with such segmented memory as with contiguous region.
    */

   class BufferNew;

   class Pointer {

      friend class Buffer;
      friend class BufferNew;

      protected:
         Buffer            fBuf;      ///< we keep reference on the buffer, could be only used in same thread
         unsigned          fSegm;     ///< segment id
         unsigned char*    fPtr;      ///< pointer on the raw buffer
         BufferSize_t      fRawSize;  ///< size of contiguous memory, pointed by fPtr
         BufferSize_t      fFullSize; ///< full size of memory from pointer till the end

         void long_shift(BufferSize_t sz) throw();

      public:
         inline Pointer() :
            fBuf(),
            fSegm(0),
            fPtr(0),
            fRawSize(0),
            fFullSize(0)
         {
         }

         inline Pointer(const void* buf, BufferSize_t sz) :
            fBuf(),
            fSegm(0),
            fPtr((unsigned char*) buf),
            fRawSize(sz),
            fFullSize(sz)
         {
         }

         inline Pointer(const Pointer& src, BufferSize_t pos = 0, BufferSize_t fullsz = 0) :
            fBuf(src.fBuf),
            fSegm(src.fSegm),
            fPtr(src.fPtr),
            fRawSize(src.fRawSize),
            fFullSize(src.fFullSize)
         {
            if (pos>0) shift(pos);
            if (fullsz>0) setfullsize(fullsz);
         }

         inline Pointer(const Buffer& buf, unsigned pos = 0, unsigned len = 0) :
            fBuf(),
            fSegm(0),
            fPtr(0),
            fRawSize(0),
            fFullSize(0)
         {
            reset(buf, pos, len);
         }

         inline Pointer& operator=(const Pointer& src)
         {
            reset(src);
            return *this;
         }

         inline Pointer& operator=(const Buffer& src)
         {
            reset(src);
            return *this;
         }

         inline void reset(void* buf, BufferSize_t sz)
         {
            fBuf.Release();
            fSegm = 0;
            fPtr = (unsigned char*) buf;
            fRawSize = sz;
            fFullSize = sz;
         }

         inline void reset(const Pointer& src, BufferSize_t pos = 0, BufferSize_t fullsz = 0)
         {
            fBuf = src.fBuf;
            fSegm = src.fSegm;
            fPtr = src.fPtr;
            fRawSize = src.fRawSize;
            fFullSize = src.fFullSize;
            if (pos>0) shift(pos);
            if (fullsz>0) setfullsize(fullsz);
         }

         inline void reset(const Buffer& src, BufferSize_t pos = 0, BufferSize_t fullsz = 0)
         {
            fSegm = 0;
            fBuf = src;
            fFullSize = fBuf.GetTotalSize();

            if (fFullSize>0) {
               fPtr = (unsigned char*) fBuf.SegmentPtr(0);
               fRawSize = fBuf.SegmentSize(0);
            } else {
               fPtr = 0;
               fRawSize = 0;
            }
            if (pos>0) shift(pos);
            if (fullsz>0) setfullsize(fullsz);
         }


         inline void reset()
         {
            fBuf.Release();
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

         inline BufferSize_t shift(BufferSize_t sz) throw()
         {
            if (sz<fRawSize) {
               fPtr += sz;
               fRawSize -= sz;
               fFullSize -= sz;
            } else
            if (sz>fFullSize) {
               sz = fFullSize;
               reset();
            } else
               long_shift(sz);
            return sz;
         }

         void setfullsize(BufferSize_t sz)
         {
            if (fFullSize>sz) {
               fFullSize = sz;
               if (fRawSize>sz) fRawSize = sz;
            }
         }

         inline void operator+=(BufferSize_t sz) throw() { shift(sz); }

         BufferSize_t copyto(void* tgt, BufferSize_t sz) const throw();

         inline BufferSize_t copyto_shift(void* tgt, BufferSize_t sz) throw()
            { return shift(copyto(tgt,sz)); }

         BufferSize_t copyto(Pointer& src, BufferSize_t sz) const throw();

         inline BufferSize_t copyto_shift(Pointer& src, BufferSize_t sz) throw()
           { return shift(copyto(src, sz)); }

         /** Returns actual size copied */
         BufferSize_t copyfrom(const Pointer& src, BufferSize_t sz = 0) throw();

         /** Returns actual size copied and shifted */
         inline BufferSize_t copyfrom_shift(const Pointer& src, BufferSize_t sz = 0) throw()
            { return shift(copyfrom(src, sz)); }

         BufferSize_t copyfrom(const void* src, BufferSize_t sz) throw();

         inline BufferSize_t copyfrom_shift(const void* src, BufferSize_t sz) throw()
            { return shift(copyfrom(src, sz)); }

         BufferSize_t copyfromstr(const char* str, unsigned len = 0) throw();

         int distance_to(const Pointer& child) const throw();

         int distance_to_ownbuf() const { return fBuf.null() ? 0 : Pointer(fBuf).distance_to(*this); }

         bool is_same_buf(const Pointer& ptr) const { return fBuf == ptr.fBuf; }

         unsigned segmid() const { return fSegm; }
   };

}


#endif
