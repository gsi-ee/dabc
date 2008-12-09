#include "dabc/MemoryPool.h"

#include <stdlib.h>
#include <list>

#include "dabc/logging.h"
#include "dabc/threads.h"
#include "dabc/Configuration.h"

unsigned int dabc::MemoryAllign = 16;


dabc::MemoryBlock::MemoryBlock() :
   fBlock(0),
   fBlockSize(0),
   fBufferSize(0),
   fNumBuffers(0),
   fNumIncrease(0),
   fAlignment(0),
   fBlockId(0),
   fNextBlock(0),
   fUsage(0),
   fNumFree(0),
   fSeekPos(0),
   fFreeQueue(0),
   fFreeFirst(0),
   fFreeLast(0),
   fChangeCounter(0)
{
}

dabc::MemoryBlock::~MemoryBlock()
{
   Release();
}

bool dabc::MemoryBlock::Allocate(BufferSize_t buffersize, BufferNum_t numbuffers, unsigned align, bool usagequeue)
{
   if (!Release()) return false;

   if (align==0) {
      unsigned align = sizeof(void*);
      while (align < dabc::MemoryAllign) align*=2;
      if (align!=dabc::MemoryAllign)
          EOUT(("Unsuitable align value %u, used %u", dabc::MemoryAllign, align));
   }

   if (buffersize % align != 0) buffersize = (buffersize/align + 1) * align;

   uint64_t totalsize = buffersize;
   totalsize *= numbuffers;

   void* buf = 0;

   DOUT3(("Allocate memory block = %x single = %x number= %u align = %x" , totalsize, buffersize, numbuffers, align));

   int res = posix_memalign(&buf, align, totalsize);

   if ((res!=0) || (buf==0)) {
      EOUT(("Cannot allocate data for new Memory Block"));
      return false;
   }

   BufferUsage_t* usage = new BufferUsage_t[numbuffers];
   if (usage==0) {
      EOUT(("Cannot allocate usage array"));
      free(buf);
      return false;
   }

   BufferNum_t* fq = 0;
   if (usagequeue) {
      fq = new BufferNum_t[numbuffers];
      if (fq == 0) {
         EOUT(("Cannot allocate usage queue"));
         delete[] usage;
         free(buf);
         return false;
      }
   }

   fBlock = buf;
   fBlockSize = totalsize;
   fBufferSize = buffersize;
   fNumBuffers = numbuffers;
   fAlignment = align;

   fUsage = usage;
   fNumFree = numbuffers;
   fSeekPos = 0;
   fFreeQueue = fq;
   fFreeFirst = 0;
   fFreeLast = 0;

   for (BufferNum_t id=0;id<fNumBuffers;id++) {
      fUsage[id] = 0;
      if (fFreeQueue) fFreeQueue[id] = id;
   }

   return true;
}

bool dabc::MemoryBlock::Release()
{
   if (fFreeQueue) delete [] fFreeQueue;
   if (fUsage) delete [] fUsage;
   if (fBlock) free(fBlock);

   fBlock = 0;
   fBlockSize = 0;
   fBufferSize = 0;
   fNumBuffers = 0;
   fUsage = 0;
   fNumFree = 0;
   fSeekPos = 0;
   fFreeQueue = 0;
   fFreeFirst = 0;
   fFreeLast = 0;

   return true;
}

bool dabc::MemoryBlock::TakeBuffer(BufferNum_t& id)
{
   if (fNumFree==0) return false;

   if (fFreeQueue) {
      id = fFreeQueue[fFreeFirst++];
      if (fFreeFirst==fNumBuffers) fFreeFirst = 0;
   } else {
      for (BufferNum_t cnt = 0; cnt<fNumBuffers; cnt++) {
         id = fSeekPos++;
         if (fSeekPos == fNumBuffers) fSeekPos = 0;
         if (fUsage[id] == 0) break;
      }
   }

   fNumFree--;
   fUsage[id] = 1;

   return true;
}

unsigned dabc::MemoryBlock::SumUsageCounters() const
{
   unsigned sum = 0;
   for (BufferNum_t id = 0; id < fNumBuffers; id++)
      sum += fUsage[id];
   return sum;
}

void dabc::MemoryBlock::DecReference(BufferNum_t id)
{
   fUsage[id]--;

   if (fUsage[id] == 0) {
      fNumFree++;
      if (fFreeQueue) {
         fFreeQueue[fFreeLast++] = id;
         if (fFreeLast==fNumBuffers) fFreeLast = 0;
      }
   }
}

