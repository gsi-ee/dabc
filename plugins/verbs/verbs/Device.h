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
#ifndef VERBS_Device
#define VERBS_Device

#ifndef DABC_GenericDevice
#include "dabc/Device.h"
#endif

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

#include <infiniband/verbs.h>

namespace verbs {

   class OpenSM;
   class QueuePair;
   class ComplQueue;
   class Device;
   class MemoryPool;
   class PoolRegistry;

   class Transport;
   class Thread;
   class ProtocolProcessor;

   extern const char* xmlMcastAddr;

   // ________________________________________________________

   class PoolRegistry : public dabc::Basic {
      public:
         PoolRegistry(Device* verbs, dabc::MemoryPool* pool, bool local = false);
         virtual ~PoolRegistry();

         dabc::MemoryPool* GetPool() const { return fPool; }
         dabc::Mutex* WorkMutex() const { return fWorkMutex; }

         int GetUsage() const { return fUsage; }

         void IncUsage() { fUsage++; }
         void DecUsage() { fUsage--; }

         inline void _CheckMRStructure()
         {
            if ((fPool!=0) && (fWorkMutex!=0) && (fLastChangeCounter != fPool->GetChangeCounter()))
              _UpdateMRStructure();
         }

         inline uint32_t GetLkey(dabc::BufferId_t id) { return f_mr[dabc::GetBlockNum(id)]->lkey; }

         virtual void DependendDestroyed(dabc::Basic* obj);

         void CleanMRStructure();

      protected:
         Device            *fVerbs;
         dabc::MemoryPool  *fPool;
         dabc::Mutex*       fWorkMutex;
         int                fUsage;
         unsigned           fLastChangeCounter;
         unsigned           f_nummr;     // size of f_mr array
         struct ibv_mr*    *f_mr;
         unsigned          *fBlockChanged;

         void _UpdateMRStructure();
   };


   // ____________________________________________________________________

   class Device : public dabc::Device {
      friend class Transport;
      friend class Thread;
      friend class ProtocolProcessor;

      public:

         Device(dabc::Basic* parent, const char* name);
         virtual ~Device();

         bool IsMulticast() const { return fOsm!=0; }
         int IbPort() const { return fIbPort; }
         struct ibv_context *context() const { return fContext; }
         struct ibv_pd *pd() const { return fPD; }
         uint16_t lid() const { return fPortAttr.lid; }
         uint16_t mtu() const { return f_mtu; }
         int GetGidIndex(ibv_gid* lookgid);

         struct ibv_ah* CreateAH(uint32_t dest_lid, int dest_port = -1);

         bool RegisterMultiCastGroup(ibv_gid* mgid, uint16_t& mlid);
         bool UnRegisterMultiCastGroup(ibv_gid* mgid, uint16_t mlid);
         struct ibv_ah* CreateMAH(ibv_gid* mgid, uint32_t mlid, int dest_port = -1);

         virtual bool ServerConnect(dabc::Command* cmd, dabc::Port* port, const char*);
         virtual bool ClientConnect(dabc::Command* cmd, dabc::Port* port, const char*);
         virtual bool SubmitRemoteCommand(const char* servid, const char* channelid, dabc::Command* cmd);

         dabc::Folder* GetPoolRegFolder(bool force = false);

         Thread* MakeThread(const char* name, bool force = false);

         bool IsAllocateIndividualCQ() const { return fAllocateIndividualCQ; }
         void SetAllocateIndividualCQ(bool on) { fAllocateIndividualCQ = on; }

         virtual const char* ClassName() const { return "Device"; }

         static bool IsThreadSafeVerbs() { return fThreadSafeVerbs; }

      protected:

         virtual int ExecuteCommand(dabc::Command* cmd);

         bool OpenVerbs(bool withmulticast = false, const char* devicename = 0, int ibport = -1);
         bool CloseVerbs();

         bool CreatePortQP(const char* thrd_name, dabc::Port* port, int conn_type,
                           Thread* &thrd, ComplQueue* &port_cq, QueuePair* &port_qp);
         void CreateVerbsTransport(const char* thrdname, const char* portname, bool useackn, ComplQueue* cq, QueuePair* qp);

         virtual int CreateTransport(dabc::Command* cmd, dabc::Port* port);

         PoolRegistry* FindPoolRegistry(dabc::MemoryPool* pool);
         PoolRegistry* RegisterPool(dabc::MemoryPool* pool);
         void UnregisterPool(PoolRegistry* reg);

         uint8_t fIbPort;  // infiniband port number

         struct ibv_context *fContext;         // device context
         struct ibv_pd *fPD;                   // protection domain
         struct ibv_device_attr fDeviceAttr;  // device attributes
         struct ibv_port_attr fPortAttr;      // port attributes
         uint16_t f_mtu;                      // default raw packet size

         OpenSM      *fOsm; // osm object created when multicast is required

         bool fAllocateIndividualCQ; // for connections individual CQ will be used

         static bool fThreadSafeVerbs;  // identifies if verbs is thread safe
   };


   extern bool ConvertStrToGid(const std::string& s, ibv_gid &gid);
   extern std::string ConvertGidToStr(ibv_gid &gid);

}

#endif
