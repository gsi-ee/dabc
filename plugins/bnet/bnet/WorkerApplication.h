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
         WorkerApplication(const char* classname);

         int   CombinerModus() const { return GetParInt(xmlCombinerModus, 0); }
         int   NumReadouts() const { return GetParInt(xmlNumReadouts, 1); }
         std::string ReadoutPar(int nreadout = 0) const;
         bool  IsGenerator() const { return GetParBool(xmlIsGenerator, false); }
         bool  IsSender() const { return GetParBool(xmlIsSender, false); }
         bool  IsReceiver() const { return GetParBool(xmlIsReceiever, false); }
         bool  IsFilter() const { return GetParBool(xmlIsFilter, false); }
         std::string StoragePar() const { return GetParStr("StoragePar"); }

         // these are parameters which fixed during lifetime of the modules
         // normally these parameters copied in modules constructor
         std::string Thrd1Name() const { return GetParStr("Thread1Name", "Thread1"); }
         std::string Thrd2Name() const { return GetParStr("Thread2Name", "Thread2"); }
         std::string ThrdCtrlName() const { return GetParStr("ThreadCtrlName", "ThreadCtrl"); }

         virtual int ExecuteCommand(dabc::Command* cmd);

         virtual bool CreateReadout(const char* portname, int portnumber) { return false; }

         virtual dabc::Module* CreateCombiner(const char* name) { return 0; }
         virtual dabc::Module* CreateBuilder(const char* name) { return 0; }
         virtual dabc::Module* CreateFilter(const char* name) { return 0; }

         virtual bool CreateStorage(const char* portname);

         virtual bool CreateAppModules();
         virtual int IsAppModulesConnected();

         virtual bool IsModulesRunning();

      protected:

         virtual void DiscoverNodeConfig(dabc::Command* cmd);
         void ApplyNodeConfig(dabc::Command* cmd);
         bool CheckWorkerModules();
   };
}

#endif
