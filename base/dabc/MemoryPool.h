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
#ifndef DABC_MemoryPool
#define DABC_MemoryPool

#ifndef DABC_Folder
#include "dabc/Folder.h"
#endif

#ifndef DABC_WorkingProcessor
#include "dabc/WorkingProcessor.h"
#endif

#ifndef DABC_collections
#include "dabc/collections.h"
#endif

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

#ifndef DABC_Command
#include "dabc/Command.h"
#endif

#include <stdlib.h>

namespace dabc {

   class Buffer;
   class Mutex;
   class MemoryPool;

   // this is alignment for the buffers and memory blocks, created by pool
   // It must be a power of two and should be like 16 or 32 or 256
   extern unsigned DefaultMemoryAllign;

   // Default number of preallocated segments in reference array
   extern unsigned DefaultNumSegments;

   extern unsigned DefaultBufferSize;


   class MemoryPoolRequester {
      friend class MemoryPool;

      private:
         // this members will be accessible only by MemoryPool

         MemoryPool*   pool;
         BufferSize_t  bufsize;
         BufferSize_t  hdrsize;
         Buffer*       buf;
      public:
      // method must be reimplemented in user code to accept or
      // not buffer which was requested before
      // User returns true if he accept buffer, false means that buffer must be released

         MemoryPoolRequester() : pool(0), bufsize(0), hdrsize(0), buf(0) {}
         virtual bool ProcessPoolRequest() = 0;
         virtual ~MemoryPoolRequester() { if (buf!=0) EOUT(("buf !=0 in requester")); }
   };

   // _________________________________________________________

   // This is block of buffers, which can be created/destroyed by memory
   // pool on the "fly". It keeps allocated region and usage information


   struct MemoryBlock {
      typedef uint32_t BufferUsage_t;

      void          *fBlock;         // buffer itself
      uint64_t       fBlockSize;     // total size of block

      BufferSize_t   fBufferSize;    // size of individual buffer
      BufferNum_t    fNumBuffers;    // number of buffers in block
      BufferNum_t    fNumIncrease;   // number of buffers to be increased
      unsigned       fAlignment;     // alignment as it was specified in Allocate

      BufferId_t     fBlockId;       // id of block itself (only block number in higher bytes)
      MemoryBlock   *fNextBlock;     // pointer on the next block of the same kind

      BufferUsage_t *fUsage;         // usage counters for individual buffers
      BufferNum_t    fNumFree;       // total number of still free buffers
      BufferNum_t    fSeekPos;       // last position where free buffer was found (used when no free queue is exists)
      BufferNum_t   *fFreeQueue;     // circle queue of free buffers
      BufferNum_t    fFreeFirst;     // first empty item in fFreeQueue
      BufferNum_t    fFreeLast;      // last used item in fFreeQueue

      unsigned       fChangeCounter; // value of change counter when block was created

      MemoryBlock();
      ~MemoryBlock();

      bool IsNull() const { return fBlock==0; }

      bool Allocate(BufferSize_t buffersize, BufferNum_t numbuf,  unsigned align = 0, bool usagequeue = true);
      bool Release();

      bool IsBlockFree() const { return fNumFree == fNumBuffers; }
      unsigned NumUsedBuffers() const { return fNumBuffers - fNumFree; }
      unsigned SumUsageCounters() const;

      inline void* Block() const { return fBlock; }
      inline uint64_t BlockSize() const { return fBlockSize; }

      BufferSize_t   BufferSize() const { return fBufferSize; }
      BufferNum_t    NumBuffers() const { return fNumBuffers; }
      BufferNum_t    NumIncrease() const { return fNumIncrease; }
      unsigned       Alignment() const { return fAlignment; }
      bool           UseFreeQueue() const { return fFreeQueue!=0; }

      inline void* RawBuffer(BufferNum_t id) { return (char*) fBlock + ((uint64_t) id) * fBufferSize; }

      bool TakeBuffer(BufferNum_t& id);
      void IncReference(BufferNum_t id)
      {
         if (fUsage[id]==0) {
            EOUT(("Internal error"));
            exit(101);
         }
         fUsage[id]++;
      }
      void DecReference(BufferNum_t id);
      void CleanAllReferences();

      uint64_t AllocatedMemorySize();
      uint64_t MemoryPerBuffer();
   };

   // __________________________________________________________________

   struct ReferencesBlock : public MemoryBlock {

      Buffer      *fArr; // array of references
      MemoryPool  *fPool; // back pointer on the pool
      BufferSize_t fHeaderSize; // allocated header size
      unsigned     fNumSegments; // allocated number of segments per buffer

      ReferencesBlock();
      ~ReferencesBlock();

      BufferSize_t HeaderSize() const { return fHeaderSize; }
      unsigned     NumSegments() const { return fNumSegments; }

      bool Allocate(MemoryPool* pool, BufferSize_t headersize, BufferNum_t numrefs, unsigned numsegments);
      bool Release();

      Buffer* TakeReference(unsigned nseg, BufferSize_t hdrsize, bool force);
      bool ReleaseReference(Buffer* buf);

