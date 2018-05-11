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

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
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

#include <vector>

namespace dabc {

   class Mutex;
   class MemoryBlock;
   class MemoryPoolRef;

   /** \brief Memory pool
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    * Allocates and manage memory, which than can be accessed via \ref dabc::Buffer class
    */

   class BufferContainer;

   class MemoryPool : public ModuleAsync {
      friend class Manager;
      friend class Buffer;
      friend class MemoryPoolRef;
      friend class BufferContainer;

      struct RequesterReq {
         BufferSize_t   size;
         bool           pending;   // if true, request is pending
         bool           disconn;   // if true, requester output is free and can be reused

         RequesterReq() : size(0), pending(false), disconn(false) {}
      };

      protected:

         MemoryBlock*              fMem;    ///< list of preallocated memory

         unsigned                  fAlignment;      ///< alignment boundary for memory

         std::vector<RequesterReq> fReqests;        ///< configuration for each output

         Queue<unsigned, true>    fPending;    ///< queue with requester indexes which are waiting release of the memory

         bool                     fEvntFired;      ///< indicates if event was fired to process memory requests
         bool                     fProcessingReq;  ///< flag indicate if memory pool processing requests, for debug purposes

         unsigned                 fChangeCounter;  ///< memory pool change counter, incremented every time memory is allocated or freed

         bool                     fUseThread;      ///< indicate if thread functionality should be used to process supplied requests

         static unsigned          fDfltAlignment;   ///< default alignment for memory allocation
         static unsigned          fDfltBufSize;     ///< default buffer size

         virtual bool Find(ConfigIO &cfg);

         /** Reserve raw buffer without creating Buffer instance */
         bool TakeRawBuffer(unsigned& indx);

         /** Release raw buffer, allocated before by TakeRawBuffer */
         void ReleaseRawBuffer(unsigned indx);

         /** Central method, which reserves memory from pool and fill structures of buffer */
         Buffer _TakeBuffer(BufferSize_t size, bool except, bool reserve_memory = true) throw();

         /** Method to allocate memory for the pool, mutex should be locked */
         bool _Allocate(BufferSize_t bufsize = 0, unsigned number = 0) throw();

         /** Process submitted requests, if returns true if any requests was processed */
         bool _ProcessRequests();

         /** Inform requesters, that buffer is provided */
         void ReplyReadyRequests();

         /** Method increases ref.counuters of all segments */
         void IncreaseSegmRefs(MemSegment* segm, unsigned num) throw();

         /** Decrease references of specified segments */
         void DecreaseSegmRefs(MemSegment* segm, unsigned num) throw();

         /** Return true when all segments has refcnt==1 */
         bool IsSingleSegmRefs(MemSegment* segm, unsigned num) throw();

         virtual void OnThreadAssigned()
         {
            fUseThread = HasThread();
            dabc::ModuleAsync::OnThreadAssigned();
         }

         bool ProcessSend(unsigned port);

         virtual void ProcessEvent(const EventId&);

         virtual int ExecuteCommand(Command cmd);

         /** Method called when port started or stopped. We could start buffer sending */
         virtual void ProcessConnectionActivated(const std::string &name, bool on);

         virtual void ProcessConnectEvent(const std::string &name, bool on);


         bool RecheckRequests(bool from_recv = false);

      public:

         MemoryPool(const std::string &name, bool withmanager = false);
         virtual ~MemoryPool();

         virtual const char* ClassName() const { return "MemoryPool"; }

         inline Mutex* GetPoolMutex() const { return ObjectMutex(); }

         /** Return true, if memory pool is not yet created */
         bool IsEmpty() const;

         /** Following methods could be used for configuration of pool before memory pool is allocated */

         /** Set alignment of allocated memory */
         bool SetAlignment(unsigned align);

         /** Allocates memory for the memory pool and creates references.
          * Only can be called for empty memory pool.
          * If no values are specified, requested values, configured by modules are used.
          * TODO: Another alternative is to configure memory pool via xml file */
         bool Allocate(BufferSize_t bufsize = 0, unsigned number = 0) throw();

         /** This is alternative method to supply memory to the pool.
          * User could allocate buffers itself and provide it to this method.
          * If specified, memory pool will take ownership over this memory -
          * means free() function will be called to release this memory */
         bool Assign(bool isowner, const std::vector<void*>& bufs, const std::vector<unsigned>& sizes) throw();

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

         /** \brief Return maximum buffer size in the pool */
         unsigned GetMaxBufSize() const;

         /** \brief Return minimum buffer size in the pool */
         unsigned GetMinBufSize() const;


         /** \brief Returns Buffer object with exclusive access rights
          *
          * \param[in] size defines requested buffer area, if = 0 returns next empty buffer
          * If size longer as single buffer, memory pool will try to produce segmented list.
          * Returned object will have at least specified size (means, size can be bigger).
          * In case when memory pool cannot provide specified memory exception will be thrown */
         Buffer TakeBuffer(BufferSize_t size = 0) throw();

         /** \brief Check if memory pool structure was changed since last call, do not involves memory pool mutex */
         bool CheckChangeCounter(unsigned &cnt);

         /** \brief Reconstruct memory pool base on command configuration
          *
          * If some parameters not specified, configured values from xml file will be used.
          * As very last point, defaults from static variables will be used */
         bool Reconstruct(Command cmd);

         // these are static methods to change default configuration for all newly created pools

         static unsigned GetDfltAlignment() { return fDfltAlignment; }
         static unsigned GetDfltBufSize() { return fDfltBufSize; }

         static void SetDfltAlignment(unsigned v) { fDfltAlignment = v; }
         static void SetDfltBufSize(unsigned v) { fDfltBufSize = v; }
   };


   class CmdCreateMemoryPool : public Command {

      DABC_COMMAND(CmdCreateMemoryPool, "CreateMemoryPool")

      CmdCreateMemoryPool(const std::string &poolname) : Command(CmdName())
      {
         SetStr(xmlPoolName, poolname);
      }

      void SetMem(unsigned buffersize, unsigned numbuffers, unsigned align = 0)
      {
         if (buffersize*numbuffers != 0) {
            SetUInt(xmlNumBuffers, numbuffers);
            SetUInt(xmlBufferSize, buffersize);
         }

         if (align) SetUInt(xmlAlignment, align);
      }
   };

   // ________________________________________________________________________________

   /** \brief %Reference on \ref dabc::MemoryPool class
    *
    * \ingroup dabc_all_classes
    */

   class MemoryPoolRef : public ModuleAsyncRef {

      DABC_REFERENCE(MemoryPoolRef, ModuleAsyncRef, MemoryPool)

      /** Return usage of memory pool */
      double GetUsedRatio() const
      {
         return GetObject() ? GetObject()->GetUsedRatio() : 0.;
      }

      /** Return maximum buffer size in the pool */
      unsigned GetMaxBufSize() const
      {
         return GetObject() ? GetObject()->GetMaxBufSize() : 0;
      }

      /** Return minimum buffer size in the pool */
      unsigned GetMinBufSize() const
      {
         return GetObject() ? GetObject()->GetMinBufSize() : 0;
      }

      Buffer TakeBuffer(BufferSize_t size = 0)
      {
         return GetObject() ? GetObject()->TakeBuffer(size) : Buffer();
      }

      Reference CreateNewRequester();
   };

}

#endif
