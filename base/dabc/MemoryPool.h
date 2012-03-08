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

#ifndef DABC_MemoryPool
#define DABC_MemoryPool

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#ifndef DABC_Queue
#include "dabc/Queue.h"
#endif

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

#ifndef DABC_Command
#include "dabc/Command.h"
#endif

#include <stdlib.h>

namespace dabc {

   class Mutex;
   class MemoryPool;

   class MemoryPoolRequester {
      friend class MemoryPool;

      private:
         // this members will be accessible only by MemoryPool

         MemoryPool*   pool;
         BufferSize_t  bufsize;
         Buffer        buf;

      protected:

         Buffer TakeRequestedBuffer() { return buf; }

      public:
      // method must be reimplemented in user code to accept or
      // not buffer which was requested before
      // User returns true if he accept buffer, false means that buffer must be released

         MemoryPoolRequester() : pool(0), bufsize(0), buf() {}
         virtual bool ProcessPoolRequest() = 0;
         virtual ~MemoryPoolRequester() { if (!buf.null()) EOUT(("buf !=0 in requester")); }
   };



   // =========================== NEW CODE, REPLACEMENT FOR OLD MEMORYPOOL ========================


   class MemoryBlock;
   class MemoryPoolRef;

   class MemoryPool : public Worker {
      friend class Manager;
      friend class Buffer;
      friend class MemoryPoolRef;

      enum  { evntProcessRequests = evntFirstSystem };

      protected:

         MemoryBlock* fMem;    //!< list of preallocated memory

         MemoryBlock* fSeg;    //!< list of preallocated segmented lists

         unsigned        fAlignment;      //!< alignment border for memory
         unsigned        fMaxNumSegments; //!< maximum number of segments in Buffer objects

         // FIXME: request queue could be expand on the fly, do we need this
         PointersQueue<MemoryPoolRequester> fReqQueue; //!< queue of submitted requests

         bool            fEvntFired;      //!< indicates if event was fired to process memory requests

         unsigned        fChangeCounter;  //!< memory pool change counter, incremented every time memory is allocated or freed

         bool            fUseThread;      //!< indicate if thread functionality should be used to process supplied requests

         static unsigned fDfltAlignment;   //!< default alignment for memory allocation
         static unsigned fDfltNumSegments; //!< default number of segments in memory pool
         static unsigned fDfltRefCoeff;    //!< default coefficient for reference creation
         static unsigned fDfltBufSize;     //!< default buffer size


         /** Reserve raw buffer without creating Buffer instance */
         bool TakeRawBuffer(unsigned& indx);

         /** Release raw buffer, allocated before by TakeRawBuffer */
         void ReleaseRawBuffer(unsigned indx);

         /** Release all references, used in the record (under pool mutex).
          * If return trues, new space is available in the memory pool */
         bool _ReleaseBufferRec(dabc::Buffer::BufferRec* rec);

         /** Release all references, used in the record */
         void ReleaseBufferRec(dabc::Buffer::BufferRec* rec);

         static void _TakeSegmentsList(MemoryPool* pool, dabc::Buffer& buf, unsigned numsegm);

         Buffer _TakeBuffer(BufferSize_t size, bool except, bool reserve_memory = true) throw();

         /** Method to allocate memory for the pool, mutex should be locked */
         bool _Allocate(BufferSize_t bufsize = 0, unsigned number = 0, unsigned refcoef = 0) throw();

         /** Method called by assignment operator of Buffer class to
          * create copy of existing buffer - means same segments list and
          * increase refcounter for all used segments */
         static void DuplicateBuffer(const Buffer& src, Buffer& tgt, unsigned firstseg = 0, unsigned numsegm = 0) throw();

         /** Process submitted requests, if returns true if any requests was processed */
         bool _ProcessRequests();

         /** Inform requesters, that buffer is provided */
         void ReplyReadyRequests();

         /** Method increases ref.counuters of all segments */
         void IncreaseSegmRefs(MemSegment* segm, unsigned num) throw();

         /** Decrease references of specified segments */
         void DecreaseSegmRefs(MemSegment* segm, unsigned num) throw();

         virtual void OnThreadAssigned() { fUseThread = HasThread(); }

         virtual void ProcessEvent(const EventId&);

      public:

         MemoryPool(const char* name, bool withmanager = false);
         virtual ~MemoryPool();

         virtual const char* ClassName() const { return clMemoryPool; }

         inline Mutex* GetPoolMutex() const { return ObjectMutex(); }

         /** Return true, if memory pool is not yet created */
         bool IsEmpty() const;

         /** Following methods could be used for configuration of pool before memory pool is allocated */

         /** Set alignment of allocated memory */
         bool SetAlignment(unsigned align);

         /** Set number of preallocated segments in the buffer.
          * Buffer cannot contain list bigger than this number */
         bool SetMaxNumSegments(unsigned num);

         /** Allocates memory for the memory pool and creates references.
          * Only can be called for empty memory pool.
          * If no values are specified, requested values, configured by modules are used.
          * TODO: Another alternative is to configure memory pool via xml file */
         bool Allocate(BufferSize_t bufsize = 0, unsigned number = 0, unsigned refcoef = 0) throw();

         /** This is alternative method to supply memory to the pool.
          * User could allocate buffers itself and provide it to this method.
          * If specified, memory pool will take ownership over this memory -
          * means free() function will be called to release this memory */
         bool Assign(bool isowner, const std::vector<void*>& bufs, const std::vector<unsigned>& sizes, unsigned refcoef = 0) throw();