      uint64_t AllocatedMemorySize();
      uint64_t MemoryPerBuffer();
   };


   // _________________________________________________________

   class MemoryPool : public Folder,
                      public WorkingProcessor {

      enum  { evntProcessRequests = evntFirstSystem };

      enum ECleanupStatus { stOff, stMemCleanup, stRefCleanup };

      friend class Buffer;

      public:
         MemoryPool(Basic* parent, const char* name);
         virtual ~MemoryPool();

         static BufferSize_t minBufferSize() { return 0x100; }
         static BufferSize_t maxBufferSize() { return 0x10000000; }

         virtual const char* ClassName() const { return clMemoryPool; }

         void UseMutex(Mutex* mutex = 0);
         inline Mutex* GetPoolMutex() const { return fPoolMutex; }

         /** Sets maximum allowed memory to be used by the pool.
           * Zero means that pool is not allowed at all to dynamically extends memory blocks. */
         void SetMemoryLimit(uint64_t limit = 0);

         /** Sets timeout in second after which memory pool can shrink its size back */
         void SetCleanupTimeout(double tmout = 5.);

         void SetLayoutFixed();

         /** Allocate memory in one block. Either creates primary block or continues
           * list of existing ones with probably other buffers numbers */
         bool AllocateMemory(BufferSize_t buffersize,
                             BufferNum_t numbuffers,
                             BufferNum_t increase = 0,
                             unsigned align = 0x10,
                             bool withqueue = true);

         /** Allocate references with preallocated header and segments lists */
         bool AllocateReferences(BufferSize_t headersize,
                                 BufferNum_t numrefs,
                                 BufferNum_t increase = 0,
                                 unsigned numsegm = 4);

         /** These methods used in module constructor to configure pool sizes
          * instead of creating pool immediately. Appropriate memory and reference block
          * will be created when first request of memory buffer will be done
          */
         bool AddMemReq(BufferSize_t bufsize, BufferNum_t number, BufferNum_t increment, unsigned align);
         bool AddRefReq(BufferSize_t hdrsize, BufferNum_t number, BufferNum_t increment, unsigned numsegm);

         // this is information about internal structure
         // of memory pool and how this structure changes
         bool IsEmpty() const { return (fNumMem==0) && (fNumRef==0); }

         BlockNum_t NumMemBlocks() const { return fNumMem; }
         bool IsMemoryBlock(BlockNum_t n) { return n<fNumMem && fMem[n]!=0; }
         unsigned MemBlockChangeCounter(BlockNum_t n) { return fMem[n]->fChangeCounter; }
         void* GetMemBlock(BlockNum_t n) const { return fMem[n]->Block(); }
         uint64_t GetMemBlockSize(BlockNum_t n) const { return fMem[n]->BlockSize(); }
         unsigned GetChangeCounter() const { return fChangeCounter; }
         bool IsMemLayoutFixed() const { return fMemLayoutFixed; }

         bool CheckChangeCounter(unsigned &cnt);

         BufferNum_t GetNumBuffersInBlock(BlockNum_t block = 0) const { return fMem[block]->NumBuffers(); }
         inline BufferSize_t GetBufferSize(BufferId_t id) const { return fMem[GetBlockNum(id)]->BufferSize(); }
         inline void* GetBufferLocation(BufferId_t id) const
           { return fMem[GetBlockNum(id)]->RawBuffer(GetBufferNum(id)); }

         uint64_t GetTotalSize() const;
         uint64_t GetUsedSize() const;

         bool CanHasBufferSize(BufferSize_t size);
         bool CanHasHeaderSize(BufferSize_t size);

         // this is information about each buffer

         /** method locked buffer and return reference on it */
         bool TakeRawBuffer(BufferId_t &id);

         /** release buffer, which was taken before */
         void ReleaseRawBuffer(BufferId_t id);

         /** Mark all raw buffers as unused
           * Only should be used if no real references exists on raw buffers */
         void ReleaseAllRawBuffers();

         // returns Buffer object with writable access to consequent data of specified size
         // size defines requested buffer area, if = 0 return exactly one buffer
         // size can be longer as single buffer - pool will try to find several of them one after another
         // size CANNOT be longer as size of one memory block
         Buffer* TakeBuffer(BufferSize_t size = 0, BufferSize_t hdrsize = 0);

         // return buffer without any data segments
         Buffer* TakeEmptyBuffer(BufferSize_t hdrsize = 0);

         // return buffer or set request to the pool, which will be processed
         // when new memory is available in the buffer
         Buffer* TakeBufferReq(MemoryPoolRequester* req, BufferSize_t size = 0, BufferSize_t hdrsize = 0);

         Buffer* TakeRequestedBuffer(MemoryPoolRequester* req);

         // Cancel previously submitted request
         // May happen, if object want to be deleted
         void RemoveRequester(MemoryPoolRequester* req);

         void Print();

         void StoreConfig();
         bool Reconstruct(dabc::Command* cmd = 0);

         virtual bool Store(ConfigIO &cfg);
         virtual bool Find(ConfigIO &cfg);

         static dabc::BufferSize_t RoundBufferSize(dabc::BufferSize_t bufsize);
         static std::string BlockName(dabc::BufferSize_t bufsize);

      protected:

         bool ReleaseMemory();
         bool _ExtendArr(void* arr, BlockNum_t &capacity);

         MemoryBlock* _AllocateMemBlock(BufferSize_t buffersize, BufferNum_t numbuffers, unsigned align, bool withqueue);
         ReferencesBlock* _AllocateRefBlock(BufferSize_t headersize, BufferNum_t numrefs, unsigned numsegm);

         bool _ReleaseLastMemBlock();
         bool _ReleaseLastRefBlock();

         virtual void ProcessEvent(EventId);
         virtual double ProcessTimeout(double);

         bool NewReferences(MemSegment* segs, unsigned numsegs);
         void ReleaseBuffer(Buffer* buf);

         Buffer* _TakeBuffer(BufferSize_t size, BufferSize_t hdrsize);
         Buffer* _TakeEmptyBuffer(unsigned nseg, BufferSize_t hdrsize);
         void _ReleaseBuffer(Buffer* buf);

         bool _ProcessRequests();
         void ReplyReadyRequests();

         bool _CheckCleanupStatus();

         Mutex            *fPoolMutex;
         bool              fOwnMutex;

         uint64_t          fMemoryLimit; // maximal size of memory, allowed to allocated by memory pool
         uint64_t          fMemoryAllocated;  // currently allocated memory by the pool (beside few small arrays like fMem or fRef)

         MemoryBlock*     *fMem;           // array of all memory blocks
         BlockNum_t        fNumMem;        // number of all memory blocks
         BlockNum_t        fMemCapacity;   // capacity of fMem array
         BlockNum_t        fMemPrimary;    // number of primary blocks, upper limit for search of different buffer sizes
         BlockNum_t        fMemSecondary;  // number of secondary blocks, upper limit for cleanup
         bool              fMemCleanupStatus; // true if last block was free during cleanup loop
         bool              fMemLayoutFixed; // when set to true, blocks any possibility to change layout of the pool
         ReferencesBlock* *fRef;
         BlockNum_t        fNumRef;
         BlockNum_t        fRefCapacity;
         BlockNum_t        fRefPrimary;       // number of primary references blocks, upper limit for cleanup
         bool              fRefCleanupStatus; // true if last block was free during cleanup loop

         unsigned          fChangeCounter;

         Queue<MemoryPoolRequester*>  fReqQueue;
         bool              fEvntFired; // indicate if we already fire event for requests processing
         ECleanupStatus    fCleanupStatus;
         double            fCleanupTmout; // length of cleanup loop
   };