void dabc::MemoryBlock::CleanAllReferences()
{
   for (BufferNum_t id=0; id<fNumBuffers; id++) {
      fUsage[id] = 0;
      if (fFreeQueue) fFreeQueue[id] = id;
   }

   fNumFree = fNumBuffers;
   fFreeFirst = 0;
   fFreeLast = 0;
   fSeekPos = 0;
}

uint64_t dabc::MemoryBlock::AllocatedMemorySize()
{
    uint64_t res = BlockSize(); // block itself

    res += fNumBuffers * sizeof(BufferUsage_t); // usage counters

    if (fFreeQueue) res += fNumBuffers * sizeof(BufferNum_t); // free queue

    return res;
}

uint64_t dabc::MemoryBlock::MemoryPerBuffer()
{
   uint64_t res = BufferSize();
   res += sizeof(BufferUsage_t);
   if (fFreeQueue) res += sizeof(BufferNum_t);
   return res;
}



// ______________________________________________________________________

dabc::ReferencesBlock::ReferencesBlock() :
   MemoryBlock(),
   fArr(0),
   fPool(0),
   fHeaderSize(0),
   fNumSegments(0)
{
}

dabc::ReferencesBlock::~ReferencesBlock()
{
   Release();
}


bool dabc::ReferencesBlock::Allocate(MemoryPool* pool, BufferSize_t headersize, BufferNum_t numrefs, unsigned numsegments)
{
   if (!Release()) return false;

   // align header that address on the segments is also on the right place
   if (headersize>0) {
      unsigned align = 4 * sizeof(void*);
      while (align < headersize) align*=2;
      headersize = align;
   }

   if (numsegments<1) numsegments = 1;

   unsigned datasize = headersize + numsegments*sizeof(MemSegment);

   if (!MemoryBlock::Allocate(datasize, numrefs, 0x10, true)) return false;

   fArr = new Buffer[numrefs];

   if (fArr==0) {
      MemoryBlock::Release();
      return false;
   }

   fPool = pool;
   fHeaderSize = headersize;
   // actual number of items due-to allign
   fNumSegments = (fBufferSize - fHeaderSize) / sizeof(MemSegment);

   return true;
}

bool dabc::ReferencesBlock::Release()
{
   if (fArr) delete [] fArr;
   fArr = 0;
   fPool = 0,
   fHeaderSize = 0;
   fNumSegments = 0;

   return MemoryBlock::Release();
}

dabc::Buffer* dabc::ReferencesBlock::TakeReference(unsigned nseg, BufferSize_t hdrsize, bool force)
{
   BufferNum_t id = 0;

   // if not forced, return 0 when preallocated memory is not enough
   if (!force && ((nseg > NumSegments()) || hdrsize>HeaderSize())) return 0;

   if (!TakeBuffer(id)) return 0;

   dabc::Buffer* ref = &(fArr[id]);

   void* buf = RawBuffer(id);

   void* header = fHeaderSize > 0 ? buf : 0;
   MemSegment* segm = (MemSegment*) ((char*) buf + fHeaderSize);

   ref->ReInit(fPool, fBlockId | id, header, fHeaderSize, segm, fNumSegments);

   if (nseg > fNumSegments)
      if (!ref->RellocateSegments(nseg)) {
         MemoryBlock::DecReference(id);
         return 0;
      }
   ref->fNumSegments = nseg;

   if (hdrsize > fHeaderSize)
      if (!ref->RellocateHeader(hdrsize, false)) {
         MemoryBlock::DecReference(id);
         return 0;
      }
   ref->fHeaderSize = hdrsize;

   return ref;
}

bool dabc::ReferencesBlock::ReleaseReference(Buffer* buf)
{
   unsigned refid = GetBufferNum(buf->fReferenceId);

   #ifdef DO_INDEX_CHECK
   if (refid>=NumBuffers()) {
      EOUT(("Error id number of the reference"));
      return false;
   }
   #endif

   buf->ReClose();

   MemoryBlock::DecReference(refid);

   return true;
}

uint64_t dabc::ReferencesBlock::AllocatedMemorySize()
{
   return MemoryBlock::AllocatedMemorySize() + NumBuffers() * sizeof(Buffer);
}

uint64_t dabc::ReferencesBlock::MemoryPerBuffer()
{
   return MemoryBlock::MemoryPerBuffer() + sizeof(Buffer);
}

