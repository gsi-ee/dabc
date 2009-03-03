/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#include "verbs/Device.h"

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

#include "dabc/MemoryPool.h"
#include "dabc/Manager.h"
#include "dabc/Port.h"
#include "dabc/Factory.h"

#include "verbs/ComplQueue.h"
#include "verbs/QueuePair.h"
#include "verbs/Thread.h"
#include "verbs/Transport.h"

const int LoopBackQueueSize = 8;
const int LoopBackBufferSize = 64;

int null_gid(union ibv_gid *gid)
{
   return !(gid->raw[8] | gid->raw[9] | gid->raw[10] | gid->raw[11] |
       gid->raw[12] | gid->raw[13] | gid->raw[14] | gid->raw[15]);
}

// this boolean indicates if one can use verbs calls from different threads
// if no, all post/recv/complition operation for all QP/CQ will happens in the same thread

bool verbs::Device::fThreadSafeVerbs = true;

namespace verbs {
   class VerbsFactory : public dabc::Factory {
      public:
         VerbsFactory(const char* name) : dabc::Factory(name) {}

         virtual dabc::Device* CreateDevice(const char* classname,
                                            const char* devname,
                                            dabc::Command*);

         virtual dabc::WorkingThread* CreateThread(const char* classname, const char* thrdname, const char* thrddev, dabc::Command* cmd);
   };


   VerbsFactory verbsfactory("verbs");
}

dabc::Device* verbs::VerbsFactory::CreateDevice(const char* classname,
                                                const char* devname,
                                                dabc::Command*)
{
	if (strcmp(classname, "verbs::Device")!=0) return 0;

	return new Device(dabc::mgr()->GetDevicesFolder(true), devname);
}

dabc::WorkingThread* verbs::VerbsFactory::CreateThread(const char* classname, const char* thrdname, const char* thrddev, dabc::Command* cmd)
{
   if ((classname==0) || (strcmp(classname, VERBS_THRD_CLASSNAME)!=0)) return 0;

   if (thrddev==0) {
      EOUT(("Device name not specified to create verbs thread"));
      return 0;
   }

   verbs::Device* dev = dynamic_cast<verbs::Device*> (dabc::mgr()->FindDevice(thrddev));
   if (dev==0) {
      EOUT(("Did not found verbs device with name %s", thrddev));
      return 0;
   }

   return new verbs::Thread(dev, dabc::mgr()->GetThreadsFolder(true), thrdname);
}

// *******************************************************************

verbs::PoolRegistry::PoolRegistry(Device* verbs, dabc::MemoryPool* pool, bool local) :
   dabc::Basic(local ? 0 : verbs->GetPoolRegFolder(true), pool->GetName()),
   fVerbs(verbs),
   fPool(pool),
   fWorkMutex(0),
   fUsage(0),
   fLastChangeCounter(0),
   f_nummr(0),
   f_mr(0),
   fBlockChanged(0)
{
   if (dabc::mgr() && fPool)
     dabc::mgr()->RegisterDependency(this, fPool);

   dabc::LockGuard lock(fPool->GetPoolMutex());

   // we only need to lock again pool mutex when pool can be potentially changed
   // otherwise structure, created once, can be used forever
   if (!fPool->IsMemLayoutFixed()) fWorkMutex = fPool->GetPoolMutex();

   _UpdateMRStructure();
}

verbs::PoolRegistry::~PoolRegistry()
{
   DOUT3(("~PoolRegistry %s", GetName()));

   if (dabc::mgr() && fPool)
      dabc::mgr()->UnregisterDependency(this, fPool);

   CleanMRStructure();

   DOUT3(("~PoolRegistry %s done", GetName()));

}

void verbs::PoolRegistry::DependendDestroyed(dabc::Basic* obj)
{
   if (obj == fPool) {
//      EOUT(("!!!!!!!!! Hard error - memory pool %s destroyed behind the scene", fPool->GetName()));
      CleanMRStructure();
      fPool = 0;
      fWorkMutex = 0;
      DeleteThis();
   }
}

void verbs::PoolRegistry::CleanMRStructure()
{
   DOUT3(("CleanMRStructure %s call ibv_dereg_mr %u", GetName(), f_nummr));

   for (unsigned n=0;n<f_nummr;n++)
     if (f_mr[n] != 0) {
        DOUT3(("CleanMRStructure %s mr[%u] = %p", GetName(), n, f_mr[n]));
//        if (strcmp(GetName(),"TransportPool")!=0)
           ibv_dereg_mr(f_mr[n]);
//        else
//           EOUT(("Skip ibv_dereg_mr(f_mr[n])"));
        DOUT3(("CleanMRStructure %s mr[%u] = %p done", GetName(), n, f_mr[n]));
     }

   delete[] fBlockChanged; fBlockChanged = 0;

   delete[] f_mr;
   f_mr = 0;
   f_nummr = 0;

   fLastChangeCounter = 0;
}


