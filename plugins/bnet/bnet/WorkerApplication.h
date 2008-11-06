#ifndef BNET_WorkerApplication
#define BNET_WorkerApplication

#include "dabc/Application.h"
#include "dabc/Basic.h"
#include "dabc/threads.h"
#include "dabc/MemoryPool.h"
#include "dabc/Manager.h"
#include "dabc/Module.h"

#include "bnet/common.h"

namespace bnet {

   class WorkerApplication : public dabc::Application {
      public:
         WorkerApplication(const char* name);

         virtual dabc::Module* CreateModule(const char* classname, const char* modulename, dabc::Command* cmd);

         int   NumWorkers() const { return GetParInt("NumWorkers", 1); }

         int   CombinerModus() const { return GetParInt("CombinerModus", 0); }
         int   NumReadouts() const { return GetParInt("NumReadouts", 1); }
         const char* ReadoutPar(int nreadout = 0) const;
         bool  IsGenerator() const { return GetParInt("IsGenerator", 0) > 0; }
         bool  IsSender() const { return GetParInt("IsSender", 0) > 0; }
         bool  IsReceiver() const { return GetParInt("IsReceiver", 0) > 0; }
         bool  IsFilter() const { return GetParInt("IsFilter", 0) > 0; }
         const char* StoragePar() const { return GetParCharStar("StoragePar"); }

         // these are parameters which fixed furing lifetime of the modules
         // normally these parameters copied in modules constructor
         int   CfgNumNodes() const { return GetParInt("CfgNumNodes", 1); }
         int   CfgNodeId() const { return GetParInt("CfgNodeId", 0); }
         int   CfgEventsCombine() const { return GetParInt("CfgEventsCombine", 1); }

         // these parameters should not be used in modules constructor
         // because of dynamic configuration, which can be changed on the fly (reconfigured)
         bool  CfgController() const { return GetParInt("CfgController", 1) !=0; }
         const char* CfgSendMask() const { return GetParCharStar("CfgSendMask"); }
         const char* CfgRecvMask() const { return GetParCharStar("CfgRecvMask"); }
         const char* CfgClusterMgr() const { return GetParCharStar("CfgClusterMgr"); }
         const char* CfgNetDevice() const { return GetParCharStar("CfgNetDevice"); }

         const char* Thrd1Name() const { return GetParCharStar("Thread1Name", "Thread1"); }
         const char* Thrd2Name() const { return GetParCharStar("Thread2Name", "Thread2"); }
         const char* ThrdCtrlName() const { return GetParCharStar("ThreadCtrlName", "ThreadCtrl"); }

         int   CombinerInQueueSize() const { return GetParInt("CombinerInQueueSize", 4); }
         int   CombinerOutQueueSize() const { return GetParInt("CombinerOutQueueSize", 4); }

         const char* ReadoutPoolName() const { return CombinerModus()==0 ? "ReadoutPool" : TransportPoolName(); }
         uint64_t    ReadoutBufferSize() const { return GetParInt("ReadoutBuffer", 2048); }
         uint64_t    ReadoutPoolSize() const { return GetParInt("ReadoutPoolSize", 2*0x100000); }

         const char* TransportPoolName() const { return "TransportPool"; }
         uint64_t    TransportBufferSize() const { return GetParInt("TransportBuffer", 2048*4); }
         uint64_t    TransportPoolSize() const { return GetParInt("TransportPoolSize", 16*0x100000); }

         const char* EventPoolName() const { return "EventPool"; }
         uint64_t    EventBufferSize() const { return GetParInt("EventBuffer", 2048*16); }
         uint64_t    EventPoolSize() const { return GetParInt("EventPoolSize", 4*0x100000); }

         const char* ControlPoolName() const { return "ControlPool"; }
         uint64_t    ControlBufferSize() const { return GetParInt("CtrlBuffer", 2048); }
         uint64_t    ControlPoolSize() const { return GetParInt("CtrlPoolSize", 4*0x100000); }

         virtual int ExecuteCommand(dabc::Command* cmd);

         virtual bool CreateReadout(const char* portname, int portnumber) { return false; }

         virtual dabc::Module* CreateCombiner(const char* name) { return 0; }
         virtual dabc::Module* CreateBuilder(const char* name) { return 0; }
         virtual dabc::Module* CreateFilter(const char* name) { return 0; }

         virtual bool CreateStorage(const char* portname);

         void SetPars(bool is_all_to_all,
                      int numreadouts,
                      int combinermodus);

         static const char* ItemName();

         static const char* PluginName() { return "BnetPlugin"; }

         virtual bool CreateAppModules();
         virtual int IsAppModulesConnected();

         virtual bool IsModulesRunning();

      protected:

         void DiscoverNodeConfig(dabc::Command* cmd);
         void ApplyNodeConfig(dabc::Command* cmd);
         bool CheckWorkerModules();

         static dabc::String fPluginName;
   };
}

#endif
