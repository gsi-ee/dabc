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

#include "verbs/Context.h"

#include <sys/poll.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>


#ifndef  __NO_MULTICAST__
#include "verbs/OpenSM.h"
#else
#include <infiniband/arch.h>
#endif


#include "dabc/timing.h"
#include "dabc/logging.h"
#include "dabc/Manager.h"

int null_gid(union ibv_gid *gid)
{
   return !(gid->raw[8] | gid->raw[9] | gid->raw[10] | gid->raw[11] |
       gid->raw[12] | gid->raw[13] | gid->raw[14] | gid->raw[15]);
}


// *******************************************************************

verbs::PoolRegistry::PoolRegistry(ContextRef ctx, dabc::MemoryPool* pool) :
   dabc::Object(0, pool->GetName()),
   fContext(ctx),
   fPool(pool),
   fLastChangeCounter(0),
   f_nummr(0),
   f_mr(0)
{
   DOUT4(("Create registry for the POOL: %s numref %u", GetName(), NumReferences()));
}

verbs::PoolRegistry::~PoolRegistry()
{
   DOUT4(("~PoolRegistry %s refs %u", GetName(), NumReferences()));

   CleanMRStructure();

   DOUT4(("~PoolRegistry %s refs %u done", GetName(), NumReferences()));
}

void verbs::PoolRegistry::ObjectCleanup()
{
   // TODO: later one can enable this code to get info when pool destroyed too early
   // if (dabc::mgr() && fPool)
   //    dabc::mgr()->UnregisterDependency(this, fPool);

   dabc::Object::ObjectCleanup();
}

void verbs::PoolRegistry::ObjectDestroyed(dabc::Object* obj)
{
   if (obj == fPool) {
      EOUT(("!!!!!!!!! Hard error - memory pool %s destroyed behind the scene", fPool->GetName()));
      CleanMRStructure();
      fPool = 0;
      DeleteThis();
   }
}

void verbs::PoolRegistry::CleanMRStructure()
{
   DOUT3(("CleanMRStructure %s call ibv_dereg_mr %u", GetName(), f_nummr));

   for (unsigned n=0;n<f_nummr;n++)
     if (f_mr[n] != 0) {
        DOUT5(("CleanMRStructure %s mr[%u] = %p", GetName(), n, f_mr[n]));
//        if (strcmp(GetName(),"TransportPool")!=0)
           ibv_dereg_mr(f_mr[n]);
//        else
//           EOUT(("Skip ibv_dereg_mr(f_mr[n])"));
        DOUT5(("CleanMRStructure %s mr[%u] = %p done", GetName(), n, f_mr[n]));
     }

   delete[] f_mr;
   f_mr = 0;
   f_nummr = 0;

//   fLastChangeCounter = 0;
}


void verbs::PoolRegistry::SyncMRStructure()
{
   if (fPool==0) return;

   DOUT5(("CreateMRStructure %s for pool %p numrefs %u", GetName(), fPool, NumReferences()));

   std::vector<void*> bufs;
   std::vector<unsigned> sizes;

   // we perform complete synchronization under pool mutex that concurrent thread do not disturb each other

   // FIXME: in current implementation pool is fixed-layout therefore first transport will produce
   // correct structure which should not be changed until the end
   // but this method cannot be used to update structure if pool changes in between

   {

      dabc::LockGuard lock(fPool->GetPoolMutex());

      if (!fPool->_GetPoolInfo(bufs, sizes, &fLastChangeCounter)) return;

      if (f_nummr!=0) CleanMRStructure();

      DOUT5(("CreateMRStructure %p for pool %p size %u", this, fPool, bufs.size()));

      f_nummr = bufs.size();

      f_mr = new struct ibv_mr* [f_nummr];

      for (unsigned n=0;n<f_nummr;n++) {

         f_mr[n] = ibv_reg_mr(fContext.pd(), bufs[n], sizes[n],
               (ibv_access_flags) (IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE));

         DOUT5(("CreateMRStructure %p for pool %p mr[%u] = %p done", this, fPool, n, f_mr[n]));

         if (f_mr[n]==0) {
            EOUT(("Fail to register VERBS memory - HALT"));
            exit(138);
         }
      }
   }

   DOUT5(("CreateMRStructure %p for pool %p size %u done", this, fPool, bufs.size()));

   // TODO: later one can enable this code to get

//   if (dabc::mgr() && fPool)
//      dabc::mgr()->RegisterDependency(this, fPool);
}

