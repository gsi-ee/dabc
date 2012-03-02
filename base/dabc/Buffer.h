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

#ifndef DABC_Pointer
#include "dabc/Pointer.h"
#endif

#ifndef DABC_Reference
#include "dabc/Reference.h"
#endif

namespace dabc {

   // buffer id includes memory block id (high 16-bit)  and number inside block (low 16 bit)
   // buffer size if limited by 32 bit - 4 gbyte is pretty enough for single structure

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
   class BuffersQueue;
   class Mutex;

   struct MemSegment {
       unsigned        id;        // id of the buffer
       BufferSize_t    datasize;  // length of data
       void*           buffer;    // pointer on the beginning of buffer (must be in the area of id)
   };


   class Buffer {
      friend class MemoryPool;
      friend class BuffersQueue;

      protected:

         Reference    fPool;         //!< reference to the pool, prevents pool deletion until buffer is used
         unsigned     fPoolId;       //!< id, used by pool to identify segments list

         MemSegment*  fSegments;     //!< list of memory segments, allocated by memory pool
         unsigned     fNumSegments;  //!< number of entries in segments list
         unsigned     fCapacity;     //!< capacity of segments list

         uint32_t     fTypeId;       //!< buffer type, identifies content of the buffer

         void Locate(BufferSize_t p, unsigned& seg_indx, unsigned& seg_shift) const;

      public:

         Buffer();
         Buffer(const Buffer& src) throw();
         ~Buffer();

         /** Changes transient state of the buffer. If true, in any assign or method call
          * all references in buffer will be moved and buffer will be empty afterwards
          * For instance, after following code:
          *    dabc::Buffer buf1 = pool->TakeBuffer(4096), buf2;
          *    buf1.SetTransient(true);
          *    buf2 = buf1;
          * Buffer buf2 will reference 4096 bytes long buffer when buf1 will be empty. Code is
          * equivalent to following:
          *    dabc::Buffer buf1 = pool->TakeBuffer(4096), buf2;
          *    buf2 << buf1;
          **/
         Buffer& SetTransient(bool on = true) { fPool.SetTransient(on); return *this; }

         /** Return transient state of buffer, see SetTransient for more detailed explanation */
         bool IsTransient() const { return fPool.IsTransient(); }

         /** Disregard of transient state duplicate instance of Buffer will be created
          * Means original instance will remain as is.
          * This method can be time consuming, while pool mutex should be locked and
          * reference counter of all segments should be incremented.
          * When created instance destroyed, same operation in reverse order will be performed.
          * Just do not duplicate buffer without real need for that.  */
         Buffer Duplicate() const;

         void SetTypeId(uint32_t tid) { fTypeId = tid; }
         inline uint32_t GetTypeId() const { return fTypeId; }

         /** Returns true if buffer does not reference any memory
             TODO: Check if second condition could be removed, than ispool can be removed */
         inline bool null() const { return fPool.null() || (NumSegments() == 0); }

         /** Return true if pool is assigned and list for segments is available */
         inline bool ispool() const { return !fPool.null(); }

         inline unsigned NumSegments() const { return fNumSegments; }
         inline unsigned SegmentId(unsigned n = 0) const { return n < NumSegments() ? fSegments[n].id : 0; }
         inline void* SegmentPtr(unsigned n = 0) const { return n < NumSegments() ? fSegments[n].buffer : 0; }
         inline BufferSize_t SegmentSize(unsigned n = 0) const { return n < NumSegments() ? fSegments[n].datasize : 0; }

         /** Return total size of all buffer segments */
         BufferSize_t GetTotalSize() const;

         /** Set total length of the buffer to specified value
          *  Size cannot be bigger than original size of the buffer */
         void SetTotalSize(BufferSize_t len) throw();

         Buffer& operator=(const Buffer& src) throw();

         Buffer& operator<<(const Buffer& src);

         /** \brief Release reference on the pool memory */
         void Release() throw();

         /** \brief Release reference on the pool memory,
          * content of object will be changed under locked mutex */
         void Release(Mutex* m) throw();

         /** Append content of \param src buffer to the existing buffer.
          * If source buffer belong to other memory pool, content will be copied.
          * If source buffer is transient, after the call it will be empty */
         bool Append(Buffer& src) throw();

         /** Prepend content of \param src buffer to the existing buffer.
          * If source buffer belong to other memory pool, content will be copied.
          * If source buffer is transient, after the call it will be empty */
         bool Prepend(Buffer& src) throw();

         /** Insert content of \param src buffer at specified position to the existing buffer.
          * If source buffer belong to other memory pool, content will be copied.
          * If source buffer is transient, after the call it will be empty */
         bool Insert(BufferSize_t pos, Buffer& src) throw();

         /** Convert content of the buffer into std::string */
         std::string AsStdString();

