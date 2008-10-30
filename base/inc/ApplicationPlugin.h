#ifndef DABC_ApplicationPlugin
#define DABC_ApplicationPlugin

#ifndef DABC_Folder
#include "dabc/Folder.h"
#endif

#ifndef DABC_WorkingProcessor
#include "dabc/WorkingProcessor.h"
#endif

namespace dabc {
 
   class Module;   
   class FileIO;
   class DataInput;
   class DataOutput;
   
   class ApplicationPlugin : public Folder,
                             public WorkingProcessor {
       public: 
          ApplicationPlugin(Manager* m, const char* name, const char* thrdname = 0);
          
          virtual ~ApplicationPlugin();
          
          virtual CommandReceiver* GetCmdReceiver() { return this; }

          virtual Module* CreateModule(const char* classname, const char* modulename, Command* cmd) { return 0; }

          virtual int ExecuteCommand(dabc::Command* cmd);
          
          virtual bool DoStateTransition(const char* state_trans_name);
          
          // These methods are called in the state transition process,
          // but always from plugin thread (via command channel)
          // Implemenet them to create modules, test if connection is done and
          // do specific actions before modules stars, sfter modules stopped and before module destroyed
          
          virtual bool CreateAppModules() { return true; }
          virtual int IsAppModulesConnected() { return 1; } // 0 - error, 1 - ok, 2 - not ready
          virtual bool BeforeAppModulesStarted() { return true; }
          virtual bool AfterAppModulesStopped() { return true; }
          virtual bool BeforeAppModulesDestroyed() { return true; }
          
          virtual int SMCommandTimeout() const { return 10; }
          
       protected:
          virtual double ProcessTimeout(double last_diff);
          
          Command* fConnCmd;
          double fConnTmout;

   };
   
   typedef void (*InitApplicationPluginFunc)(Manager* mgr);

}

#endif