// _________________________________________________________________________

verbs::Context::Context() :
   dabc::Object(0,"VERBS"),
   fIbPort(0),
   fContext(0),
   fPD(0),
   fOsm(0)
{
}


verbs::Context::~Context()
{
   CloseVerbs();

   DOUT0(("VERBS Context destroyed"));
}



bool verbs::Context::OpenVerbs(bool withmulticast, const char* devicename, int ibport)
{
   DOUT4(("Call  verbs::Device::Open"));

#ifndef  __NO_MULTICAST__
   if (withmulticast) {
      fOsm = new OpenSM;
      if (!fOsm->Init()) return false;
   }
#endif

   int num_of_hcas = 0;

   struct ibv_device **dev_list = ibv_get_device_list(&num_of_hcas);

   if ((dev_list==0) || (num_of_hcas<=0)) {
      EOUT(("No verbs devices found"));
      return false;
   }

   DOUT4(( "Number of hcas %d", num_of_hcas));

   struct ibv_device *selected_device = 0;
   uint64_t gid;
   bool res = false;

   if (devicename==0) {
      selected_device = dev_list[0];
      fIbPort = 1;
      devicename = ibv_get_device_name(selected_device);
   } else {
      fIbPort = ibport;
      if (fIbPort<=0) fIbPort = 1;
      for (int n=0;n<num_of_hcas;n++)
         if (strcmp(ibv_get_device_name(dev_list[n]), devicename)==0)
            selected_device = dev_list[n];
      if (selected_device==0) {
         EOUT(("No verbs device with name %s", devicename));
      }
   }

   if (selected_device==0) goto cleanup;

   fContext = ibv_open_device(selected_device);
   if (fContext==0) {
      EOUT(("Cannot open device %s", devicename));
      goto cleanup;
   }

   if (ibv_query_device(fContext, &fDeviceAttr)) {
      EOUT(("Failed to query device props"));
      goto cleanup;
   }

   gid = ibv_get_device_guid(selected_device);
   DOUT4(( "Open device: %s  gid: %016lx",  devicename, ntohll(gid)));

   fPD = ibv_alloc_pd(fContext);
   if (fPD==0) {
      EOUT(("Couldn't allocate protection domain (PD)"));
      goto cleanup;
   }

   if (ibv_query_port(fContext, fIbPort, &fPortAttr)) {
      EOUT(("Fail to query port attributes"));
      goto cleanup;
   }

//   PrintDevicesList(true);

   DOUT4(("Call new TVerbsConnMgr(this) done"));

   res = true;

cleanup:

   ibv_free_device_list(dev_list);

   return res;

}

bool verbs::Context::CloseVerbs()
{
   if (fContext==0) return true;

   bool res = true;

   if (ibv_dealloc_pd(fPD)) {
      EOUT(("Fail to deallocate PD"));
      res = false;
   }
   if (ibv_close_device(fContext)) {
      EOUT(("Fail to close device context"));
      res = false;
   }

   fPD = 0;
   fContext = 0;

#ifndef  __NO_MULTICAST__
   if (fOsm!=0) {
      fOsm->Close();
      delete fOsm;
      fOsm = 0;
   }
#endif

   return res;
}


bool verbs::ContextRef::OpenVerbs(bool withmulticast, const char* devicename, int ibport)
{
   if (GetObject()) return true;

   Context* ctx = new Context;

   if (!ctx->OpenVerbs(withmulticast, devicename, ibport)) {
      delete ctx;
      return false;
   }

   SetObject(ctx, true);
   SetTransient(false);
   return true;
}


