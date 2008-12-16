#ifndef DABC_PoolHandle
#define DABC_PoolHandle

#ifndef DABC_ModuleItem
#include "dabc/ModuleItem.h"
#endif

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

namespace dabc {

   class DoubleParameter;

   class PoolHandle : public ModuleItem,
                      public MemoryPoolRequester {
      friend class Module;

      private:
         PoolHandle(Basic* parent, const char* name, MemoryPool* pool,
                    BufferSize_t size, BufferNum_t number, BufferNum_t increment);
         virtual ~PoolHandle();

      public:

         virtual const char* ClassName() const { return clPoolHandle; }

         BufferSize_t GetRequiredBufferSize() const { return fRequiredSize; }

         MemoryPool* getPool() const { return fPool; }

         inline Buffer* TakeEmptyBuffer(BufferSize_t hdrsize = 0)
         {
            return fPool->TakeEmptyBuffer(hdrsize);
         }

         inline Buffer* TakeBuffer(BufferSize_t size = 0, BufferSize_t hdrsize = 0)
         {
            return fPool->TakeBuffer(size ? size : fRequiredSize, hdrsize);
         }

         inline Buffer* TakeBufferReq(BufferSize_t size = 0, BufferSize_t hdrsize = 0)
         {
            return fPool->TakeBufferReq(this, size ? size : fRequiredSize, hdrsize);
         }

         inline Buffer* TakeRequestedBuffer()
         {
            return fPool->TakeRequestedBuffer(this);
         }

         bool ProcessPoolRequest();

         double UsedRatio() const;

         void SetUsageParameter(Parameter* par, double interval = 1.);

         virtual bool Store(ConfigIO &cfg);
         virtual bool Find(ConfigIO &cfg);

      protected:

         virtual void ProcessEvent(EventId evid);

         virtual double ProcessTimeout(double);

         MemoryPool       *fPool;

         BufferNum_t       fRequiredNumber;
         BufferNum_t       fRequiredIncrement;
         BufferSize_t      fRequiredSize;
         BufferSize_t      fRequiredHeaderSize;

         DoubleParameter  *fUsagePar;
         double            fUpdateTimeout;

         bool              fStoreHandle;
   };

}

#endif
