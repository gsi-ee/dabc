#include "dabc/BufferNew.h"

#include "dabc/Exception.h"
#include "dabc/MemoryPool.h"
#include "dabc/Pointer.h"


namespace dabc {


   /** Helper class to release memory, allocated independently from memory pool
    * Object will be deleted (with memory) when last reference in buffer disappear */
   class MemoryContainer : public Object {
      public:

         void* fPtr;

         MemoryContainer(void *ptr) :
            Object(0, "", flAutoDestroy),
            fPtr(ptr)
         {
         }

         virtual ~MemoryContainer()
         {
            free(fPtr); fPtr = 0;
            DOUT0("Destroy custom container");
         }
   };
}


dabc::BufferContainer::~BufferContainer()
{
   dabc::MemoryPool* pool = dynamic_cast<dabc::MemoryPool*> (fPool());

   if (pool) {
      pool->DecreaseSegmRefs(fSegm, fNumSegments);
   } else {

   }

   fPool.Release();
}


// ========================================================


dabc::Reference dabc::BufferNew::GetPool() const
{
   return PoolPtr();
}

dabc::MemoryPool* dabc::BufferNew::PoolPtr() const
{
   if (null()) return 0;

   return dynamic_cast<MemoryPool*> (GetObject()->fPool());
}


dabc::BufferSize_t dabc::BufferNew::GetTotalSize() const
{
   BufferSize_t sz = 0;
   for (unsigned n=0;n<NumSegments();n++)
      sz += SegmentSize(n);
   return sz;
}

void dabc::BufferNew::SetTotalSize(BufferSize_t len) throw()
{
   if (len==0) {
      Release();
      return;
   }

   if (null()) return;

   BufferSize_t totalsize = GetTotalSize();
   if (len > totalsize)
      throw dabc::Exception(dabc::ex_Buffer, "Cannot extend size of the buffer by SetTotalSize method, use Append() method instead", "Buffer");

   if (len == totalsize) return;

   unsigned nseg(0), npos(0);
   Locate(len, nseg, npos);

   if (nseg >= NumSegments())
      throw dabc::Exception(dabc::ex_Buffer, "FATAL nseg>=NumSegments", dabc::format("Buffer numseg %u", NumSegments()));

   if (npos>0) {
      GetObject()->fSegm[nseg].datasize = npos;
      nseg++;
   }

   // we should release peaces which are no longer required

   if (nseg<NumSegments()) {

      dabc::MemoryPool* pool = (dabc::MemoryPool*) GetObject()->fPool();

      if (pool)
         pool->DecreaseSegmRefs(Segments()+nseg, NumSegments() - nseg);

      GetObject()->fNumSegments = nseg;
   }
}



void dabc::BufferNew::CutFromBegin(BufferSize_t len) throw()
{
   if (len==0) return;

   if (len>=GetTotalSize()) {
      Release();
      return;
   }

   unsigned nseg(0), npos(0);
   Locate(len, nseg, npos);

   if (nseg >= NumSegments())
      throw dabc::Exception(dabc::ex_Buffer, "FATAL nseg >= NumSegments()", dabc::format("Buffer numseg %u", NumSegments()));

   // we should release segments which are no longer required
   if (nseg>0) {

      if (PoolPtr()) PoolPtr()->DecreaseSegmRefs(Segments(), nseg);

      for (unsigned n=0;n<NumSegments()-nseg;n++)
         GetObject()->fSegm[n].copy_from(GetObject()->fSegm + n + nseg);

      GetObject()->fNumSegments -= nseg;
   }

   if (npos>0) {
      GetObject()->fSegm[0].datasize -= npos;
      GetObject()->fSegm[0].buffer = (char*) (GetObject()->fSegm[0].buffer) + npos;
   }
}


void dabc::BufferNew::Locate(BufferSize_t p, unsigned& seg_indx, unsigned& seg_shift) const
{
   seg_indx = 0;
   seg_shift = 0;

   BufferSize_t curr(0);

   while ((curr < p) && (seg_indx<NumSegments())) {
      if (curr + SegmentSize(seg_indx) <= p) {
         curr += SegmentSize(seg_indx);
         seg_indx++;
         continue;
      }

      seg_shift = p - curr;
      return;
   }
}


