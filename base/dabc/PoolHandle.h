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

#ifndef DABC_PoolHandle
#define DABC_PoolHandle

#ifndef DABC_ModuleItem
#include "dabc/ModuleItem.h"
#endif

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

namespace dabc {

   class Port;
   class Module;

   class PoolHandle : public ModuleItem,
                      public MemoryPoolRequester {

      friend class Module;
      friend class Port;

      private:
         PoolHandle(Reference parent, const char* name, Reference pool);
         virtual ~PoolHandle();

      protected:

         /** \brief Inherited method, should cleanup all references and object itself */
         virtual void ObjectCleanup();

         /** \brief Inherited method, should check if pool is destroying */
         virtual void ObjectDestroyed(Object*);

         Reference GetPoolRef();

      public:

         virtual const char* ClassName() const { return clPoolHandle; }

         inline MemoryPool* Pool() const { return fPool(); }

         inline Buffer TakeEmptyBuffer()
         {
            return Pool()->TakeEmpty();
         }

         inline Buffer TakeBuffer(BufferSize_t size = 0)
         {
            return Pool()->TakeBuffer(size);
         }

         inline Buffer TakeBufferReq(BufferSize_t size = 0)
         {
            return Pool()->TakeBufferReq(this, size);
         }

         inline Buffer TakeRequestedBuffer()
         {
            return MemoryPoolRequester::TakeRequestedBuffer();
         }

         bool ProcessPoolRequest();

         double UsedRatio() const;

         void SetUsageParameter(const std::string& parname, double interval = 1.);

         virtual bool Find(ConfigIO &cfg);

      protected:

         virtual void ProcessEvent(const EventId&);

         virtual double ProcessTimeout(double);

         MemoryPoolRef     fPool;

         std::string       fUsagePar; // !< parameter name, used to display memory pool usage
         double            fUpdateTimeout;
   };
}

#endif