         /** Return pointers and sizes of all memory buffers in the pool
          * Could be used by devices and transport to map all buffers into internal structures.
          * Memory pool mutex must be locked at this point, changecnt can be used to identify
          * if memory pool was changed since last call */
         bool _GetPoolInfo(std::vector<void*>& bufs, std::vector<unsigned>& sizes, unsigned* changecnt = 0);

         /** Return pointers and sizes of all memory buffers in the pool */
         bool GetPoolInfo(std::vector<void*>& bufs, std::vector<unsigned>& sizes);


         /** Release memory and structures, allocated by memory pool */
         bool Release() throw();

         /** Following methods should be used after memory pool is created */

         /** Returns alignment, used for memory pool allocation */
         unsigned GetAlignment() const;

         /** Returns number of preallocated segments */
         unsigned GetMaxNumSegments() const;

         /** Returns number of preallocated buffers */
         unsigned GetNumBuffers() const;

         /** Returns size of preallocated buffer */
         unsigned GetBufferSize(unsigned id) const;

         /** Returns location of preallocated buffer */
         void* GetBufferLocation(unsigned id) const;

         /** Return relative usage of memory pool buffers */
         double GetUsedRatio() const;

         /** Return maximum buffer size in the pool */
         unsigned GetMaxBufSize() const;

         /** Return minimum buffer size in the pool */
         unsigned GetMinBufSize() const;

         /** Returns Buffer object with exclusive access rights
          * \param size defines requested buffer area, if = 0 returns next empty buffer
          * If size longer as single buffer, memory pool will try to produce segmented list.
          * Returned object will have at least specified size (means, size can be bigger).
          * In case when memory pool cannot provide specified memory exception will be thrown */
         Buffer TakeBuffer(BufferSize_t size = 0) throw();

         /** Returns Buffer or set request to the pool, which will be processed
          * when new memory is available in the buffer */
         Buffer TakeBufferReq(MemoryPoolRequester* req, BufferSize_t size = 0) throw();

         /** Cancel previously submitted request */
         void RemoveRequester(MemoryPoolRequester* req);

         /** Returns Buffer object without any memory reserved.
          * Instance can be used in later code to add references on the memory from other buffers */
         Buffer TakeEmpty(unsigned capacity = 0) throw();

         /** Method used to produce deep copy of source buffer.
          * Means in any case new space will be reserved and content will be copied */
         Buffer CopyBuffer(const Buffer& src, bool except = true) throw();


         /** Check if memory pool structure was changed since last call, do not involves memory pool mutex */
         bool CheckChangeCounter(unsigned &cnt);

         /** Reconstruct memory pool base on command configuration
          * If some parameters not specified, configured values from xml file will be used.
          * As very last point, defaults from static variables will be used */
         bool Reconstruct(Command cmd);

         // these are static methods to change default configuration for all newly created pools

         static unsigned GetDfltAlignment() { return fDfltAlignment; }
         static unsigned GetDfltNumSegments() { return  fDfltNumSegments; }
         static unsigned GetDfltRefCoeff() { return fDfltRefCoeff; }
         static unsigned GetDfltBufSize() { return fDfltBufSize; }

         static void SetDfltAlignment(unsigned v) { fDfltAlignment = v; }
         static void SetDfltNumSegments(unsigned v) { fDfltNumSegments = v; }
         static void SetDfltRefCoeff(unsigned v) { fDfltRefCoeff = v; }
         static void SetDfltBufSize(unsigned v) { fDfltBufSize = v; }
   };


   class CmdCreateMemoryPool : public Command {

      DABC_COMMAND(CmdCreateMemoryPool, "CreateMemoryPool")

      CmdCreateMemoryPool(const char* poolname) : Command(CmdName())
      {
         SetStr(xmlPoolName, poolname);
      }

      void SetMem(unsigned buffersize, unsigned numbuffers, unsigned align = 0)
      {
         if (buffersize*numbuffers) {
            SetUInt(xmlNumBuffers, numbuffers);
            SetUInt(xmlBufferSize, buffersize);
         }

         if (align) SetUInt(xmlAlignment, align);
      }

      void SetRefs(unsigned refcoeff, unsigned numsegm = 0)
      {
         if (refcoeff) SetUInt(xmlRefCoeff, refcoeff);
         if (numsegm) SetUInt(xmlNumSegments, numsegm);
      }

   };

   class MemoryPoolRef : public WorkerRef {

      DABC_REFERENCE(MemoryPoolRef, WorkerRef, MemoryPool)

      Buffer TakeBufferReq(MemoryPoolRequester* req, BufferSize_t size = 0)
      {
         return GetObject() ? GetObject()->TakeBufferReq(req, size) : Buffer();
      }

      /** Return usage of memory pool */
      double GetUsedRatio() const
      {
         return GetObject() ? GetObject()->GetUsedRatio() : 0.;
      }

      /** Return maximum buffer size in the pool */
      unsigned GetMaxBufSize() const
      {
         return GetObject() ? GetObject()->GetMaxBufSize() : 0.;
      }

      /** Return minimum buffer size in the pool */
      unsigned GetMinBufSize() const
      {
         return GetObject() ? GetObject()->GetMinBufSize() : 0.;
      }
   };


}

#endif
