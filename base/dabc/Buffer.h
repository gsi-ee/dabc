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
#ifndef DABC_Buffer
#define DABC_Buffer

#include <stdint.h>

namespace dabc {

   // buffer id includes memory block id (high 16-bit)  and number inside block (low 16 bit)
   // buffer size if limited by 32 bit - 4 gbyte is pretty enough for single structure

   typedef uint32_t BufferId_t;
   typedef uint32_t BufferSize_t;
   typedef uint32_t BlockNum_t; // we use 32 bit only for speed reason, actually only 16 bit are used
   typedef uint32_t BufferNum_t; // we use 32 bit only for speed reason, actually only 16 bit are used

   static inline BlockNum_t  GetBlockNum(BufferId_t id) { return id >> 16; }
   static inline BufferNum_t GetBufferNum(BufferId_t id) { return id & 0xffff; }
   static inline BufferId_t  CodeBufferId(BlockNum_t blocknum, BufferNum_t bufnum) { return (blocknum << 16) | bufnum; }

   extern const BufferSize_t BufferSizeError;

   enum EBufferTypes {
      mbt_Generic      = 0,
      mbt_Int64        = 1,
      mbt_TymeSync     = 2,
      mbt_AcknCounter  = 3,
      mbt_RawData      = 4,
      mbt_EOF          = 5,   // such packet produced when transport is closed completely
      mbt_EOL          = 6,   // this is more line end-of-line case, where transport is not closed, but may deliver new kind of data (new event ids) afterwards
      mbt_User         = 1000
   };

   class MemoryPool;
   class Mutex;
   struct ReferencesBlock;

   struct MemSegment {
       BufferId_t      id;        // id of the buffer
       BufferSize_t    datasize;  // length of data
       void*           buffer;    // pointer on the begining of buffer (must be in the area of id)
   };

   class Buffer {

      friend class MemoryPool;
      friend struct ReferencesBlock;

      protected:
         Buffer();
         virtual ~Buffer();

         void ReInit(MemoryPool* pool, BufferId_t refid,
                     void* header, unsigned headersize,
                     MemSegment* segm, unsigned numsegments);
         void ReClose();

         bool RellocateSegments(unsigned newcapacity);
         bool RellocateHeader(BufferSize_t newcapacity, bool copyoldcontent);
         bool AllocateInternBuffer(BufferSize_t sz);

         MemoryPool*   fPool;
         uint32_t      fTypeId;
         MemSegment*   fSegments;
         unsigned      fCapacity;
         unsigned      fNumSegments;
         void*         fHeader;
         BufferSize_t  fHeaderSize;
         BufferSize_t  fHeaderCapacity;
         void*         fInternBuffer; // for the case, when buffer created independent from memory pool
         BufferSize_t  fInternBufferSize; //

         BufferId_t    fReferenceId; // id of reference for memory pool
         void*         fOwnHeader; // this is self-allocated header if size of pool-provided header not enougth
         MemSegment*   fOwnSegments; // this is self-allocated segments if size of pool-provided array not enougth

      public:

         inline MemoryPool* Pool() const { return fPool; }

         void SetTypeId(uint32_t tid) { fTypeId = tid; }
         inline uint32_t GetTypeId() const { return fTypeId; }

         inline unsigned NumSegments() const { return fNumSegments; }
         inline const MemSegment& Segment(unsigned n = 0) const { return fSegments[n]; }

         inline BufferId_t GetId(unsigned n = 0) const { return fSegments[n].id; }
         inline void* GetDataLocation(unsigned n = 0) const { return fSegments[n].buffer; }
         inline BufferSize_t GetDataSize(unsigned n = 0) const { return fSegments[n].datasize; }

         BufferSize_t GetTotalSize() const;

         inline bool SetDataSize(BufferSize_t newsize, unsigned n = 0)
         {
            if ((newsize==0) || (fSegments[n].datasize<newsize)) return false;
            fSegments[n].datasize = newsize;
            return true;
         }

         bool AddBuffer(Buffer* buf, bool adopt = false);

         bool AddSegment(Buffer* buf, unsigned nseg = 0, BufferSize_t offset = 0, BufferSize_t newsize = 0);

         bool CopyFrom(Buffer* src);

         Buffer* MakeReference();

         inline void* GetHeader() const { return fHeaderSize > 0 ? fHeader : 0; }
         inline BufferSize_t GetHeaderSize() const { return fHeaderSize; }
         void SetHeaderSize(BufferSize_t sz);

         // this is the only official method to release buffer
         // after this call pointer on buffer object is invalid and cannot be used further
         static void Release(Buffer* buf);

         // has same meaning as previous, but value of variable is protected by mutex
         static void Release(Buffer* &buf, Mutex* mutex);

         // these methods only for create/reallocation of standalone
         // (without pool) memory buffer
         static Buffer* CreateBuffer(BufferSize_t sz);
         bool RellocateBuffer(BufferSize_t sz);
   };


   // small object, which helps to release buffer in case of exceptions
   // works like lockguard
   class BufferGuard {
      protected:
         Buffer* fBuf;
      public:
         inline BufferGuard() : fBuf(0) {}
         inline BufferGuard(Buffer* buf) : fBuf(buf) {}
         inline BufferGuard(const BufferGuard& buf) : fBuf(const_cast<BufferGuard*>(&buf)->Take()) {}
         inline ~BufferGuard() { dabc::Buffer::Release(fBuf); }

         inline void operator=(Buffer* buf) { dabc::Buffer::Release(fBuf); fBuf = buf; }

         inline Buffer* operator() () const { return fBuf; }
         inline Buffer* Take() { Buffer* res = fBuf; fBuf = 0; return res; }
         inline void Release() { dabc::Buffer::Release(fBuf); fBuf = 0; }
   };
};

#endif
