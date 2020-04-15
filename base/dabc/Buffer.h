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

#ifndef DABC_Buffer
#define DABC_Buffer

#include <cstdint>
#include <cstdlib>

#ifndef DABC_Object
#include "dabc/Object.h"
#endif

#ifndef DABC_defines
#include "dabc/defines.h"
#endif

namespace dabc {

   typedef uint32_t BufferSize_t;

   extern const BufferSize_t BufferSizeError;

   class Pointer;

   enum EBufferTypes {
      mbt_Null         = 0,
      mbt_Generic      = 1,
      mbt_Int64        = 2,
      mbt_TymeSync     = 3,
      mbt_AcknCounter  = 4,
      mbt_RawData      = 5,
      mbt_EOF          = 6,   // such packet produced when transport is closed completely
      mbt_EOL          = 7,   // this is more like end-of-line case, where transport is not closed, but may deliver new kind of data (new event ids) afterwards
      mbt_User         = 1000
   };

   class MemoryPool;

   /** \brief Structure with descriptor of single memory segment */
   struct MemSegment {
      unsigned        id;        ///< id of the buffer
      unsigned        datasize;  ///< length of data
      void*           buffer;    ///< pointer on the beginning of buffer (must be in the area of id)
      void copy_from(MemSegment* src)
        { id = src->id; datasize = src->datasize; buffer = src->buffer; }
   };

   /** \brief Container for data, referenced by \ref Buffer class
    *
    * \ingroup dabc_all_classes
    *
    * It contains lists of memory segments from memory pool.
    * In special case memory can be allocated and owned by container itself.
    */

   class BufferContainer : public Object {
      friend class Buffer;
      friend class MemoryPool;

      protected:
         unsigned     fTypeId;       ///< buffer type, identifies content of the buffer

         unsigned     fNumSegments;  ///< number of entries in segments list
         unsigned     fCapacity;     ///< capacity of segments list

         Reference    fPool;         ///< reference on the memory pool (or other object, containing memory)

         MemSegment*  fSegm;         ///< array of segments

         BufferContainer() :
            Object(0, "", flAutoDestroy | flNoMutex),
            fTypeId(0),
            fNumSegments(0),
            fCapacity(0),
            fPool(),
            fSegm(0)
            {
              #ifdef DABC_EXTRA_CHECKS
                 DebugObject("Buffer", this, 1);
              #endif
            }

         void* operator new(size_t sz) {
            return malloc(sz);
         }

         void* operator new(size_t sz, void* area) {
            if (area) return area;
            return malloc(sz);
         }

         void operator delete(void* area)
         {
            free(area);
         }

      public:

         virtual ~BufferContainer();
   };

   /** \brief Reference on memory from memory pool
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    * Central class for memory usage concept in DABC framework.
    *
    * All memory for data transfer should be pre-allocated in \ref dabc::MemoryPool.
    * Buffer class provides access to that memory.
    *
    * Main concepts:
    *   * avoid memcpy as much as possible
    *   * exchange references between local modules
    *   * support zero-copy where possible (like InfiniBand VERBS)
    *   * reuse memory from memory pool, do not allocate/deallocate memory every time
    *   * provide possibility to gather many segments together (segments list)
    *
    * Actual class description need to be done.
    */

   class Buffer : public Reference {

      friend class MemoryPool;

      DABC_REFERENCE(Buffer, Reference, BufferContainer)

      protected:

         void Locate(BufferSize_t p, unsigned& seg_indx, unsigned& seg_shift) const;

         void AllocateContainer(unsigned capacity);

         MemoryPool* PoolPtr() const;

      public:

      void SetTypeId(unsigned tid) { if (!null()) GetObject()->fTypeId = tid; }
      inline unsigned GetTypeId() const { return null() ? 0 : GetObject()->fTypeId; }

      /** Returns reference on the pool, in user code MemoryPoolRef can be used like
       *  dabc::Buffer buf = Recv();
       *  dabc::MemoryPoolRef pool = buf.GetPool(); */
      Reference GetPool() const;

      /** Returns true if buffer produced by the pool provided as reference */
      bool IsPool(const Reference& pool) const { return null() ? false : pool == GetObject()->fPool; }

      /** Returns number of segment in buffer */
      unsigned NumSegments() const { return null() ? 0 : GetObject()->fNumSegments; }

      MemSegment* Segments() const { return null() ? 0 : GetObject()->fSegm; }

      /** Returns id of the segment, no any boundary checks */
      inline unsigned SegmentId(unsigned n = 0) const { return GetObject()->fSegm[n].id; }

      /** Returns pointer on the segment, no any boundary checks */
      inline void* SegmentPtr(unsigned n = 0) const { return GetObject()->fSegm[n].buffer; }

      /** Returns size on the segment, no any boundary checks */
      inline unsigned SegmentSize(unsigned n = 0) const { return GetObject()->fSegm[n].datasize; }

      /** Return total size of all buffer segments */
      BufferSize_t GetTotalSize() const;

      /** \brief Duplicates instance of Buffer with new segments list independent from source.
       *
       * Means original instance will remain as is.
       * Memory which is referenced by Buffer object is not duplicated or copied,
       * means both instances are reference same memory regions.
       * This method can be time consuming, while pool mutex should be locked and
       * reference counter of all segments should be incremented.
       * When created instance destroyed, same operation in reverse order will be performed.
       * Method must be used when new instance should be used in other thread while initial
       * instance Do not duplicate buffer without real need for that. */
      Buffer Duplicate() const;

