#include "dabc/Buffer.h"

#include "dabc/MemoryPool.h"
#include "dabc/Pointer.h"
#include "dabc/logging.h"

const dabc::BufferSize_t dabc::BufferSizeError = (dabc::BufferSize_t) -1;



void dabc::Buffer::Release(Buffer* buf)
{
   if (buf!=0){
      if (buf->fPool==0)
    	  delete buf;
      else
    	  buf->fPool->ReleaseBuffer(buf);
   }
}

void dabc::Buffer::Release(Buffer* &buf, Mutex* mutex)
{
   dabc::Buffer* zap = 0;

   if (buf!=0) {
      dabc::LockGuard lock(mutex);
      zap = buf; buf = 0;
   }

   dabc::Buffer::Release(zap);
}


dabc::Buffer* dabc::Buffer::CreateBuffer(BufferSize_t sz)
{
   if (sz<=0) return 0;

   dabc::Buffer* buf = new dabc::Buffer::Buffer();

   if (!buf->AllocateInternBuffer(sz)) {
      delete buf;
      return 0;
   }

   return buf;
}


dabc::Buffer::Buffer() :
   fPool(0),
   fTypeId(mbt_Generic),
   fSegments(0),
   fCapacity(0),
   fNumSegments(0),
   fHeader(0),
   fHeaderSize(0),
   fHeaderCapacity(0),
   fInternBuffer(0),
   fReferenceId(0),
   fOwnHeader(0),
   fOwnSegments(0)
{
}

void dabc::Buffer::ReInit(MemoryPool* pool, BufferId_t refid,
                          void* header, unsigned headersize,
                          MemSegment* segm, unsigned numsegments)
{
   // method must be reentrant and allow to restore reference state
   // to the beginning

   if (fOwnHeader) free(fOwnHeader); fOwnHeader = 0;
   if (fOwnSegments) delete [] fOwnSegments; fOwnSegments = 0;

   fPool = pool;
   fReferenceId = refid;

   fHeader = header;
   fHeaderCapacity = headersize;
   fSegments = segm;
   fCapacity = numsegments;

   fNumSegments = 0;
   fHeaderSize = 0;
}

void dabc::Buffer::ReClose()
{
   if (fOwnHeader) {
      if (fHeader==fOwnHeader) { fHeader = 0; fHeaderCapacity = 0; fHeaderSize = 0; }
      free(fOwnHeader);
      fOwnHeader = 0;
   }
   if (fOwnSegments) {
       if (fSegments == fOwnSegments) { fSegments = 0; fNumSegments = 0; fCapacity = 0; }
       delete [] fOwnSegments;
       fOwnSegments = 0;
   }
}


dabc::Buffer::~Buffer()
{
//   DOUT1(("Buffer::~Buffer() %p", this));
   if (fOwnHeader) free(fOwnHeader); fOwnHeader = 0;
   if (fOwnSegments) delete [] fOwnSegments; fOwnSegments = 0;

   fHeader = 0;
   fHeaderSize = 0;
   fHeaderCapacity = 0;

   if (fInternBuffer) free(fInternBuffer);
   fInternBuffer = 0;
   fInternBufferSize = 0;

   fNumSegments = 0;
   fCapacity = 0;
}


bool dabc::Buffer::AllocateInternBuffer(BufferSize_t sz)
{
   if (fInternBuffer!=0) {
      if (fInternBufferSize<sz) {
         fInternBuffer = realloc(fInternBuffer, sz);
         fInternBufferSize = sz;
      }
   }  else {
      fInternBuffer = malloc(sz);
      fInternBufferSize = sz;
   }

   if (fInternBuffer == 0) {
      EOUT(("No memory to allocate buffer size %u", sz));
      return false;
   }

   RellocateSegments(1);

   fNumSegments = 1;

   fSegments[0].id = 0;
   fSegments[0].buffer = fInternBuffer;
   fSegments[0].datasize = sz;

   return true;
}

bool dabc::Buffer::RellocateBuffer(BufferSize_t sz)
{
   if ((fInternBuffer==0) || (NumSegments()!=1)) return false;

   return AllocateInternBuffer(sz);
}

dabc::BufferSize_t dabc::Buffer::GetTotalSize() const
{
   dabc::BufferSize_t sz = 0;
   for (unsigned n=0;n<fNumSegments;n++)
      sz += fSegments[n].datasize;
   return sz;
}

bool dabc::Buffer::RellocateSegments(unsigned newcapacity)
{
   MemSegment* seg = 0;

   if (newcapacity>0) {
      if (newcapacity <= fCapacity) return true;
      if (newcapacity>128) newcapacity = (newcapacity/64 + 1) * 64;
                      else newcapacity = (newcapacity/4 + 1) * 4;
      seg = new MemSegment[newcapacity];
      if (seg==0) return false;
      memset(seg, 0, sizeof(MemSegment) * newcapacity);
   }

   if (fNumSegments > newcapacity)
      fNumSegments = newcapacity;

   if (fNumSegments>0)
      memcpy(seg, fSegments, sizeof(MemSegment) * fNumSegments);

   delete [] fOwnSegments; fOwnSegments = seg;

   fSegments = fOwnSegments;
   fCapacity = newcapacity;

   return true;
}