// ____________________________________________________________________


dabc::MemoryPool::MemoryPool(Basic* parent, const char* name) :
   Folder(parent, name),
   WorkingProcessor(),
   fPoolMutex(0),
   fOwnMutex(false),
   fMemoryLimit(0),
   fMemoryAllocated(0),
   fMem(0),
   fNumMem(0),
   fMemCapacity(0),
   fMemPrimary(0),
   fMemSecondary(0),
   fMemCleanupStatus(0),
   fMemLayoutFixed(false),
   fRef(0),
   fNumRef(0),
   fRefCapacity(0),
   fRefPrimary(0),
   fRefCleanupStatus(0),
   fChangeCounter(0),
   fReqQueue(16, true), // one can extend requests on the fly
   fEvntFired(false),
   fCleanupStatus(stOff),
   fCleanupTmout(5.)
{
}


dabc::MemoryPool::~MemoryPool()
{
   DOUT4(("POOOOOOOOOL Deleting: %s %p", GetName(), this));

   unsigned n1(0), n2(0);
   for (BlockNum_t nblock = 0; nblock<fNumMem; nblock++) {
      n1 += fMem[nblock]->NumUsedBuffers();
      n2 += fMem[nblock]->SumUsageCounters();
      DOUT3(("Destroy block %u: Numfree %u NumBuf %u", nblock, fMem[nblock]->fNumFree, fMem[nblock]->fNumBuffers));
   }

   if (n1>0)
     EOUT(("!!!!!!!!!!!!!!!!!!!!!! Pool %s, %u buffers used %u times !!!!!!!!!!!!!!!!!!!!!!!!", GetName(), n1, n2));

   ReleaseMemory();

   DOUT4(("POOOOOOOOOL Deleting done: %s %p", GetName(), this));
}


void dabc::MemoryPool::UseMutex(Mutex* mutex)
{
   fPoolMutex = mutex;
   fOwnMutex = mutex==0;
   if (fOwnMutex) fPoolMutex = new dabc::Mutex;
}

void dabc::MemoryPool::SetMemoryLimit(uint64_t limit)
{
   LockGuard lock(fPoolMutex);
   fMemoryLimit = limit;
}

void dabc::MemoryPool::SetCleanupTimeout(double tmout)
{
   LockGuard lock(fPoolMutex);
   fCleanupTmout = tmout;

   if (fCleanupTmout <= 0.)
      fCleanupStatus = stOff; // disable any possible cleanup immidiately
}

void dabc::MemoryPool::SetLayoutFixed()
{
   LockGuard lock(fPoolMutex);
   fMemLayoutFixed = true;
}

bool dabc::MemoryPool::AllocateMemory(BufferSize_t buffersize,
                                      BufferNum_t numbuffers,
                                      BufferNum_t increase,
                                      unsigned align,
                                      bool withqueue)
{
   LockGuard lock(fPoolMutex);

   if (fMemLayoutFixed) return false;

   if (increase>0xffff) increase = 0xffff;

   if (numbuffers==0) {
      EOUT(("Cannot allocate memory with 0 buffers"));
      return false;
   }

   while (numbuffers > 0) {

      BufferNum_t numbufalloc = (numbuffers<0x10000) ? numbuffers : 0xffff;

      MemoryBlock* block = _AllocateMemBlock(buffersize, numbufalloc, align, withqueue);

      if (block==0) return false;

      MemoryBlock* master = 0;

      for (BlockNum_t id = 0; id<fMemPrimary; id++)
         if (fMem[id]->BufferSize() == buffersize) {
            master = fMem[id];
            break;
         }

      if (master!=0) {
         while (master->fNextBlock) master = master->fNextBlock;
         master->fNextBlock = block;

         if (increase < master->NumIncrease()) increase = master->NumIncrease();
      } else {
         if (fMemSecondary>fMemPrimary)
            EOUT(("Not a good idea to create primary block when secondary already existing"));
         fMemPrimary = fNumMem;
      }

      fMemSecondary = fMemPrimary;

      block->fNumIncrease = increase;

      numbuffers -= numbufalloc;
   }

   return true;
}