void verbs::PoolRegistry::_UpdateMRStructure()
{
   if (fPool==0) return;

   if (f_nummr < fPool->NumMemBlocks()) {
      // this is a case, when pool size was extended

      struct ibv_mr* *new_mr = new struct ibv_mr* [fPool->NumMemBlocks()];

      unsigned *new_changed = new unsigned [fPool->NumMemBlocks()];

      for (unsigned n=0;n<fPool->NumMemBlocks();n++) {
         new_mr[n] = 0;
         new_changed[n] = 0;
         if (n<f_nummr) {
             new_mr[n] = f_mr[n];
             new_changed[n] = fBlockChanged[n];
         }
      }

      delete[] f_mr;
      delete[] fBlockChanged;

      f_mr = new_mr;
      fBlockChanged = new_changed;

   } else
   if (f_nummr > fPool->NumMemBlocks()) {
      //  this is a case, when pool size was reduced
      // first, cleanup no longer avaliable memory regions
      for (unsigned n=fPool->NumMemBlocks(); n<f_nummr; n++) {
         if (f_mr[n]) {
            ibv_dereg_mr(f_mr[n]);
            f_mr[n] = 0;
         }
         fBlockChanged[n] = 0;
      }
   }

   f_nummr = fPool->NumMemBlocks();

   for (unsigned n=0;n<f_nummr;n++)
      if ( fPool->IsMemoryBlock(n) &&
          ((f_mr[n]==0) || (fBlockChanged[n] != fPool->MemBlockChangeCounter(n))) ) {
              if (f_mr[n]!=0) {
                 ibv_dereg_mr(f_mr[n]);
                 f_mr[n] = 0;
              }

              if ((fPool->GetMemBlock(n)==0) || (fPool->GetMemBlockSize(n)==0)) continue;

              f_mr[n] = ibv_reg_mr(fVerbs->pd(),
                                   fPool->GetMemBlock(n),
                                   fPool->GetMemBlockSize(n),
                                   IBV_ACCESS_LOCAL_WRITE);

//                                   (ibv_access_flags) (IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE));

              DOUT3(("New PoolRegistry %s mr[%u] = %p done", GetName(), n, f_mr[n]));

              if (f_mr[n]==0) {
                 EOUT(("Fail to registry VERBS memory - HALT"));
                 exit(1);
              }
              fBlockChanged[n] = fPool->MemBlockChangeCounter(n);
          } else

          if (!fPool->IsMemoryBlock(n)) {
             if (f_mr[n]!=0) ibv_dereg_mr(f_mr[n]);
             f_mr[n] = 0;
             fBlockChanged[n] = 0;
          }

   fLastChangeCounter = fPool->GetChangeCounter();
}

// ____________________________________________________________________

verbs::Device::Device(dabc::Basic* parent, const char* name) :
   dabc::Device(parent, name),
   fIbPort(0),
   fContext(0),
   fPD(0),
   f_mtu(2048),
   fOsm(0),
   fAllocateIndividualCQ(false)
{
   DOUT3(("Creating VERBS device %s", name));

   if (!OpenVerbs(true)) {
      EOUT(("FATAL. Cannot start VERBS device"));
      exit(1);
   }

   DOUT3(("Creating VERBS thread %s", GetName()));

   Thread* thrd = MakeThread(GetName(), true);

   DOUT3(("Assign VERBS device to thread %s", GetName()));

   AssignProcessorToThread(thrd);

   DOUT3(("Creating VERBS device %s done", name));

}

verbs::Device::~Device()
{
   DOUT5(("Start Device::~Device()"));

   // one should call method here, while thread(s) must be deleted
   // before CQ, Channel, PD will be destroyed
   CleanupDevice(true);

   DOUT5(("Cleanup verbs device done"));

   RemoveProcessorFromThread(true);

   dabc::Folder* f = dabc::mgr()->GetThreadsFolder();
   if (f) {
      for (int n=f->NumChilds()-1; n>=0; n--) {
         Thread* thrd = dynamic_cast<Thread*> (f->GetChild(n));
         if (thrd && (thrd->GetDevice()==this))
            if (!thrd->IsItself()) {
               if (thrd==ProcessorThread()) EOUT(("AAAAAAAAAAAAAAAA BBBBBBBBBBBBBBBBBBB"));
               delete thrd;
            } else {
               thrd->CloseThread();
               dabc::mgr()->DestroyObject(thrd);
            }
      }
   }

   DOUT5(("DestroyRegisteredThreads done"));

   CloseVerbs();

   DOUT5(("Stop verbs::Device::~Device()"));
}