bool dabc::Buffer::RellocateHeader(BufferSize_t newcapacity, bool copyoldcontent)
{
   void* header = 0;

   if (newcapacity > 0) {
      if (newcapacity <= fHeaderCapacity) return true;

      newcapacity = (newcapacity / 16 + 1) * 16;
      header = malloc(newcapacity);
      if (header==0) return false;
      memset(header, 0, newcapacity);
   }

   if (fHeaderSize>newcapacity) fHeaderSize = newcapacity;

   if (copyoldcontent) {
      if (fHeaderSize>0)
         memcpy(header, fHeader, fHeaderSize);
   }

   free(fOwnHeader); fOwnHeader = header;

   fHeader = fOwnHeader;
   fHeaderCapacity = newcapacity;

   return true;
}

void dabc::Buffer::SetHeaderSize(BufferSize_t sz)
{
   if (sz>fHeaderCapacity) RellocateHeader(sz, true);

   fHeaderSize = sz;
}

bool dabc::Buffer::AddSegment(Buffer* buf, unsigned nseg, BufferSize_t offset, BufferSize_t newsize)
{
   // add new segment, copied from buffer

   if (buf==0) return false;

   if ((Pool()==0) || (Pool() != buf->Pool())) {
      EOUT(("Mismatch of memory pools"));
      return false;
   }

   if (buf->NumSegments()<=nseg) return false;

   MemSegment* segm = &(buf->fSegments[nseg]);

   if (offset >= segm->datasize) {
      EOUT(("Invalid offset value"));
      return false;
   }

   if (newsize == 0) newsize = segm->datasize - offset;
   if (offset + newsize >= segm->datasize) {
      EOUT(("Invalid datasize value"));
      return false;
   }

   if (!fPool->NewReferences(segm, 1)) {
      EOUT(("Cannot make new references for segment"));
      return false;
   }

   if (fNumSegments>=fCapacity) RellocateSegments(fNumSegments+1);

   fSegments[fNumSegments].id = segm->id;
   fSegments[fNumSegments].buffer = (char*) segm->buffer + offset;
   fSegments[fNumSegments].datasize = newsize;

   fNumSegments++;

   return true;
}


bool dabc::Buffer::AddBuffer(Buffer* buf, bool adopt)
{
   // add to list of segments new buffer
   // if adopt true, buffer buf will be relased at the end of function call
   // one MUST have adopt = true, if both this & buf has writable on modes
   // otherwise we will get two refernces with write access

   if (buf==0) return false;

   if ((Pool()==0)  || (Pool() != buf->Pool())) {
      EOUT(("Missmtach in memory pools"));
      return false;
   }

   if (buf->NumSegments()==0) {
      if (adopt) dabc::Buffer::Release(buf);
      return true;
   }

   // if modes not the same or we cannot  adopt buffer,
   // we need to request new references from pool,
   if (!adopt)
     if (!fPool->NewReferences(buf->fSegments, buf->NumSegments())) {
        EOUT(("Cannot make new references for segments"));
        return false;
     }

   unsigned numsegs = NumSegments() + buf->NumSegments();

   if (numsegs > fCapacity) RellocateSegments(numsegs);

   // copy ids at the end
   for (unsigned n=0;n<buf->NumSegments();n++)
      fSegments[fNumSegments + n] = buf->fSegments[n];

   if (adopt) {
      buf->fNumSegments = 0;
      dabc::Buffer::Release(buf);
   }

   fNumSegments = numsegs;
   return true;
}

dabc::Buffer* dabc::Buffer::MakeReference()
{
   dabc::Buffer* res = 0;

   if (fPool==0) {
      res = new dabc::Buffer::Buffer();
   } else {

     res = fPool->TakeEmptyBuffer(GetHeaderSize());
     if (res && !fPool->NewReferences(fSegments, NumSegments())) {
        EOUT(("Cannot make new references for segments"));
        dabc::Buffer::Release(res);
        res = 0;
     }
   }

   if (res==0) return 0;

   res->RellocateSegments(NumSegments());

   if (NumSegments()>0)
      memcpy(res->fSegments, fSegments, NumSegments() * sizeof(MemSegment));
   res->fNumSegments = NumSegments();

   if (GetHeaderSize()>0) {
      res->SetHeaderSize(GetHeaderSize());
      memcpy(res->GetHeader(), GetHeader(), GetHeaderSize());
   }

   res->SetTypeId(GetTypeId());

   return res;
}

bool dabc::Buffer::CopyFrom(dabc::Buffer* buf)
{
   // copies content and set same length as source buffer
   // Header also will be copied

   if (buf==0) return false;

   BufferSize_t buf_size = buf->GetTotalSize();

   if (buf_size > GetTotalSize()) {
      EOUT(("No place to copy complete buffer"));
      return false;
   }

   Pointer src(buf);
   Pointer tgt(this);

   tgt.copyfrom(src, buf_size);

   if (buf_size < GetTotalSize()) {
      tgt.shift(buf_size);

      while (tgt.fullsize() > 0) {
         fSegments[tgt.segm()].datasize -= tgt.rawsize();
         tgt.shift_to_segment();
      }
   }

   if (buf->GetHeaderSize() > 0) {
      RellocateHeader(buf->GetHeaderSize(), false);
      fHeaderSize = buf->GetHeaderSize();
      memcpy(fHeader, buf->GetHeader(), fHeaderSize);
   } else {
      fHeaderSize = 0;
   }

   SetTypeId(buf->GetTypeId());

   return true;
}