bool dabc::MemoryPool::AllocateReferences(BufferSize_t headersize,
                                          BufferNum_t numrefs,
                                          BufferNum_t increase,
                                          unsigned numsegm)
{
   LockGuard lock(fPoolMutex);

   if (fMemLayoutFixed) return false;

   while (numrefs>0) {

      BufferNum_t numrefsalloc = (numrefs < 0x10000) ? numrefs : 0xffff;

      ReferencesBlock* block = _AllocateRefBlock(headersize, numrefsalloc, numsegm);

      if (block==0) return false;

      block->fNumIncrease = increase;

      fRefPrimary = fNumRef;

      numrefs -= numrefsalloc;
   }

   return true;
}

bool dabc::MemoryPool::Allocate(Command* cmd)
{
   if (cmd==0) return false;

   BufferSize_t buffersize = RoundBufferSize(cmd->GetUInt(xmlBufferSize, 0));

   if (IsMemLayoutFixed()) {
      if (!CanHasBufferSize(buffersize)) {
         EOUT(("Memory pool structure is fixed and pool will not provide buffers of size %u", buffersize));
         return false;
      }

      BufferSize_t headersize = GetCfgInt((BlockName(0)+xmlHeaderSize).c_str(), 0, cmd);

      if (!CanHasHeaderSize(headersize)) {
         EOUT(("Memory pool structure is fixed and pool will not provide headers of size %u", headersize));
         return false;
      }

      return true;
   }

   std::string blockname = BlockName(buffersize);

   BufferNum_t numbuffers = GetCfgInt((blockname+xmlNumBuffers).c_str(), 0, cmd);

   BufferNum_t numincrement = GetCfgInt((blockname+xmlNumIncrement).c_str(), 0, cmd);

//   std::string sbuf;
//   cmd->SaveToString(sbuf);
//   DOUT0(("Pool:%s Create block %u X %u cmd:%s", GetName(), buffersize, numbuffers, sbuf.c_str()));

   if (buffersize>0) {

      if (numbuffers>0)
         AllocateMemory(buffersize, numbuffers,  numincrement);

      blockname = BlockName(0);
   }

   BufferSize_t headersize = GetCfgInt((blockname+xmlHeaderSize).c_str(), 0, cmd);

   BufferNum_t numsegments = GetCfgInt((blockname+xmlNumSegments).c_str(), 8, cmd);

   AllocateReferences(headersize, numbuffers, numincrement > 0 ? numincrement : numbuffers / 2, numsegments);

   return true;
}


bool dabc::MemoryPool::ReleaseMemory()
{
   {
      LockGuard lock(fPoolMutex);
      while (_ReleaseLastMemBlock());
      while (_ReleaseLastRefBlock());

      if (fMem) delete [] fMem;
      fMem = 0; fMemCapacity = 0;
      fNumMem = 0;

      if (fRef) delete [] fRef;
      fRef = 0; fRefCapacity = 0;
      fNumRef = 0;
   }

   if (fOwnMutex) delete fPoolMutex;
   fPoolMutex = 0;
   fOwnMutex = false;

   return true;
}

bool dabc::MemoryPool::_ExtendArr(void* arr, BlockNum_t &capacity)
{
   BlockNum_t newsz = capacity*2;
   if (newsz<16) newsz = 16;

   void** new_arr = new void* [newsz];
   if (new_arr==0) return false;

   void** old_arr = *((void***) arr);

   for (BlockNum_t n=0;n<newsz;n++)
     new_arr[n] = 0;

   if (old_arr && (capacity>0))
     for (BlockNum_t n=0;n<capacity;n++)
        new_arr[n] = old_arr[n];

   delete [] old_arr;
   *((void***) arr) = new_arr;
   capacity = newsz;

   return true;
}

dabc::MemoryBlock* dabc::MemoryPool::_AllocateMemBlock(BufferSize_t buffersize, BufferNum_t numbuffers, unsigned align, bool withqueue)
{
   DOUT5(("_AllocateMemBlock pool:%s bufsize:%u number:%u align:%u", GetName(), buffersize, numbuffers, align));

   if (fNumMem>=fMemCapacity)
      if (!_ExtendArr(&fMem, fMemCapacity)) return 0;

   MemoryBlock* block = new MemoryBlock;

   if (!block->Allocate(buffersize, numbuffers, align, withqueue)) {
      delete block;
      return 0;
   }

   block->fBlockId = CodeBufferId(fNumMem, 0);

   fMem[fNumMem] = block;
   fNumMem++;

   block->fChangeCounter = ++fChangeCounter;

   DOUT3(("_AllocateMemBlock new block of memory %u 0x%x", numbuffers, buffersize));

   return block;
}