         // ============ all following methods are relative to current position in the buffer

         /** Initialize pointer instance.
          * By default, complete buffer content covered by the pointer.
          * If \param pos specified (non zero), pointer shifted
          * If \param len specified (non zero), length is specified
          */

         Pointer GetPointer(BufferSize_t pos = 0, BufferSize_t len = 0) const
         {
            Pointer res;
            res.fSegm = 0;
            res.fPtr = (unsigned char*) SegmentPtr(0);
            res.fRawSize = SegmentSize(0);
            res.fFullSize = GetTotalSize();
            if (pos>0) Shift(res, pos);
            if (len>0) res.setfullsize(len);
            return res;
         }

         void Shift(Pointer& ptr, BufferSize_t len) const;

         /** Calculates distance between two pointers */
         int Distance(const Pointer& ptr1, const Pointer& ptr2) const;

         /** Performs memcpy of data from source buffer, starting from specified \param tgtptr position
          * If len not specified, all data from buffer \param srcbuf will be copied, starting from specified \param srcptr position
          * Return actual size of memory which was copied. */
         BufferSize_t CopyFrom(Pointer tgtptr, const Buffer& srcbuf, Pointer srcptr, BufferSize_t len = 0) throw();


         /** Copy content of source buffer \param srcbuf to the buffer */
         BufferSize_t CopyFrom(const Buffer& srcbuf, BufferSize_t len = 0) throw()
         {
            return CopyFrom(GetPointer(), srcbuf, srcbuf.GetPointer(), len);
         }

         /** Performs memcpy of data from source buffer, starting from specified \param tgtptr position
          * If len not specified, all data from buffer \param srcbuf will be copied, starting from specified \param srcptr position
          * Return actual size of memory which was copied. */
         BufferSize_t CopyFrom(Pointer tgtptr, Pointer srcptr, BufferSize_t len = 0) throw();

         /** Performs memcpy of data from raw buffer \param ptr */
         BufferSize_t CopyFrom(Pointer tgtptr, const void* ptr, BufferSize_t len) throw()
         {
            return CopyFrom(tgtptr, Pointer(ptr,len), len);
         }

         /** Performs memcpy of data from raw buffer \param ptr */
         BufferSize_t CopyFrom(const void* ptr, BufferSize_t len) throw()
         {
            return CopyFrom(GetPointer(), ptr, len);

         }

         /** Copy data from \param src into Buffer memory
          * Returns byte counts actually copied  */
         BufferSize_t CopyFromStr(Pointer tgtptr, const char* src, unsigned len = 0) throw();

         BufferSize_t CopyFromStr(const char* src, unsigned len = 0) throw()
         {
            return CopyFromStr(GetPointer(), src, len);
         }


         /** Performs memcpy of data into raw buffer \param ptr */
         BufferSize_t CopyTo(Pointer srcptr, void* ptr, BufferSize_t len) const throw();

         /** Performs memcpy of data into raw buffer \param ptr */
         BufferSize_t CopyTo(void* ptr, BufferSize_t len) const throw()
         {
            return CopyTo(GetPointer(), ptr, len);
         }

         /** Returns reference on the part of the memory, referenced by the object.
          * \param pos specified start point and will be increased when method returns valid buffer.
          * \param len specifies length of memory piece. If zero is specified,
          * full size of next segment will be delivered.
          * \param allowsegmented defines if return memory peace could be segmented or not.
          * Method returns empty buffer when requested memory size is not available in the source buffer
          *
          * This method could be used to manage small peaces of memory in the memory pool with big buffers.
          * For instance, one need some small headers which should managed separately from the main data.
          * Than one need to take big buffer from the pool and extract by small pieces:
          *
          *  // somewhere in the initialization
          * Buffer buf = Pool()->TakeBuffer(64000);
          *
          *  // later in the code
          * Buffer data = Input()->Recv();
          * Buffer hdr = buf.GetNextPart(16)
          * data.Prepend(hdr);
          * Output()->Send(data);
          *
          * Of course, one should check if buffer is expired and one need to take next piece
          * from the memory pool.
          */
         Buffer GetNextPart(Pointer& ptr, BufferSize_t len, bool allowsegmented = true) throw();


         // ===================================================================================

         /** This static method create independent buffer for any other memory pools
          * Therefore it can be used in standalone case */
         static Buffer CreateBuffer(BufferSize_t sz, unsigned numrefs = 8, unsigned numsegm = 4) throw();

         /** This static method create Buffer instance, which contains pointer on specified peace of memory
          * Therefore it can be used in standalone case */
         static Buffer CreateBuffer(const void* ptr, unsigned size, bool owner = false, unsigned numrefs = 8, unsigned numsegm = 4) throw();
   };

};

#endif
