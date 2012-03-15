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

#include <stdint.h>

#ifndef DABC_Reference
#include "dabc/Reference.h"
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
      mbt_EOL          = 7,   // this is more line end-of-line case, where transport is not closed, but may deliver new kind of data (new event ids) afterwards
      mbt_User         = 1000
   };

   class MemoryPool;
   class BuffersQueue;
   class Mutex;

   struct MemSegment {
       unsigned        id;        // id of the buffer
       unsigned        datasize;  // length of data
       void*           buffer;    // pointer on the beginning of buffer (must be in the area of id)
   };

   class Buffer {
      friend class MemoryPool;
      friend class BuffersQueue;

      protected:

         struct BufferRec {

            int          fRefCnt;       //!< counter of local references on the record, accessed without mutex, only allowed from the same thread

            // TODO: for debug purposes one can put thread id to verify that access performing always from the same thread
            // TODO: this is critical when fRefCnt>1 - means several buffers are exists and reference same record

            // TODO: In future one could use Reference here, but now back to pointer
            MemoryPool*  fPool;         //!< pointer to the pool, reserved with ref counter
            unsigned     fPoolId;       //!< id, used by pool to identify segments list, when -1 without pool identifies that memory in segments owned by Buffer

            unsigned     fNumSegments;  //!< number of entries in segments list
            unsigned     fCapacity;     //!< capacity of segments list

            unsigned     fTypeId;       //!< buffer type, identifies content of the buffer

            /** list of memory segments, allocated by memory pool, allocated right after record itself */
            inline MemSegment* Segments() { return (MemSegment*) ((char*) this + sizeof(BufferRec)); }

            inline MemSegment* Segment(unsigned nseg) { return Segments() + nseg; }

            inline void increfcnt() { fRefCnt++; }
            inline bool decrefcnt() { return --fRefCnt==0; }
         };

         BufferRec*   fRec;          //!< pointer on the record, either allocated by pool or explicitely, can be detected


         MemSegment* Segments() const { return fRec ? fRec->Segments() : 0; }

         void SetNumSegments(unsigned sz) { if (fRec) fRec->fNumSegments = sz; }

         void Locate(BufferSize_t p, unsigned& seg_indx, unsigned& seg_shift) const;

         /** Allocates own record with specified capacity */
         void AllocateRec(unsigned capacity = 4);

         /** Assigns external rec */
         void AssignRec(void* rec, unsigned recfullsize);

      public:

         Buffer();
         Buffer(const Buffer& src) throw();
         ~Buffer();

         /** Returns true if buffer is empty. Happens when buffer created via default destructor
           * or was released */
         inline bool null() const { return fRec == 0; }

         /** Duplicate instance of Buffer with new segments list independent from source.
          * Means original instance will remain as is.
          * Memory which is referenced by Buffer object is not duplicated or copied,
          * means both instances are reference same memory regions.
          * This method can be time consuming, while pool mutex should be locked and
          * reference counter of all segments should be incremented.
          * When created instance destroyed, same operation in reverse order will be performed.
          * Just do not duplicate buffer without real need for that. */
         Buffer Duplicate() const;

         /** This method creates temporary object which could be deliver to the
          *  any other method as argument. At the same time original object forgets
          *  buffer and will be empty at the end of the call. Typical place where it should be used:
          *  ...
          *  port->Send(buf.HandOver());
          *  ...
          *  If one wants to keep reference on the memory after such call one should do:
          *  ...
          *  port->Send(buf.Duplicate());
          *  ...
          */
         Buffer HandOver();

         void SetTypeId(unsigned tid) { if (fRec) fRec->fTypeId = tid; }
         inline unsigned GetTypeId() const { return fRec ? fRec->fTypeId : mbt_Null; }

         MemoryPool* GetPool() const { return fRec ? fRec->fPool : 0; }

         inline unsigned NumSegments() const { return fRec ? fRec->fNumSegments : 0; }

         /** Returns id of the segment, no any boundary checks */
         inline unsigned SegmentId(unsigned n = 0) const { return fRec->Segment(n)->id; }

         /** Returns ponter on the segment, no any boundary checks */
         inline void* SegmentPtr(unsigned n = 0) const { return fRec->Segment(n)->buffer; }

         /** Returns size on the segment, no any boundary checks */
         inline unsigned SegmentSize(unsigned n = 0) const { return fRec->Segment(n)->datasize; }

         /** Return total size of all buffer segments */
         BufferSize_t GetTotalSize() const;

         /** Set total length of the buffer to specified value
          *  Size cannot be bigger than original size of the buffer */
         void SetTotalSize(BufferSize_t len) throw();

         Buffer& operator=(const Buffer& src) throw();

         Buffer& operator<<(Buffer& src) throw();

         bool operator==(const Buffer& src) const throw() { return fRec == src.fRec; }

         bool operator!=(const Buffer& src) const throw() { return fRec != src.fRec; }

         /** \brief Release reference on the pool memory */
         void Release() throw();

         /** \brief Release reference on the pool memory,
          * content of object will be changed under locked mutex */
         void Release(Mutex* m) throw();

         /** Append content of \param src buffer to the existing buffer.
          * If source buffer belong to other memory pool, content will be copied.
          * If moverefs = true specified, references moved to the target and source buffer will be empty */
         bool Append(Buffer& src, bool moverefs = true) throw();

         /** Prepend content of \param src buffer to the existing buffer.
          * If source buffer belong to other memory pool, content will be copied.
          * If moverefs = true specified, references moved to the target and source buffer will be empty */
         bool Prepend(Buffer& src, bool moverefs = true) throw();

         /** Insert content of \param src buffer at specified position to the existing buffer.
          * If source buffer belong to other memory pool, content will be copied.
          * If moverefs = true specified, references moved to the target and source buffer will be empty */
         bool Insert(BufferSize_t pos, Buffer& src, bool moverefs = true) throw();

         /** Convert content of the buffer into std::string */
         std::string AsStdString();

         // ============ all following methods are relative to current position in the buffer

         /** Initialize pointer instance.
          * By default, complete buffer content covered by the pointer.
          * If \param pos specified (non zero), pointer shifted
          * If \param len specified (non zero), length is specified   */
         Pointer GetPointer(BufferSize_t pos = 0, BufferSize_t len = 0) const;

         void Shift(Pointer& ptr, BufferSize_t len) const;

         /** Copy content of source buffer \param srcbuf to the buffer */
         BufferSize_t CopyFrom(const Buffer& srcbuf, BufferSize_t len = 0) throw();

         /** Copy data from \param src into Buffer memory
          * Returns byte counts actually copied  */
         BufferSize_t CopyFromStr(const char* src, unsigned len = 0) throw();

         /** Performs memcpy of data into raw buffer \param ptr */
         BufferSize_t CopyTo(void* ptr, BufferSize_t len) const throw();

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
         static Buffer CreateBuffer(BufferSize_t sz) throw();

         /** This static method create Buffer instance, which contains pointer on specified peace of memory
          * Therefore it can be used in standalone case */
         static Buffer CreateBuffer(const void* ptr, unsigned size, bool owner = false) throw();
   };

};

#endif