int verbs::ContextRef::ManageMulticast(int action, ibv_gid& mgid, uint16_t& mlid)
{
   // Use MulticastActions for
   // action = mcst_Error - do nothing    return mcst_Error
   // action = mcst_Ok - do nothing       return mcst_Ok
   // action = mcst_Register - register,  return mcst_Unregister/mcst_Error
   // action = mcst_Query - query         return mcst_Ok/mcst_Error
   // action = mcst_Init - query or reg   return mcst_Ok/mcst_Unregister/mcst_Error
   // action = mcst_Unregister - unreg    return mcst_Ok/mcst_Error

#ifndef  __NO_MULTICAST__

   OpenSM osm = GetObject() ? GetObject()->fOsm : 0;

   if (osm==0) return 0;
   switch (action) {
      case mcst_Error: return mcst_Error;
      case mcst_Ok: return mcst_Ok;
      case mcst_Register:
         mlid = 0;
         if (osm->ManageMultiCastGroup(true, mgid.raw, &mlid)) return mcst_Unregister;
         return mcst_Error;
      case mcst_Query:
         mlid = 0;
         if (osm->QueryMyltucastGroup(mgid.raw, mlid)) return mcst_Ok;
         return mcst_Error;
      case mcst_Init:
         mlid = 0;
         if (osm->QueryMyltucastGroup(mgid.raw, mlid))
            if (mlid!=0) return mcst_Ok;
         if (osm->ManageMultiCastGroup(true, mgid.raw, &mlid)) return mcst_Unregister;
         return mcst_Error;
      case mcst_Unregister:
         if (osm->ManageMultiCastGroup(false, mgid.raw, &mlid)) return mcst_Ok;
         return mcst_Error;
   }
#endif

   return mcst_Error;
}

struct ibv_ah* verbs::ContextRef::CreateAH(uint32_t dest_lid)
{
   ibv_ah_attr ah_attr;
   memset(&ah_attr, 0, sizeof(ah_attr));
   ah_attr.is_global  = 0;
   ah_attr.dlid       = dest_lid;
   ah_attr.sl         = 0;
   ah_attr.src_path_bits = 0;
   ah_attr.port_num   = IbPort(); // !!!!!!!  probably, here should be destination port

   ibv_ah *ah = ibv_create_ah(pd(), &ah_attr);
   if (ah==0) {
      EOUT(("Failed to create Address Handle"));
   }
   return ah;
}

struct ibv_ah* verbs::ContextRef::CreateMAH(ibv_gid& mgid, uint32_t mlid)
{
   ibv_ah_attr mah_attr;
   memset(&mah_attr, 0, sizeof(ibv_ah_attr));

   mah_attr.dlid = mlid; // in host order ?
   mah_attr.port_num = IbPort();
   mah_attr.sl = 0;
   mah_attr.static_rate = 0; //0x83; // should be copied from member rec

   mah_attr.is_global  = 1;
   memcpy(&(mah_attr.grh.dgid), &mgid, sizeof(ibv_gid));
   mah_attr.grh.sgid_index = 0; // GetGidIndex(mgid);

   mah_attr.grh.flow_label = 0; // should be copied from member rec
   mah_attr.grh.hop_limit = 63; // should be copied from member rec
   mah_attr.grh.traffic_class = 0; // should be copied from member rec

   //DOUT1(("Addr %02x %02x", ah_attr.grh.dgid.raw[0], ah_attr.grh.dgid.raw[1]));

   struct ibv_ah* f_ah = ibv_create_ah(pd(), &mah_attr);
   if (f_ah==0) {
     EOUT(("Failed to create Multicast Address Handle"));
   }

   return f_ah;
}


dabc::Reference verbs::ContextRef::RegisterPool(dabc::MemoryPool* pool)
{
   if (pool==0) return 0;

   dabc::Reference folder = GetFolder("PoolReg", true, true);

   PoolRegistryRef ref = folder.PutChild(new PoolRegistry(Ref(), pool));

   if (ref.null()) {
      EOUT(("Error - cannot create pool registry object for pool %s", pool->GetName()));
      return 0;
   }

   if (ref()->GetPool() != pool) {
      EOUT(("Registry entry for name %s exists but pool pointer mismatch", pool->GetName()));

      exit(543);
   }

   ref()->SyncMRStructure();

   return ref;
}

int verbs::ContextRef::GetGidIndex(ibv_gid* lookgid)
{
   ibv_gid gid;
   int ret = 0;

   DOUT5(( "Search for gid in table: %016lx : %016lx  ",
             ntohll(lookgid->global.subnet_prefix),
             ntohll(lookgid->global.interface_id)));

   for (int i = 0; !ret; i++) {
      ret = ibv_query_gid(context(), IbPort(), i, &gid);

        if (ret) break;

        if (null_gid(&gid)) continue;

        DOUT5(( "   gid[%2d]: %016lx : %016lx  ", i,
             ntohll(gid.global.subnet_prefix),
             ntohll(gid.global.interface_id)));

        if (!ret && !memcmp(lookgid, &gid, sizeof(ibv_gid))) return i;
   }
   return 0;
}

