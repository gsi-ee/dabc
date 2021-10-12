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

#include "dabc/MemoryPool.h"

#include <cstdlib>

#include "dabc/defines.h"

namespace dabc {

   enum  { evntProcessRequests = evntModuleLast };


   class MemoryBlock {
      public:

         struct Entry {
            void         *buf;     ///< pointer on raw memory
            BufferSize_t  size;    ///< size of the block
            bool          owner;   ///< is memory should be released
            int           refcnt;  ///< usage counter - number of references on the memory
         };

         typedef Queue<unsigned, false> FreeQueue;

         Entry*    fArr;       ///< array of buffers
         unsigned  fNumber;    ///< number of buffers
         FreeQueue fFree;      ///< list of free buffers

         MemoryBlock() :
            fArr(0),
            fNumber(0),
            fFree()
         {
         }

         virtual ~MemoryBlock()
         {
            Release();
         }

         inline bool IsAnyFree() const { return !fFree.Empty(); }

         void Release()
         {
            if (!fArr) return;

            for (unsigned n=0;n<fNumber;n++) {
               if (fArr[n].buf && fArr[n].owner) {
                  std::free(fArr[n].buf);
               }
               fArr[n].buf = nullptr;
               fArr[n].owner = false;
            }

            delete [] fArr;
            fArr = nullptr;
            fNumber = 0;

            fFree.Reset();
         }

         bool Allocate(unsigned number, unsigned size, unsigned align)
         {
            Release();

            fArr = new Entry[number];
            fNumber = number;

            fFree.Allocate(number);

            for (unsigned n=0;n<fNumber;n++) {

               void* buf(0);
               int res = posix_memalign(&buf, align, size);

               if ((res!=0) || (buf==0)) {
                  EOUT("Cannot allocate data for new Memory Block");
                  throw dabc::Exception(ex_Pool, "Cannot allocate buffer", "MemBlock");
                  return false;
               }

               fArr[n].buf = buf;
               fArr[n].size = size;
               fArr[n].owner = true;
               fArr[n].refcnt = 0;

               fFree.Push(n);
            }

            return true;
         }

         bool Assign(bool isowner, const std::vector<void*>& bufs, const std::vector<unsigned>& sizes) throw()
         {
            Release();

            fArr = new Entry[bufs.size()];
            fNumber = bufs.size();

            fFree.Allocate(bufs.size());

            for (unsigned n=0;n<fNumber;n++) {
               fArr[n].buf = bufs[n];
               fArr[n].size = sizes[n];
               fArr[n].owner = isowner;
               fArr[n].refcnt = 0;

               fFree.Push(n);
            }
            return true;
         }

   };

}

// ---------------------------------------------------------------------------------

unsigned dabc::MemoryPool::fDfltAlignment = 16;
unsigned dabc::MemoryPool::fDfltBufSize = 4096;


dabc::MemoryPool::MemoryPool(const std::string &name, bool withmanager) :
   dabc::ModuleAsync(std::string(withmanager ? "" : "#") + name),
   fMem(0),
   fAlignment(fDfltAlignment),
   fReqests(),
   fPending(16),  // size 16 is preliminary and always can be extended
   fEvntFired(false),
   fProcessingReq(false),
   fChangeCounter(0),
   fUseThread(false)
{
   DOUT3("MemoryPool %p name %s constructor", this, GetName());
   SetAutoStop(false);
}

dabc::MemoryPool::~MemoryPool()
{
   Release();
}

bool dabc::MemoryPool::Find(ConfigIO &cfg)
{
   DOUT4("Module::Find %p name = %s parent %p", this, GetName(), GetParent());

   if (GetParent()==0) return false;

   // module will always have tag "Module", class could be specified with attribute
   while (cfg.FindItem(xmlMemoryPoolNode)) {
      if (cfg.CheckAttr(xmlNameAttr, GetName())) return true;
   }

   return false;
}


bool dabc::MemoryPool::SetAlignment(unsigned align)
{
   LockGuard lock(ObjectMutex());
   if (fMem!=0) return false;
   fAlignment = align;
   return true;
}


unsigned dabc::MemoryPool::GetAlignment() const
{
   LockGuard lock(ObjectMutex());
   return fAlignment;
}

bool dabc::MemoryPool::_Allocate(BufferSize_t bufsize, unsigned number) throw()
{
   if (fMem!=0) return false;

   if (bufsize*number==0) return false;

   DOUT3("POOL:%s Create num:%u X size:%u buffers align:%u", GetName(), number, bufsize, fAlignment);

   fMem = new MemoryBlock;
   fMem->Allocate(number, bufsize, fAlignment);

   fChangeCounter++;

   return true;
}

