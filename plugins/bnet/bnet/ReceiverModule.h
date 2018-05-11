#ifndef BNET_ReceiverModule
#define BNET_ReceiverModule

#include "dabc/ModuleAsync.h"

namespace bnet {

   class ReceiverModule : public dabc::ModuleAsync {
      protected:

         virtual bool ProcessRecv(unsigned port);
         virtual bool ProcessSend(unsigned port);
         virtual void ProcessTimerEvent(unsigned timer);

      public:

         ReceiverModule(const std::string &name, dabc::Command cmd);

         virtual ~ReceiverModule();

         virtual int ExecuteCommand(dabc::Command cmd);

         virtual void BeforeModuleStart();

         virtual void AfterModuleStop();
   };

}

#endif
