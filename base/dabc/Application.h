#ifndef DABC_Application
#define DABC_Application

#ifndef DABC_Folder
#include "dabc/Folder.h"
#endif

#ifndef DABC_WorkingProcessor
#include "dabc/WorkingProcessor.h"
#endif

namespace dabc {

   class Module;

   class Application : public Folder,
                       public WorkingProcessor {
       public:

          friend class Manager;

          typedef void* ExternalFunction();

          Application(const char* classname);

          virtual ~Application();

          virtual CommandReceiver* GetCmdReceiver() { return this; }

          virtual Module* CreateModule(const char* classname, const char* modulename, Command* cmd) { return 0; }

          virtual int ExecuteCommand(dabc::Command* cmd);

          virtual bool DoStateTransition(const char* state_trans_name);

          // These methods are called in the state transition process,
          // but always from application thread (via command channel)
          // Implement them to create modules, test if connection is done and
          // do specific actions before modules stars, after modules stopped and before module destroyed

          virtual bool CreateAppModules();
          virtual int ConnectAppModules(Command* cmd);
          virtual int IsAppModulesConnected() { return 1; } // 0 - error, 1 - ok, 2 - not ready
          virtual bool BeforeAppModulesStarted() { return true; }
          virtual bool AfterAppModulesStopped() { return true; }
          virtual bool BeforeAppModulesDestroyed() { return true; }

          virtual int SMCommandTimeout() const { return 10; }

          virtual bool IsModulesRunning();

          void InvokeCheckModulesCmd();

          virtual const char* MasterClassName() const;
          virtual const char* ClassName() const { return fAppClass.c_str(); }
          virtual bool UseMasterClassName() const { return true; }

          virtual bool Store(ConfigIO &cfg);
          virtual bool Find(ConfigIO &cfg);

       protected:

          Application(ExternalFunction* initfunc);

          virtual double ProcessTimeout(double last_diff);

          std::string        fAppClass;
          Command*           fConnCmd;
          double             fConnTmout;
          ExternalFunction*  fInitFunc;

   };

}

#endif