bool dabc::MemoryPool::Allocate(BufferSize_t bufsize, unsigned number) throw()
{
   LockGuard lock(ObjectMutex());
   return _Allocate(bufsize, number);
}

bool dabc::MemoryPool::Assign(bool isowner, const std::vector<void*>& bufs, const std::vector<unsigned>& sizes) throw()
{
   LockGuard lock(ObjectMutex());

   if (fMem!=0) return false;

   if ((bufs.size() != sizes.size()) || (bufs.size()==0)) return false;

   fMem = new MemoryBlock;
   fMem->Assign(isowner, bufs, sizes);

   return true;
}

bool dabc::MemoryPool::Release() throw()
{
   LockGuard lock(ObjectMutex());

   if (fMem) {
      delete fMem;
      fMem = 0;
      fChangeCounter++;
   }

   return true;
}


bool dabc::MemoryPool::IsEmpty() const
{
   LockGuard lock(ObjectMutex());

   return fMem==0;
}

unsigned dabc::MemoryPool::GetNumBuffers() const
{
   LockGuard lock(ObjectMutex());

   return (fMem==0) ? 0 : fMem->fNumber;
}

unsigned dabc::MemoryPool::GetBufferSize(unsigned id) const
{
   LockGuard lock(ObjectMutex());

   return (fMem==0) || (id>=fMem->fNumber) ? 0 : fMem->fArr[id].size;
}

unsigned dabc::MemoryPool::GetMaxBufSize() const
{
   LockGuard lock(ObjectMutex());

   if (fMem==0) return 0;

   unsigned max(0);

   for (unsigned id=0;id<fMem->fNumber;id++)
      if (fMem->fArr[id].size > max) max = fMem->fArr[id].size;

   return max;
}

unsigned dabc::MemoryPool::GetMinBufSize() const
{
   LockGuard lock(ObjectMutex());

   if (fMem==0) return 0;

   unsigned min(0);

   for (unsigned id=0;id<fMem->fNumber;id++)
      if ((min==0) || (fMem->fArr[id].size < min)) min = fMem->fArr[id].size;

   return min;
}


void* dabc::MemoryPool::GetBufferLocation(unsigned id) const
{
   LockGuard lock(ObjectMutex());

   return (fMem==0) || (id>=fMem->fNumber) ? 0 : fMem->fArr[id].buf;
}

bool dabc::MemoryPool::_GetPoolInfo(std::vector<void*>& bufs, std::vector<unsigned>& sizes, unsigned* changecnt)
{
   if (changecnt!=0) {
      if (*changecnt == fChangeCounter) return false;
   }

   if (fMem!=0)
      for(unsigned n=0;n<fMem->fNumber;n++) {
         bufs.push_back(fMem->fArr[n].buf);
         sizes.push_back(fMem->fArr[n].size);
      }

   if (changecnt!=0) *changecnt = fChangeCounter;

   return true;
}

bool dabc::MemoryPool::GetPoolInfo(std::vector<void*>& bufs, std::vector<unsigned>& sizes)
{
   LockGuard lock(ObjectMutex());
   return _GetPoolInfo(bufs, sizes);
}


bool dabc::MemoryPool::TakeRawBuffer(unsigned& indx)
{
   LockGuard lock(ObjectMutex());
   if ((fMem==0) || !fMem->IsAnyFree()) return false;
   indx = fMem->fFree.Pop();
   return true;
}

void dabc::MemoryPool::ReleaseRawBuffer(unsigned indx)
{
   LockGuard lock(ObjectMutex());
   if (fMem) fMem->fFree.Push(indx);
}


