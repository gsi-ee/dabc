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

#ifndef VERBS_Context
#define VERBS_Context

#include <infiniband/verbs.h>

#ifndef DABC_Object
#include "dabc/Object.h"
#endif

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif


namespace verbs {

   class OpenSM;
   class ContextRef;

   extern const char *typeThread;
   extern const char *typeDevice;

   // ________________________________________________________


   /** \brief Context for all VERBS operations
    *
    * Class keeps verbs structures like context, protection domain, port and so on */

   class Context : public dabc::Object  {

      friend class ContextRef;

      protected:
         uint8_t fIbPort{0};  // infiniband port number

         struct ibv_context *fContext{nullptr};         // device context
         struct ibv_pd *fPD{nullptr};                   // protection domain
         struct ibv_device_attr fDeviceAttr;  // device attributes
         struct ibv_port_attr fPortAttr;      // port attributes

         OpenSM      *fOsm{nullptr}; // osm object created when multicast is required

         static bool fThreadSafeVerbs;  // identifies if verbs is thread safe

         Context();

         bool CloseVerbs();

         bool OpenVerbs(bool withmulticast = false, const char *devicename = nullptr, int ibport = -1);

      public:

         virtual ~Context();
   };

   // _______________________________________________________________

   /** \brief %Reference to \ref verbs::Context */

   class ContextRef : public dabc::Reference {

      DABC_REFERENCE(ContextRef, dabc::Reference, Context)

      public:

         enum MulticastActions { mcst_Error = 0, mcst_Ok, mcst_Register, mcst_Query, mcst_Init, mcst_Unregister };

         bool OpenVerbs(bool withmulticast = false, const char *devicename = nullptr, int ibport = -1);

         int IbPort() const { return GetObject() ? GetObject()->fIbPort : 0; }
         bool IsMulticast() const { return GetObject() ? GetObject()->fOsm != nullptr : false; }
         struct ibv_pd *pd() const { return GetObject()->fPD; }
         struct ibv_context *context() const { return GetObject()->fContext; }
         uint16_t lid() const { return GetObject()->fPortAttr.lid; }
         ibv_mtu mtu() const { return GetObject()->fPortAttr.active_mtu; }
         unsigned max_sge() const { return GetObject()->fDeviceAttr.max_sge - 1; }

         dabc::Reference RegisterPool(dabc::MemoryPool* pool);

         int GetGidIndex(ibv_gid* lookgid);

         struct ibv_ah *CreateAH(uint32_t dest_lid);

         int ManageMulticast(int action, ibv_gid &mgid, uint16_t &mlid);
         struct ibv_ah *CreateMAH(ibv_gid& mgid, uint32_t mlid);
   };

   // ________________________________________________________________________

   /** \brief Registry object for memory pool */

   class PoolRegistry : public dabc::Object {
      protected:
         ContextRef         fContext;
         dabc::MemoryPool  *fPool{nullptr};
         unsigned           fLastChangeCounter{0};
         unsigned           f_nummr{0};     // size of f_mr array
         struct ibv_mr*    *f_mr{nullptr};

         void ObjectCleanup() override;

      public:
         PoolRegistry(ContextRef ctx, dabc::MemoryPool* pool);
         virtual ~PoolRegistry();

         dabc::MemoryPool* GetPool() const { return fPool; }

         inline void CheckMRStructure()
         {
            if (fPool && fPool->CheckChangeCounter(fLastChangeCounter)) {
               EOUT("POOL %p change counter now = %u", fPool, fLastChangeCounter);
               throw dabc::Exception("Memory pool structure should not be changed at all");
            }
         }

         inline uint32_t GetLkey(unsigned id) { return id < f_nummr ? f_mr[id]->lkey : 0; }

         void ObjectDestroyed(dabc::Object *obj) override;

         void SyncMRStructure();

         void CleanMRStructure();
   };

   // ______________________________________________________

   /** \brief %Reference on \ref verbs::PoolRegistry */

   class PoolRegistryRef : public dabc::Reference {

      DABC_REFERENCE(PoolRegistryRef, dabc::Reference, verbs::PoolRegistry)

   };

}

#endif