bool verbs::Device::OpenVerbs(bool withmulticast, const char* devicename, int ibport)
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

bool verbs::Device::CloseVerbs()
{
   if (fContext==0) return true;

   bool res = true;

   dabc::Folder* fold = GetPoolRegFolder(false);
   if (fold!=0) fold->DeleteChilds();

   if (ibv_dealloc_pd(fPD)) {
      EOUT(("Fail to deallocate PD"));
      res = false;
   }
   if (ibv_close_device(fContext)) {
      EOUT(("Fail to close device context"));
      res = false;
   }

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

int verbs::Device::GetGidIndex(ibv_gid* lookgid)
{
   ibv_gid gid;
   int ret = 0;

   DOUT5(( "Serach for gid in table: %016lx : %016lx  ",
             ntohll(lookgid->global.subnet_prefix),
             ntohll(lookgid->global.interface_id)));

   for (int i = 0; !ret; i++) {
      ret = ibv_query_gid(fContext, fIbPort, i, &gid);

        if (ret) break;

        if (null_gid(&gid)) continue;

        DOUT5(( "   gid[%2d]: %016lx : %016lx  ", i,
             ntohll(gid.global.subnet_prefix),
             ntohll(gid.global.interface_id)));

        if (!ret && !memcmp(lookgid, &gid, sizeof(ibv_gid))) return i;
   }
   return 0;
}

struct ibv_ah* verbs::Device::CreateAH(uint32_t dest_lid, int dest_port)
{
   ibv_ah_attr ah_attr;
   memset(&ah_attr, 0, sizeof(ah_attr));
   ah_attr.is_global  = 0;
   ah_attr.dlid       = dest_lid;
   ah_attr.sl         = 0;
   ah_attr.src_path_bits = 0;
   ah_attr.port_num   = dest_port>=0 ? dest_port : IbPort(); // !!!!!!!  probably, here should be destination port

   ibv_ah *ah = ibv_create_ah(pd(), &ah_attr);
   if (ah==0) {
      EOUT(("Failed to create Address Handle"));
   }
   return ah;
}

bool verbs::Device::RegisterMultiCastGroup(ibv_gid* mgid, uint16_t& mlid)
{
   mlid = 0;
#ifndef  __NO_MULTICAST__
   if ((mgid==0) || (fOsm==0)) return false;
   return fOsm->ManageMultiCastGroup(true, mgid->raw, &mlid);
#else
   return false;
#endif
}

bool verbs::Device::UnRegisterMultiCastGroup(ibv_gid* mgid, uint16_t mlid)
{
#ifndef  __NO_MULTICAST__
   if ((mgid==0) || (fOsm==0)) return false;
   return fOsm->ManageMultiCastGroup(false, mgid->raw, &mlid);
#else
   return false;
#endif
}

struct ibv_ah* verbs::Device::CreateMAH(ibv_gid* mgid, uint32_t mlid, int mport)
{
   if (mgid==0) return 0;

   ibv_ah_attr mah_attr;
   memset(&mah_attr, 0, sizeof(ibv_ah_attr));

   mah_attr.dlid = mlid; // in host order ?
   mah_attr.port_num = mport>=0 ? mport : IbPort();
   mah_attr.sl = 0;
   mah_attr.static_rate = 0; //0x83; // shold be copied from mmember rec

   mah_attr.is_global  = 1;
   memcpy(&(mah_attr.grh.dgid), mgid, sizeof(ibv_gid));
   mah_attr.grh.sgid_index = 0; // GetGidIndex(mgid);

   mah_attr.grh.flow_label = 0; // shold be copied from mmember rec
   mah_attr.grh.hop_limit = 63; // shold be copied from mmember rec
   mah_attr.grh.traffic_class = 0; // shold be copied from mmember rec

   //DOUT1(("Addr %02x %02x", ah_attr.grh.dgid.raw[0], ah_attr.grh.dgid.raw[1]));

   struct ibv_ah* f_ah = ibv_create_ah(pd(), &mah_attr);
   if (f_ah==0) {
     EOUT(("Failed to create Multicast Address Handle"));
   }

   return f_ah;
}

void verbs::Device::CreatePortQP(const char* thrd_name, dabc::Port* port, int conn_type,
                                     ComplQueue* &port_cq, QueuePair* &port_qp)
{
   ibv_qp_type qp_type = IBV_QPT_RC;

   if (conn_type>0) qp_type = (ibv_qp_type) conn_type;

   Thread* thrd = MakeThread(thrd_name, true);

   bool isowncq = IsAllocateIndividualCQ() && !thrd->IsFastModus();

   if (isowncq)
      port_cq = new ComplQueue(fContext, port->NumOutputBuffersRequired() + port->NumInputBuffersRequired() + 2, thrd->Channel());
   else
      port_cq = thrd->MakeCQ();

   port_qp = new QueuePair(this, qp_type,
                           port_cq, port->NumOutputBuffersRequired(), fDeviceAttr.max_sge - 1,
                           port_cq, port->NumInputBuffersRequired(), /*fDeviceAttr.max_sge / 2*/ 2);
    if (!isowncq)
       port_cq = 0;
}

dabc::Folder* verbs::Device::GetPoolRegFolder(bool force)
{
   return GetFolder("PoolReg", force, true);
}

verbs::PoolRegistry* verbs::Device::FindPoolRegistry(dabc::MemoryPool* pool)
{
   if (pool==0) return 0;

   dabc::Folder* fold = GetPoolRegFolder(false);
   if (fold==0) return 0;

   for (unsigned n=0; n<fold->NumChilds(); n++) {
       PoolRegistry* reg = (PoolRegistry*) fold->GetChild(n);
       if ((reg!=0) && (reg->GetPool()==pool)) return reg;
   }

   return 0;
}

verbs::PoolRegistry* verbs::Device::RegisterPool(dabc::MemoryPool* pool)
{
   if (pool==0) return 0;

   PoolRegistry* entry = FindPoolRegistry(pool);

   if (entry==0) entry = new PoolRegistry(this, pool);

   entry->IncUsage();

   return entry;
}

void verbs::Device::UnregisterPool(PoolRegistry* entry)
{
   if (entry==0) return;

   DOUT3(("Call UnregisterPool %s", entry->GetName()));

   entry->DecUsage();

   if (entry->GetUsage()<=0) {
      DOUT3(("Pool %s no longer used in verbs device: %s entry %p", entry->GetName(), GetName(), entry));
      //delete entry;
      // entry->GetParent()->RemoveChild(entry);
      // delete entry;

      //dabc::mgr()->DestroyObject(entry);

      //entry->CleanMRStructure();

      // DOUT1(("Clean entry %p done", entry));
   }

   DOUT3(("Call UnregisterPool done"));
}

void verbs::Device::CreateVerbsTransport(const char* thrdname, const char* portname, bool useackn, ComplQueue* cq, QueuePair* qp)
{
   if (qp==0) return;

   dabc::Port* port = dabc::mgr()->FindPort(portname);

   Thread* thrd = MakeThread(thrdname, false);

   if ((thrd==0) || (port==0)) {
      EOUT(("verbs::Thread %s:%p or Port %s:%p is dissapiar!!!", thrdname, thrd, portname, port));
      delete qp;
      delete cq;
      return;
   }

   Transport* tr = new Transport(this, cq, qp, port, useackn);

   tr->AssignProcessorToThread(thrd);

   port->AssignTransport(tr);
}

bool verbs::Device::ServerConnect(dabc::Command* cmd, dabc::Port* port, const char* portname)
{
   if (cmd==0) return false;

   return ((Thread*) ProcessorThread())->DoServer(cmd, port, portname);
}

bool verbs::Device::ClientConnect(dabc::Command* cmd, dabc::Port* port, const char* portname)
{
   if (cmd==0) return false;

   return ((Thread*) ProcessorThread())->DoClient(cmd, port, portname);
}

bool verbs::Device::SubmitRemoteCommand(const char* servid, const char* channelid, dabc::Command* cmd)
{
   return false;
}

verbs::Thread* verbs::Device::MakeThread(const char* name, bool force)
{
   Thread* thrd = dynamic_cast<Thread*> (dabc::Manager::Instance()->FindThread(name, VERBS_THRD_CLASSNAME));

   if (thrd || !force) return thrd;

   return dynamic_cast<Thread*> (dabc::Manager::Instance()->CreateThread(name, VERBS_THRD_CLASSNAME, 0, GetName()));
}

int verbs::Device::CreateTransport(dabc::Command* cmd, dabc::Port* port)
{
   bool isserver = cmd->GetBool("IsServer", true);

   const char* portname = cmd->GetPar("PortName");

   if (isserver ? ServerConnect(cmd, port, portname) : ClientConnect(cmd, port, portname))
      return cmd_postponed;

   return cmd_false;
}

int verbs::Device::ExecuteCommand(dabc::Command* cmd)
{
   int cmd_res = cmd_true;

   DOUT5(("Execute command %s", cmd->GetName()));

   if (cmd->IsName("StartServer")) {
	   std::string servid;
      ((Thread*) ProcessorThread())->FillServerId(servid);
      cmd->SetPar("ConnId", servid.c_str());
   } else
      cmd_res = dabc::Device::ExecuteCommand(cmd);

   return cmd_res;
}