dabc::Buffer dabc::MemoryPool::_TakeBuffer(BufferSize_t size, bool except, bool reserve_memory)
{
   Buffer res;

//   DOUT0("_TakeBuffer obj %p", &res);

   // if no memory is available, try to allocate it
   if (fMem==0) _Allocate();

   if (fMem==0) {
      if (except) throw dabc::Exception(ex_Pool, "No memory allocated in the pool", ItemName());
      return res;
   }

   if (!fMem->IsAnyFree() && reserve_memory) {
      if (except) throw dabc::Exception(ex_Pool, "No any memory is available in the pool", ItemName());
      return res;
   }

   if ((size==0) && reserve_memory) size = fMem->fArr[fMem->fFree.Front()].size;

   // first check if required size is available
   BufferSize_t sum(0);
   unsigned cnt(0);
   while (sum<size) {
      if (cnt>=fMem->fFree.Size()) {
         if (except) throw dabc::Exception(ex_Pool, "Cannot reserve buffer of requested size", ItemName());
         return res;
      }

      unsigned id = fMem->fFree.Item(cnt);
      sum += fMem->fArr[id].size;
      cnt++;
   }

   res.AllocateContainer(cnt < 8 ? 8 : cnt);

   sum = 0;
   cnt = 0;
   MemSegment* segs = res.Segments();

   while (sum<size) {
      unsigned id = fMem->fFree.Pop();

      if (fMem->fArr[id].refcnt!=0)
         throw dabc::Exception(ex_Pool, "Buffer is not free even is declared so", ItemName());

      if (cnt>=res.GetObject()->fCapacity)
         throw dabc::Exception(ex_Pool, "All mem segments does not fit into preallocated list", ItemName());

      segs[cnt].buffer = fMem->fArr[id].buf;
      segs[cnt].datasize = fMem->fArr[id].size;
      segs[cnt].id = id;

      // Provide buffer of exactly requested size
      BufferSize_t restsize = size - sum;
      if (restsize < segs[cnt].datasize) segs[cnt].datasize = restsize;

      sum += fMem->fArr[id].size;

      // increment reference counter on the memory space
      fMem->fArr[id].refcnt++;

      cnt++;
   }

   res.GetObject()->fPool.SetObject(this, false);

   res.GetObject()->fNumSegments = cnt;

   res.SetTypeId(mbt_Generic);

   return res;
}


dabc::Buffer dabc::MemoryPool::TakeBuffer(BufferSize_t size) throw()
{
   dabc::Buffer res;

   {
      LockGuard lock(ObjectMutex());

      res = _TakeBuffer(size, true);
   }

   return res;
}


void dabc::MemoryPool::IncreaseSegmRefs(MemSegment* segm, unsigned num)
{
   LockGuard lock(ObjectMutex());

   if (fMem==0)
      throw dabc::Exception(ex_Pool, "Memory was not allocated in the pool", ItemName());

   for (unsigned cnt=0;cnt<num;cnt++) {
      unsigned id = segm[cnt].id;
      if (id>fMem->fNumber)
         throw dabc::Exception(ex_Pool, "Wrong buffer id in the segments list of buffer", ItemName());

      if (fMem->fArr[id].refcnt + 1 == 0)
         throw dabc::Exception(ex_Pool, "To many references on single segments - how it can be", ItemName());

      fMem->fArr[id].refcnt++;
   }
}

bool dabc::MemoryPool::IsSingleSegmRefs(MemSegment* segm, unsigned num)
{
   LockGuard lock(ObjectMutex());

   if (!fMem)
      throw dabc::Exception(ex_Pool, "Memory was not allocated in the pool", ItemName());

   for (unsigned cnt=0;cnt<num;cnt++) {
      unsigned id = segm[cnt].id;

      if (id>fMem->fNumber)
         throw dabc::Exception(ex_Pool, "Wrong buffer id in the segments list of buffer", ItemName());

      if (fMem->fArr[id].refcnt != 1) return false;
   }

   return true;
}


void dabc::MemoryPool::DecreaseSegmRefs(MemSegment* segm, unsigned num)
{
   LockGuard lock(ObjectMutex());

   if (!fMem)
      throw dabc::Exception(ex_Pool, "Memory was not allocated in the pool", ItemName());

   for (unsigned cnt=0;cnt<num;cnt++) {
      unsigned id = segm[cnt].id;
      if (id > fMem->fNumber)
         throw dabc::Exception(ex_Pool, "Wrong buffer id in the segments list of buffer", ItemName());

      if (fMem->fArr[id].refcnt == 0)
         throw dabc::Exception(ex_Pool, "Reference counter of specified segment is already 0", ItemName());

      if (--(fMem->fArr[id].refcnt) == 0) fMem->fFree.Push(id);
   }

}


bool dabc::MemoryPool::ProcessSend(unsigned port)
{
   if (!fReqests[port].pending) {
      fPending.Push(port);
      fReqests[port].pending = true;
   }

   RecheckRequests(true);

   return true; // always
}


void dabc::MemoryPool::ProcessEvent(const EventId& ev)
{

   if (ev.GetCode() == evntProcessRequests) {
      {
         LockGuard guard(ObjectMutex());
         fEvntFired = false;
      }

      RecheckRequests();

      return;
   }

   dabc::ModuleAsync::ProcessEvent(ev);
}