dabc::ReferencesBlock* dabc::MemoryPool::_AllocateRefBlock(BufferSize_t headersize, BufferNum_t numrefs, unsigned numsegm)
{
   if (numrefs==0) return 0;

   if (fNumRef>=fRefCapacity)
      if (!_ExtendArr(&fRef, fRefCapacity)) return 0;

   ReferencesBlock* block = new ReferencesBlock();

   // one should set this id that Allocate initialise references appropriately
   block->fBlockId = CodeBufferId(fNumRef, 0);

   if (!block->Allocate(this, headersize, numrefs,  numsegm)) {
      delete block;
      return false;
   }

   fRef[fNumRef] = block;
   fNumRef++;

   DOUT3(("_AllocateMemBlock new block of references %u", numrefs));

   return block;
}

bool dabc::MemoryPool::_ReleaseLastMemBlock()
{
   if (fNumMem==0) return false;

   fNumMem--;

   DOUT3((" _ReleaseLastMemBlock() pool:%s num:%u used %d", GetName(), fNumMem, fMem[fNumMem]->NumUsedBuffers()));

   if (fMem[fNumMem]==0) return true;

   if (!fMem[fNumMem]->Release())
      EOUT(("Error by block release"));

   for (BlockNum_t id=0;id<fNumMem;id++)
      if (fMem[id]->fNextBlock==fMem[fNumMem])
         fMem[id]->fNextBlock = 0;

   delete fMem[fNumMem];

   fMem[fNumMem] = 0;

   fChangeCounter++;

   return true;
}

bool dabc::MemoryPool::_ReleaseLastRefBlock()
{
   if (fNumRef==0) return false;

   fNumRef--;

   DOUT3((" _ReleaseLastRefBlock used %d", fRef[fNumRef]->NumUsedBuffers()));

   if (fRef[fNumRef]==0) return true;

   if (!fRef[fNumRef]->Release())
      EOUT(("Error by block release"));

   delete fRef[fNumRef];

   fRef[fNumRef] = 0;

   return true;
}


uint64_t dabc::MemoryPool::GetTotalSize() const
{
   uint64_t sum = 0;
   for (BlockNum_t nblock=0;nblock<fNumMem;nblock++)
      sum += fMem[nblock]->BlockSize();
   return sum;
}

uint64_t dabc::MemoryPool::GetUsedSize() const
{
   uint64_t sum = 0;
   for (BlockNum_t nblock=0;nblock<fNumMem;nblock++)
      sum += fMem[nblock]->BufferSize() * fMem[nblock]->NumUsedBuffers();
   return sum;
}

bool dabc::MemoryPool::TakeRawBuffer(BufferId_t &id)
{
   // Raw buffer does not support any kind of extension

   LockGuard guard(fPoolMutex);

   BufferNum_t num;

   for (BlockNum_t nblock=0;nblock<fNumMem;nblock++)
      if (fMem[nblock]->TakeBuffer(num)) {
         id = fMem[nblock]->fBlockId | num;
         return true;
      }

   return false;
}

void dabc::MemoryPool::ReleaseRawBuffer(BufferId_t id)
{
   LockGuard guard(fPoolMutex);

   fMem[GetBlockNum(id)]->DecReference(GetBufferNum(id));
}

void dabc::MemoryPool::ReleaseAllRawBuffers()
{
   LockGuard guard(fPoolMutex);

   for (BlockNum_t nblock=0;nblock<fNumMem;nblock++)
      fMem[nblock]->CleanAllReferences();
}

bool dabc::MemoryPool::CanHasBufferSize(BufferSize_t size)
{
   for (BlockNum_t n=0;n<fNumMem;n++)
      if (fMem[n]->BufferSize()>=size) return true;

   return false;
}

bool dabc::MemoryPool::CanHasHeaderSize(BufferSize_t size)
{
   for (BlockNum_t n=0;n<fNumRef;n++)
      if (fRef[n]->HeaderSize()>=size) return true;
   return false;
}

bool dabc::MemoryPool::CheckChangeCounter(unsigned &cnt)
{
   LockGuard guard(fPoolMutex);

   bool res = cnt!=fChangeCounter;

   cnt = fChangeCounter;

   return res;
}

