#ifndef DABC_VerbsDevice
#define DABC_VerbsDevice

#ifndef DABC_GenericDevice
#include "dabc/Device.h"
#endif

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

#include <infiniband/verbs.h>

namespace dabc {
    
   class VerbsOsm;
   class VerbsQP;
   class VerbsCQ;
   class VerbsDevice; 
   class MemoryPool;
   class VerbsPoolRegistry;

   class VerbsTransport;
   class VerbsThread;
   class VerbsProtocolProcessor;
   
   // ________________________________________________________
    
   class VerbsPoolRegistry : public Basic {
      public:
         VerbsPoolRegistry(VerbsDevice* verbs, MemoryPool* pool, bool local = false);
         virtual ~VerbsPoolRegistry();

         MemoryPool* GetPool() const { return fPool; }
         Mutex* WorkMutex() const { return fWorkMutex; }
         
         int GetUsage() const { return fUsage; }
         
         void IncUsage() { fUsage++; } 
         void DecUsage() { fUsage--; } 
         
         inline void _CheckMRStructure() 
         {
            if ((fPool!=0) && (fWorkMutex!=0) && (fLastChangeCounter != fPool->GetChangeCounter()))
              _UpdateMRStructure();
         }
         
         inline uint32_t GetLkey(BufferId_t id) { return f_mr[GetBlockNum(id)]->lkey; }
         
         virtual void DependendDestroyed(Basic* obj);

         void CleanMRStructure();
         
      protected:
         VerbsDevice    *fVerbs; 
         MemoryPool*     fPool;
         Mutex*          fWorkMutex;
         int             fUsage;
         unsigned        fLastChangeCounter;
         unsigned        f_nummr;     // size of f_mr array
         struct ibv_mr* *f_mr;
         unsigned       *fBlockChanged;

         void _UpdateMRStructure();
   };
 
    
   // ____________________________________________________________________
    
   class VerbsDevice : public Device {
      friend class VerbsTransport;
      friend class VerbsThread;
      friend class VerbsProtocolProcessor;

      public:

         VerbsDevice(Basic* parent, const char* name);
         virtual ~VerbsDevice();
         
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

         virtual bool ServerConnect(Command* cmd, Port* port, const char*);
         virtual bool ClientConnect(Command* cmd, Port* port, const char*);
         virtual bool SubmitRemoteCommand(const char* servid, const char* channelid, Command* cmd);

         Folder* GetPoolRegFolder(bool force = false);
         
         VerbsThread* GetVerbsThread(const char* name, bool force = false);
         
         bool IsAllocateIndividualCQ() const { return fAllocateIndividualCQ; }
         void SetAllocateIndividualCQ(bool on) { fAllocateIndividualCQ = on; }
         
         virtual const char* ClassName() const { return "VerbsDevice"; }
         
         static bool IsThreadSafeVerbs() { return fThreadSafeVerbs; }

      protected:

         virtual int ExecuteCommand(dabc::Command* cmd);
      
         bool OpenVerbs(bool withmulticast = false, const char* devicename = 0, int ibport = -1);
         bool CloseVerbs();

         void CreatePortQP(const char* thrd_name, Port* port, int conn_type,
                           VerbsCQ* &port_cq, VerbsQP* &port_qp);
         void CreateVerbsTransport(const char* thrdname, const char* portname, VerbsCQ* cq, VerbsQP* qp);
         
         virtual int CreateTransport(Command* cmd, Port* port);
         
         VerbsPoolRegistry* FindPoolRegistry(MemoryPool* pool);
         VerbsPoolRegistry* RegisterPool(MemoryPool* pool);
         void UnregisterPool(VerbsPoolRegistry* reg);

         uint8_t fIbPort;  // infiniband port number
   
         struct ibv_context *fContext;         // device context
         struct ibv_pd *fPD;                   // protection domain
         struct ibv_device_attr fDeviceAttr;  // device attributes
         struct ibv_port_attr fPortAttr;      // port attributes
         uint16_t f_mtu;                      // default raw packet size
   
         VerbsOsm      *fOsm; // osm object created when multicast is required
         
         bool fAllocateIndividualCQ; // for connections individual CQ will be used
         
         static bool fThreadSafeVerbs;  // identifies if verbs is thread safe
         
   }; 
}

#endif