   class CmdCreateMemoryPool : public Command {
      public:
         static const char* CmdName() { return "CreateMemoryPool"; }

         CmdCreateMemoryPool(const char* poolname) : Command(CmdName())
         {
            SetPar(xmlPoolName, poolname);
         }

         static bool AddCfg(Command* cmd,
                            bool fixedlayout = false,
                            unsigned size_limit_mb = 0,
                            double cleanuptmout = -1.)
         {
            if ((cmd==0) || !cmd->IsName(CmdName())) return false;

            if (fixedlayout) cmd->SetBool(xmlFixedLayout, fixedlayout);
            if (size_limit_mb>0) cmd->SetUInt(xmlSizeLimitMb, size_limit_mb);
            if (cleanuptmout>=0.) cmd->SetDouble(xmlCleanupTimeout, cleanuptmout);
         }

         static bool AddMem(Command* cmd,
                           unsigned buffersize,
                           unsigned numbuffers,
                           unsigned increment = 0,
                           unsigned align = 8)
         {
            buffersize = dabc::MemoryPool::RoundBufferSize(buffersize);

            if ((cmd==0) || !cmd->IsName(CmdName())) return false;

            if ((buffersize==0) || (numbuffers==0)) return false;

            std::string blockname = dabc::MemoryPool::BlockName(buffersize);
            cmd->SetUInt((blockname+xmlNumBuffers).c_str(), numbuffers);
            if (increment>0) cmd->SetUInt((blockname+xmlNumIncrement).c_str(), increment);
            if (align!=8) cmd->SetUInt((blockname+xmlAlignment).c_str(), align);

            return true;
         }

         static bool AddRef(Command* cmd,
                            unsigned numref,
                            unsigned headersize = 0,
                            unsigned increment = 0,
                            unsigned numsegm = 0)
         {
            if ((cmd==0) || !cmd->IsName(CmdName())) return false;
            if (numref==0) return false;

            std::string blockname = dabc::MemoryPool::BlockName(0);
            cmd->SetUInt((blockname+xmlNumBuffers).c_str(), numref);
            if (headersize>0) cmd->SetUInt((blockname+xmlHeaderSize).c_str(), headersize);
            if (increment>0) cmd->SetUInt((blockname+xmlNumIncrement).c_str(), increment);
            if (numsegm>0) cmd->SetUInt((blockname+xmlNumSegments).c_str(), numsegm);

            return true;
         }

   };

}

#endif