dabc::Buffer* dabc::MemoryPool::TakeBuffer(BufferSize_t size, BufferSize_t hdrsize)
{
   bool process_res = false;

   Buffer* buf = 0;

   {
      LockGuard guard(fPoolMutex);

      if (fReqQueue.Size()>0)
         process_res = _ProcessRequests();

      buf = _TakeBuffer(size, hdrsize);
   }

   if (process_res) ReplyReadyRequests();

   return buf;
}

dabc::Buffer* dabc::MemoryPool::TakeEmptyBuffer(BufferSize_t hdrsize)
{
   LockGuard guard(fPoolMutex);

   return _TakeEmptyBuffer(0, hdrsize);
}

dabc::Buffer* dabc::MemoryPool::TakeBufferReq(MemoryPoolRequester* req, BufferSize_t size, BufferSize_t hdrsize)
{

   bool process_res = false;

   Buffer* buf = 0;

   {
      LockGuard guard(fPoolMutex);

      if ((req->pool!=0) && (req->pool!=this)) {
         EOUT(("Hard error - requester used for other pool"));
         return 0;
      }

      if ((req->pool==this) && (req->buf==0)) {
         req->bufsize = size;
         req->hdrsize = hdrsize;
      }

      if (fReqQueue.Size()>0)
         process_res = _ProcessRequests();

      if (req->buf!=0) {

         buf = req->buf;

         if (req->pool!=0) fReqQueue.Remove(req);

         req->pool = 0;
         req->buf = 0;

         if (req->bufsize<size)
            EOUT(("Missmatch in requested (%u) and obtainbed (%u) buffer sizes", req->bufsize, size));

         if (req->hdrsize<hdrsize)
            EOUT(("Missmatch in requested (%u) and obtainbed (%u) header sizes", req->hdrsize, hdrsize));
      } else {
         buf = _TakeBuffer(size, hdrsize);

         if (buf==0) {
            req->bufsize = size;
            req->hdrsize = hdrsize;
            req->buf = 0;
            if (req->pool == 0) {
               req->pool = this;
               fReqQueue.Push(req);
            }
         } else {
            if (req->pool==this) {
               EOUT(("Internal error!!!"));
               req->pool = 0;
               fReqQueue.Remove(req);
            }
         }
      }
   }

   if (process_res) ReplyReadyRequests();

   return buf;
}

dabc::Buffer* dabc::MemoryPool::TakeRequestedBuffer(MemoryPoolRequester* req)
{
   LockGuard guard(fPoolMutex);

   if (req->buf==0) return 0;

   Buffer* buf = req->buf;
   req->buf = 0;
   if (req->pool==this) {
      req->pool = 0;
      fReqQueue.Remove(req);
   }

   return buf;
}


void dabc::MemoryPool::RemoveRequester(MemoryPoolRequester* req)
{
   LockGuard guard(fPoolMutex);

   if (req->pool == this) {
      fReqQueue.Remove(req);
      if (req->buf)
        _ReleaseBuffer(req->buf);
      req->buf = 0;
      req->pool = 0;
   }
}

bool dabc::MemoryPool::NewReferences(MemSegment* segs, unsigned numsegs)
{
   LockGuard guard(fPoolMutex);

   for (unsigned nseg=0;nseg<numsegs; nseg++) {
      MemoryBlock* block = fMem[GetBlockNum(segs[nseg].id)];

      block->IncReference(GetBufferNum(segs[nseg].id));
   }

   return true;
}

void dabc::MemoryPool::ReleaseBuffer(Buffer* buf)
{
   {
      LockGuard guard(fPoolMutex);

      _ReleaseBuffer(buf);

      if (_CheckCleanupStatus())
         ActivateTimeout(fCleanupTmout);

      if (fReqQueue.Size()==0) return;

      if (ProcessorThread()==0)
          if (!_ProcessRequests()) return;
       else {
         if (!fEvntFired) {
            fEvntFired = true;
            FireEvent(evntProcessRequests);
         }
         return;
       }
   }

   ReplyReadyRequests();
}

