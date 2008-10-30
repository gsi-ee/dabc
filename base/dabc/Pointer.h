#ifndef DABC_Pointer
#define DABC_Pointer

#ifndef DABC_Buffuer
#include "dabc/Buffer.h"
#endif

namespace dabc {
    
   class Pointer {
        
      protected:
         const Buffer*  fBuf;   // pointer on the buffer
         unsigned       fSegm;  // segment id
         unsigned char* fPtr;   // pointer on the raw buffer
         BufferSize_t   fRawSize;  // size of contigious memory, pointed by fPtr
         BufferSize_t   fFullSize; // full size of memory from pointer till the end 
       
         void long_shift(BufferSize_t sz);
       
      public:
         inline Pointer() :
            fBuf(0),
            fSegm(0),
            fPtr(0),
            fRawSize(0),
            fFullSize(0)
         {
         }
      
         inline Pointer(const Buffer* buf) :
            fBuf(buf),
            fSegm(0),
            fPtr(0),
            fRawSize(0),
            fFullSize(0)
         {
            fPtr = (unsigned char*) fBuf->GetDataLocation(0);
            fRawSize = fBuf->GetDataSize(0);
            fFullSize = fBuf->GetTotalSize();
         }
         
         inline Pointer(void* buf, BufferSize_t sz) :
            fBuf(0),
            fSegm(0),
            fPtr((unsigned char*) buf),
            fRawSize(sz),
            fFullSize(sz)
         {
         }   
         
         inline Pointer(const Pointer& src) :
            fBuf(src.fBuf),
            fSegm(src.fSegm),
            fPtr(src.fPtr),
            fRawSize(src.fRawSize),
            fFullSize(src.fFullSize)
         {
         }
         
         inline void reset(void* buf, BufferSize_t sz) 
         {
            fBuf = 0;
            fSegm = 0;
            fPtr = (unsigned char*) buf;
            fRawSize = sz;
            fFullSize = sz;
         }

         inline void reset(const Buffer* buf) 
         {
            fBuf = buf;
            fSegm = 0;
            if (fBuf) {
               fPtr = (unsigned char*) fBuf->GetDataLocation(0);
               fRawSize = fBuf->GetDataSize(0);
               fFullSize = fBuf->GetTotalSize();
            } else {
               fPtr = 0; 
               fRawSize = 0;
               fFullSize = 0;
            }
         }
         
         inline void reset(const Pointer& src, BufferSize_t fullsz = 0)
         {
            fBuf = src.fBuf;
            fSegm = src.fSegm;
            fPtr = src.fPtr;
            fRawSize = src.fRawSize;
            fFullSize = src.fFullSize;
            if (fullsz>0) setfullsize(fullsz);
         }

         inline void reset() { reset(0, 0); }
         
         inline const Buffer* buf() const { return fBuf; }
         
         inline void* operator()() const { return fPtr; }
         inline void* ptr() const { return fPtr; }
         inline bool null() const { return ptr() == 0; }
         inline BufferSize_t rawsize() const { return fRawSize; }
         inline BufferSize_t fullsize() const { return fFullSize; }
         inline unsigned segm() const { return fSegm; }
         inline void shift(BufferSize_t sz)
         {
            if (sz<fRawSize) {
               fPtr += sz;
               fRawSize -= sz;   
               fFullSize -= sz; 
            } else
               long_shift(sz);
         }
         
         void setfullsize(BufferSize_t sz);
         
         // shift pointer to the beginning of next segment
         inline void shift_to_segment() { shift(rawsize()); }
         
         inline void operator+=(BufferSize_t sz) { shift(sz); }
         
         inline void operator=(const Pointer& src) { reset(src); }
         
         BufferSize_t copyfrom(const Pointer& src, BufferSize_t sz);
         
         BufferSize_t copyto(void* tgt, BufferSize_t sz);

         BufferSize_t copyfrom(void* src, BufferSize_t sz);
         
         BufferSize_t distance_to(const Pointer& child) const;
   };
    
}



#endif
