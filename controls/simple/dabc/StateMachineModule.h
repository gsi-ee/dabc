#ifndef DABC_StateMachineModule
#define DABC_StateMachineModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

namespace dabc {
    
   class StateMachineModule : public ModuleAsync {
      public:
         StateMachineModule(Manager* mgr);

         virtual int ExecuteCommand(Command* cmd);
   };   
}

#endif