dabc::BufferNew dabc::BufferNew::Duplicate() const
{
   dabc::BufferNew res;

   if (null()) return res;

   if (PoolPtr()) PoolPtr()->IncreaseSegmRefs(Segments(), NumSegments());

   res.AllocateContainer(GetObject()->fCapacity);

   res.GetObject()->fPool = GetObject()->fPool.Ref();

   res.GetObject()->fNumSegments = NumSegments();

   for (unsigned n=0;n<NumSegments();n++)
      res.GetObject()->fSegm[n].copy_from(GetObject()->fSegm + n);

   return res;
}


bool dabc::BufferNew::Append(BufferNew& src, bool moverefs) throw()
{
   return Insert(GetTotalSize(), src, moverefs);
}

bool dabc::BufferNew::Prepend(BufferNew& src, bool moverefs) throw()
{
   return Insert(0, src, moverefs);
}

bool dabc::BufferNew::Insert(BufferSize_t pos, BufferNew& src, bool moverefs) throw()
{
   if (src.null()) return true;

   if (null()) {
      if (moverefs)
         *this = src.Take();
      else
         *this = src.Duplicate();
      return true;
   }

   BufferNew ownbuf;

   // if we have buffer assigned to the pool and its differ from
   // source object we need deep copy to be able extend refs array
   if ((PoolPtr()!=0) && (src.PoolPtr()!=PoolPtr())) {
      // ownbuf = PoolPtr()->CopyBufferNew(src); // TODO TODO TODO
   } else
   if (!moverefs)
      ownbuf = src.Duplicate();
   else
      ownbuf = src;


   unsigned tgtseg(0), tgtpos(0);

   Locate(pos, tgtseg, tgtpos);

   unsigned numrequired = NumSegments() + ownbuf.NumSegments();
   if (tgtpos>0) numrequired++;

   if (numrequired > GetObject()->fCapacity)
      throw dabc::Exception(dabc::ex_Buffer, "Required number of segments less than available in the buffer", dabc::format("Buffer numseg %u", NumSegments()));

   MemSegment* segm = Segments();

   // first move complete segments to the end
   for (unsigned n=NumSegments(); n>tgtseg + (tgtpos>0 ? 1 : 0); ) {
      n--;

      // DOUT0("Move segm %u->%u", n, n + numrequired - NumSegments());

      segm[n + numrequired - NumSegments()] = segm[n];
      segm[n].datasize = 0;
      segm[n].id = 0;
      segm[n].buffer = 0;
   }

   MemSegment* srcsegm = ownbuf.Segments();
   // copy all segments from external buffer
   for (unsigned n=0;n<ownbuf.NumSegments();n++) {
      // DOUT0("copy segm src[%u]->tgt[%u]", n, tgtseg + n + (tgtpos>0 ? 1 : 0));
      segm[tgtseg + n + (tgtpos>0 ? 1 : 0)].copy_from(&(srcsegm[n]));
   }

   // in case when segment is divided on two parts
   if (tgtpos>0) {
      if(PoolPtr()) PoolPtr()->IncreaseSegmRefs(segm + tgtseg, 1);

      unsigned seg2 = tgtseg + ownbuf.NumSegments() + 1;

      segm[seg2].id = segm[tgtseg].id;
      segm[seg2].datasize = segm[tgtseg].datasize - tgtpos;
      segm[seg2].buffer = (char*) segm[tgtseg].buffer + tgtpos;

      segm[tgtseg].datasize = tgtpos;

      DOUT0("split segment %u on two parts, second is in %u", tgtseg, seg2);
   }

   // at the end
   GetObject()->fNumSegments = numrequired;

   if (PoolPtr() && (ownbuf.PoolPtr() == PoolPtr())) {
      // forget about all segments - they are moved to target
      ownbuf.GetObject()->fNumSegments = 0;
      ownbuf.Release();
   }

   if (moverefs) src.Release();

   return true;
}