dabc::Buffer* dabc::MemoryPool::_TakeEmptyBuffer(unsigned nseg, BufferSize_t hdrsize)
{
   if (fNumRef==0) return 0;

   Buffer* ref = 0;

   for (BlockNum_t nblock=0;nblock<fNumRef;nblock++)
      if ((ref = fRef[nblock]->TakeReference(nseg, hdrsize, false)) != 0) break;

   if (ref==0)
      for (BlockNum_t nblock=0;nblock<fNumRef;nblock++)
         if ((ref = fRef[nblock]->TakeReference(nseg, hdrsize, true)) != 0) break;

   if ((ref==0) && (fNumRef>0) && !fMemLayoutFixed) {
      ReferencesBlock* master = fRef[fNumRef-1];

      // check memory limit
      if ((fMemoryLimit>0) &&
          (fMemoryAllocated + master->NumIncrease() * master->MemoryPerBuffer() > fMemoryLimit)) return 0;

      ReferencesBlock* block = _AllocateRefBlock(master->HeaderSize(), master->NumIncrease(), master->NumSegments());
      if (block!=0) {
         block->fNumIncrease = master->NumIncrease();
         ref = block->TakeReference(nseg, hdrsize, true);
      }
   }

   return ref;
}

dabc::Buffer* dabc::MemoryPool::_TakeBuffer(BufferSize_t size, BufferSize_t hdrsize)
{
   if (fMem==0) return _TakeEmptyBuffer(0, hdrsize);

   if (fNumMem==0) {
      EOUT(("Cannot take buffer from empty pool"));
      return 0;
   }

   MemoryBlock* master = 0;
   for (unsigned id = 0; id<fMemPrimary; id++) {
      MemoryBlock* block = fMem[id];
      if (size==0) size = block->BufferSize(); // if specified 0, take buffers of size of first block
      if (block->BufferSize() >= size)
         if ((master==0) || master->BufferSize() > block->BufferSize()) master = block;
   }

   if (master==0) {
      EOUT(("No master blocks with so big buffer %u", size));
      return 0;
   }

   if (size==0) size = master->BufferSize();

   BufferNum_t num = 0;
   bool res = master->TakeBuffer(num);

   while (!res && (master->fNextBlock!=0)) {
      master = master->fNextBlock;
      res = master->TakeBuffer(num);
   }

   // try if we can expand memory pool and get memory from there
   if (!res && !fMemLayoutFixed) {

      if (master->NumIncrease()==0) return 0;

      if ((fMemoryLimit>0) &&
         (fMemoryAllocated + master->NumIncrease() * master->MemoryPerBuffer() > fMemoryLimit)) return 0;

      MemoryBlock* block = _AllocateMemBlock(master->BufferSize(), master->NumIncrease(), master->Alignment(), master->UseFreeQueue());

      if (block==0) return 0;

      block->fNumIncrease = master->NumIncrease();
      master->fNextBlock = block;

      master = block;

      res = master->TakeBuffer(num);
   }

   if (!res) return 0;

   Buffer* ref = _TakeEmptyBuffer(1, hdrsize);

   if (ref==0) {
      master->DecReference(num);
      return 0;
   }

   ref->fSegments[0].id = master->fBlockId | num;
   ref->fSegments[0].buffer = master->RawBuffer(num);
   ref->fSegments[0].datasize = master->BufferSize();

   return ref;
}

void dabc::MemoryPool::_ReleaseBuffer(Buffer* buf)
{
   for (unsigned nseg=0;nseg<buf->fNumSegments;nseg++) {
      MemoryBlock* block = fMem[GetBlockNum(buf->fSegments[nseg].id)];
      block->DecReference(GetBufferNum(buf->fSegments[nseg].id));
   }

   ReferencesBlock* refblock = fRef[GetBlockNum(buf->fReferenceId)];
   refblock->ReleaseReference(buf);
}

void dabc::MemoryPool::ProcessEvent(EventId evid)
{
   DOUT4(("Pool %s process event %x locked %s", GetName(), evid, DBOOL(fPoolMutex->IsLocked())));

   bool process_res = false;

   {
      LockGuard guard(fPoolMutex);

      if (fReqQueue.Size()>0)
         process_res = _ProcessRequests();

      fEvntFired = false;
   }

   if (process_res) ReplyReadyRequests();

   DOUT4(("Pool %s process event %x done", GetName(), evid));
}


bool dabc::MemoryPool::_ProcessRequests()
{
   // method return true when at least one request is processed
   // in this case one should call ReplyReadyRequests without the locked mutex

   bool res = false;

   for(unsigned n=0; n<fReqQueue.Size(); n++) {
      MemoryPoolRequester* req = fReqQueue.Item(n);

      if (req->buf==0)
         req->buf = _TakeBuffer(req->bufsize, req->hdrsize);

      if (req->buf != 0) res = true;
   }

   return res;
}

