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
         PoolHandle(Basic* parent, const char* name, BufferNum_t number, BufferNum_t increment, BufferSize_t size);
         virtual ~PoolHandle();

      public:
         /** sets how many buffers required for the envelope */
         void AddPortRequirements(BufferNum_t number, BufferSize_t headersize);

         bool IsPoolAssigned() const { return fPool!=0; }

         BufferNum_t GetRequiredBuffersNumber() const { return fRequiredNumber; }
         BufferNum_t GetRequiredIncrement() const { return fRequiredIncrement; }
         BufferSize_t GetRequiredBufferSize() const { return fRequiredSize; }
         BufferSize_t GetRequiredHeaderSize() const { return fRequiredHeaderSize; }

         void AssignPool(MemoryPool *pool);
         void DisconnectPool();

         MemoryPool* getPool() const { return fPool; }

         Buffer* TakeEmptyBuffer(BufferSize_t hdrsize = 0);
         Buffer* TakeBuffer(BufferSize_t size, bool withrequest);
         Buffer* TakeRequestedBuffer();

         bool ProcessPoolRequest();

         double UsedRatio() const;

         void SetUsageParameter(Parameter* par, double interval = 1.);

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
   };

}

#endif