bool dabc::MemoryPool::RecheckRequests(bool from_recv)
{
   // method called when one need to check if any requester need buffer and
   // was blocked by lack of free memory

   if (fProcessingReq) {
      EOUT("Event processing mismatch in the POOL - called for the second time");
      return false;
   }

   int cnt(100);

   fProcessingReq = true;

   while (!fPending.Empty() && (cnt-->0)) {

      // no any pending requests, nothing to do
      unsigned portid = fPending.Front();

      if (portid>=fReqests.size()) {
         EOUT("Old requests is not yet removed!!!");
         fPending.Pop();
         continue;
      }

      if (!IsOutputConnected(portid)) {
         // if port was disconnected, just skip all pending requests
         fReqests[portid].pending = false;
         fPending.Pop();
         continue;
      }

      if (!fReqests[portid].pending) {
         EOUT("Request %u was not pending", portid);
         fReqests[portid].pending = true;
      }

      if (!CanSend(portid)) {
         EOUT("Cannot send buffer to output %u", portid);
         fPending.Pop();
         continue;
      }

      BufferSize_t sz = fReqests[portid].size;
      Buffer buf;

      {
         LockGuard lock(ObjectMutex());

         buf = _TakeBuffer(sz, false, true);

         if (buf.null()) { fProcessingReq = false; return false; }
      }

      // we cannot get buffer, break loop and do not

      DOUT5("Memory pool %s send buffer size %u to output %u", GetName(), buf.GetTotalSize(), portid);

      if (CanSend(portid)) {
         Send(portid, buf);
      } else {
         EOUT("Buffer %u is ready, but cannot be add to the queue", buf.GetTotalSize());
         buf.Release();
      }

      fPending.Pop();
      fReqests[portid].pending = false;

      if (from_recv && fPending.Empty()) {
         fProcessingReq = false;
         return true;
      }

      // if there are still requests pending, put it in the queue back
      if (CanSend(portid)) {
         fPending.Push(portid);
         fReqests[portid].pending = true;
      }
   }

   // we need to fire event again while not all requests was processed
   if (!fPending.Empty() && (cnt<=0))
      FireEvent(evntProcessRequests);

   fProcessingReq = false;

   return fPending.Empty();
}


bool dabc::MemoryPool::CheckChangeCounter(unsigned &cnt)
{
   bool res = cnt!=fChangeCounter;

   cnt = fChangeCounter;

   return res;
}

bool dabc::MemoryPool::Reconstruct(Command cmd)
{
   unsigned buffersize = Cfg(xmlBufferSize, cmd).AsUInt(GetDfltBufSize());

//   dabc::SetDebugLevel(2);
   unsigned numbuffers = Cfg(xmlNumBuffers, cmd).AsUInt();
//   dabc::SetDebugLevel(1);

   unsigned align = Cfg(xmlAlignment, cmd).AsUInt(GetDfltAlignment());

   DOUT1("POOL:%s bufsize:%u X num:%u", GetName(), buffersize, numbuffers);

   if (align) SetAlignment(align);

   return Allocate(buffersize, numbuffers);
}

double dabc::MemoryPool::GetUsedRatio() const
{
   LockGuard lock(ObjectMutex());

   if (fMem==0) return 0.;

   double sum1(0.), sum2(0.);
   for(unsigned n=0;n<fMem->fNumber;n++) {
      sum1 += fMem->fArr[n].size;
      if (fMem->fArr[n].refcnt>0)
         sum2 += fMem->fArr[n].size;
   }

   return sum1>0. ? sum2/sum1 : 0.;
}


int dabc::MemoryPool::ExecuteCommand(Command cmd)
{
   if (cmd.IsName("CreateNewRequester")) {

      unsigned portindx = NumOutputs();
      std::string name = dabc::format("Output%u", portindx);

      for (unsigned n=0;n<fReqests.size();n++)
         if (fReqests[n].disconn) {
            portindx = n;
            name = OutputName(portindx);
            break;
         }

      if (portindx==NumOutputs()) {
         CreateOutput(name, 1);
         portindx = FindOutput(name);
         if (portindx != fReqests.size()) {
            EOUT("Cannot create port");
            exit(444);
         }
         fReqests.push_back(RequesterReq());
      }

      cmd.SetStr("PortName", name);

      return cmd_true;
   }

   return ModuleAsync::ExecuteCommand(cmd);
}

void dabc::MemoryPool::ProcessConnectionActivated(const std::string &name, bool on)
{
   DOUT4(" MemoryPool %s Port %s active=%s", GetName(), name.c_str(), DBOOL(on));

   if (on) ProduceOutputEvent(FindOutput(name));
}


void dabc::MemoryPool::ProcessConnectEvent(const std::string &name, bool on)
{
   unsigned portid = FindOutput(name);
   if (portid >= fReqests.size()) return;
   if (!on) fReqests[portid].disconn = true;
}


// =========================================================================

dabc::Reference dabc::MemoryPoolRef::CreateNewRequester()
{
   if (GetObject()==0) return dabc::Reference();

   dabc::Command cmd("CreateNewRequester");

   if (Execute(cmd)==dabc::cmd_true)
      return FindPort(cmd.GetStr("PortName"));

   return dabc::Reference();
}