void dabc::MemoryPool::ReplyReadyRequests()
{
   MemoryPoolRequester* req = 0;

   do {
      // give signal to requester that buffer is ready
      // principal here - make call outside locked mutex
      if (req!=0)
         if (req->ProcessPoolRequest()) req = 0;

      LockGuard guard(fPoolMutex);

      // cleanup buffer of requester, who do not like to process own request
      if (req!=0) {
         _ReleaseBuffer(req->buf);
         req->buf = 0;
      }

      req = 0;

      for(unsigned n=0; n<fReqQueue.Size(); n++)
         if (fReqQueue.Item(n)->buf != 0) {
            req = fReqQueue.Item(n);
            req->pool = 0;
            fReqQueue.RemoveItem(n);
            break;
         }
   } while (req!=0);
}

bool dabc::MemoryPool::_CheckCleanupStatus()
{
   // return true when new loop of the cleanup can be started
   // in our case it means that we should activate timeout

   if (fCleanupTmout<=0.) return false;

   switch (fCleanupStatus) {
      case stOff:
         if ((fNumMem > fMemSecondary) && fMem[fNumMem-1]->IsBlockFree()) {
            fCleanupStatus = stMemCleanup;
            fMemCleanupStatus = true;
         } else
         if ((fNumRef > fRefPrimary) && fRef[fNumRef-1]->IsBlockFree()) {
            fCleanupStatus = stRefCleanup;
            fRefCleanupStatus = true;
         }

         if (fCleanupStatus != stOff) return true;

         break;

      case stMemCleanup:
         if (!fMem[fNumMem-1]->IsBlockFree())
            fMemCleanupStatus = false;
         break;

      case stRefCleanup:
         if (!fRef[fNumRef-1]->IsBlockFree())
            fRefCleanupStatus = false;
         break;
   }

   return false;
}

double dabc::MemoryPool::ProcessTimeout(double)
{
   // check cleanup flags and probably, delete last memory or references block

   DOUT5(("Process timeout status:%d", fCleanupStatus));

   LockGuard guard(fPoolMutex);

   switch (fCleanupStatus) {
      case stOff:
         break;

      case stMemCleanup:
         if (fMemCleanupStatus)
            _ReleaseLastMemBlock();
         break;

      case stRefCleanup:
         if (fRefCleanupStatus)
            _ReleaseLastRefBlock();
         break;
   }

   fCleanupStatus = stOff;

   return _CheckCleanupStatus() ? fCleanupTmout : -1.;
}

void dabc::MemoryPool::Print()
{
   LockGuard guard(fPoolMutex);

   DOUT1(("Pool:% numem:%d numref:%d fixed:%s", GetName(), fNumMem, fNumRef, DBOOL(fMemLayoutFixed)));

   for (BlockNum_t id=0;id<fNumMem;id++)
     if (fMem[id]==0)
        DOUT1(("  Mem[%u] = null", id));
     else
        DOUT1(("  Mem[%u] : size %lu bufsize %lu used %u", fMem[id]->BlockSize(), fMem[id]->BufferSize(), fMem[id]->NumUsedBuffers()));

   for (BlockNum_t id=0;id<fNumRef;id++)
     if (fRef[id]==0)
        DOUT1(("  Ref[%u] = null", id));
     else
        DOUT1(("  Ref[%u] : size %lu bufsize %lu used %u", fRef[id]->BlockSize(), fRef[id]->BufferSize(), fRef[id]->NumUsedBuffers()));
}

bool dabc::MemoryPool::Store(ConfigIO &cfg)
{
   if (NumChilds()==0) return true;

   cfg.CreateItem(clMemoryPool);

   cfg.CreateAttr(xmlNameAttr, GetName());

   StoreChilds(cfg);

   cfg.PopItem();

   return true;
}

bool dabc::MemoryPool::Find(ConfigIO &cfg)
{
   if (!cfg.FindItem(clMemoryPool)) return false;

   return cfg.CheckAttr(xmlNameAttr, GetName());

}


dabc::BufferSize_t dabc::MemoryPool::RoundBufferSize(dabc::BufferSize_t bufsize)
{
   if (bufsize==0) return 0;

   BufferSize_t size = 256;
   while (size<bufsize) size*=2;
   return size;
}

std::string dabc::MemoryPool::BlockName(dabc::BufferSize_t bufsize)
{
   if (bufsize==0) return "References/";
   return dabc::format("Block_%u/", bufsize);
}