std::string dabc::BufferNew::AsStdString()
{
   std::string sbuf;

   if (null()) return sbuf;

   DOUT0("Num segments = %u", NumSegments());

   for (unsigned nseg=0; nseg<NumSegments(); nseg++) {
      DOUT0("Segm %u = %p %u", nseg, SegmentPtr(nseg), SegmentSize(nseg));
      sbuf.append((const char*)SegmentPtr(nseg), SegmentSize(nseg));
   }

   return sbuf;
}

dabc::BufferSize_t dabc::BufferNew::CopyFrom(const BufferNew& srcbuf, BufferSize_t len) throw()
{
//   return Pointer(*this).copyfrom(Pointer(srcbuf), len);
}


dabc::BufferSize_t dabc::BufferNew::CopyFromStr(const char* src, unsigned len) throw()
{
//   return Pointer(*this).copyfromstr(src, len);
}

dabc::BufferSize_t dabc::BufferNew::CopyTo(void* ptr, BufferSize_t len) const throw()
{
//   return Pointer(*this).copyto(ptr, len);
}

dabc::BufferNew dabc::BufferNew::GetNextPart(Pointer& ptr, BufferSize_t len, bool allowsegmented) throw()
{
   dabc::BufferNew res;

   if (ptr.fullsize()<len) return res;

   while (!allowsegmented && (len > ptr.rawsize())) {
      ptr.shift(ptr.rawsize());
      if (ptr.fullsize() < len) break;
   }

   if (ptr.fullsize() < len) return res;

   unsigned firstseg(0), lastseg(0);
   void* firstptr(0);
   BufferSize_t firstlen(0), lastlen(0);

   while ((len>0) && (ptr.fSegm<NumSegments())) {
      unsigned partlen(len);
      if (partlen>ptr.rawsize()) partlen = ptr.rawsize();
      if (partlen==0) break;

      if (firstptr==0) {
         firstptr = ptr.ptr();
         firstlen = partlen;
         firstseg = ptr.fSegm;
      }

      lastseg = ptr.fSegm;
      lastlen = partlen;

      len -= partlen;
      ptr.shift(partlen);
   }

   if (len>0) {
      EOUT("Internal problem - not full length covered");
      return res;
   }

   res.AllocateContainer(GetObject()->fCapacity);

   if (PoolPtr()) {
      PoolPtr()->IncreaseSegmRefs(Segments() + firstseg, lastseg-firstseg+1);

      res.GetObject()->fPool = GetObject()->fPool.Ref();
   }

   for (unsigned n=firstseg;n<=lastseg;n++) {
      res.Segments()[n-firstseg].copy_from(Segments() + n);
      if (n==firstseg) {
         res.Segments()->buffer = firstptr;
         res.Segments()->datasize = firstlen;
      }
      if (n==lastseg)
         res.Segments()[n-firstseg].datasize = lastlen;
   }

   return res;
}



dabc::BufferNew dabc::BufferNew::CreateBuffer(BufferSize_t sz) throw()
{
    return CreateBuffer(malloc(sz), sz, true);
}

dabc::BufferNew dabc::BufferNew::CreateBuffer(const void* ptr, unsigned size, bool owner) throw()
{
   dabc::BufferNew res;

   res.AllocateContainer(8);

   if (owner) {
      MemoryContainer* mem = new MemoryContainer((void*)ptr);
      res.GetObject()->fPool = mem;
   }

   res.GetObject()->fNumSegments = 1;
   res.GetObject()->fSegm[0].buffer = (void*) ptr;
   res.GetObject()->fSegm[0].datasize = size;

   return res;
}


void dabc::BufferNew::AllocateContainer(unsigned capacity)
{
   Release();

   unsigned obj_size = sizeof(BufferContainer) + sizeof(MemSegment) * capacity;

   void* area = malloc(obj_size);

   BufferContainer* cont = new (area) BufferContainer;

   cont->fCapacity = capacity;
   cont->fSegm = (MemSegment*) (char*) area + sizeof(BufferContainer);

   memset(cont->fSegm, 0, sizeof(MemSegment) * capacity);

   *this = cont;
}