      /** \brief Method hands over buffer, that source will be emptied at the end
       *
       *  This method creates temporary object which could be deliver to the
       *  any other method as argument. At the same time original object forgets
       *  buffer and will be empty at the end of the call.
       *  Typical place where it should be used is return method when source want to
       *  forget buffer itself:
       *
       *  dabc::Buffer MyClass::GetLast()
       *  {
       *     fLastBuf.SetTotalSize(1024);
       *     fLastBuf.SetTypeId(129);
       *     return fLastBuf.HandOver();
       *  }
       *
       *  If one wants to keep reference on the memory after such call one should do:
       *  ...
       *     return fLastBuf;
       *  ...
       */
      Buffer HandOver() { return Take(); }


      /** Method produce buffer with empty segments list
       * Such buffer can be used to collect segments from other buffers without disturbing other bufs */
      void MakeEmpty(unsigned capacity = 8) { AllocateContainer(capacity); }

      /** Set total length of the buffer to specified value
       *  Size cannot be bigger than original size of the buffer */
      void SetTotalSize(BufferSize_t len) throw();

      /** Remove part of buffer from the beginning */
      void CutFromBegin(BufferSize_t len) throw();


      /** \brief Append content of provided buffer.
       *
       * If source buffer belong to other memory pool, content will be copied.
       * \param[in] src       buffer to append
       * \param[in] moverefs  if true, references moved to the target and source buffer will be emptied
       * \returns false if operation failed, otherwise true */
      bool Append(Buffer& src, bool moverefs = true) throw();

      /** \brief Prepend content of provided buffer.
       *
       * If source buffer belong to other memory pool, content will be copied.
       * \param[in] src       buffer to prepend
       * \param[in] moverefs  if true, references moved to the target and source buffer will be emptied
       * \returns false if operation failed, otherwise true */
      bool Prepend(Buffer& src, bool moverefs = true) throw();

      /** \brief Insert content of buffer at specified position.
       *
       * If source buffer belong to other memory pool, content will be copied.
       * \param[in] pos       position at which buffer must be inserted
       * \param[in] src       buffer to insert
       * \param[in] moverefs  if true, references moved to the target and source buffer will be emptied
       * \returns false if operation failed, otherwise true */
      bool Insert(BufferSize_t pos, Buffer& src, bool moverefs = true) throw();


      /** \brief Convert content of the buffer into std::string */
      std::string AsStdString();

      /** \brief Copy content from source buffer
       *
       * \param[in] srcbuf  source buffer
       * \param[in] len     bytes to copy, if 0 - whole buffer
       * \returns           actual number of bytes copied */
      BufferSize_t CopyFrom(const Buffer& srcbuf, BufferSize_t len = 0) throw();

      /** \brief Copy data from string.
       *
       * \param[in] src   source string
       * \param[in] len   bytes copied, (if 0, complete string excluding trailing null)
       * \returns         actual number of bytes copied  */
      BufferSize_t CopyFromStr(const char* src, unsigned len = 0) throw();

      /** \brief Copy content into provided raw buffer.
       *
       * \param[in] ptr  pointer to destination memory
       * \param[in] len  number of bytes to copy.
       * \returns        actual number of bytes copied*/
      BufferSize_t CopyTo(void* ptr, BufferSize_t len) const throw();


      /** \brief Returns reference on the part of the memory, referenced by the object.
       *
       * \param[in,out] ptr  pointer where next part will be extracted
       * \param[in]    len   length of memory piece (0 - rest size of current segment will be delivered).
       * \param[in]    allowsegmented defines if return memory peace could be segmented or not.
       * \returns      reference of buffer part or empty buffer when requested memory size is not available in the source buffer
       *
       * This method could be used to manage small peaces of memory in the memory pool with big buffers.
       * For instance, one need some small headers which should managed separately from the main data.
       * Than one need to take big buffer from the pool and extract by small pieces:
       *
       *  // somewhere in the initialization
       * Buffer buf = Pool()->TakeBuffer(64000);
       *
       *  // later in the code
       * Buffer data = Recv();
       * Buffer hdr = buf.GetNextPart(16)
       * data.Prepend(hdr);
       * Send(data);
       *
       * Of course, one should check if buffer is expired and one need to take next piece
       * from the memory pool.
       *
       * TODO: one can exclude pointer and just cut buffer pieces until buffer is empty
       */
      Buffer GetNextPart(Pointer& ptr, BufferSize_t len, bool allowsegmented = true) throw();

      /** Returns true when user can modify buffer
       * content without any doubts. It happens when refcnt==1 for each memory segments */
      bool CanSafelyChange() const;

      // ===================================================================================

      /** This static method create independent buffer for any other memory pools
       * Therefore it can be used in standalone case */
      static Buffer CreateBuffer(BufferSize_t sz) throw();

      /** This static method create Buffer instance, which contains pointer on specified peace of memory
       * Therefore it can be used in standalone case */
      static Buffer CreateBuffer(const void* ptr, unsigned size, bool owner = false, bool makecopy = false) throw();
   };

};

#endif
