#ifndef BNET_ReceiverModule
#define BNET_ReceiverModule

#include "dabc/ModuleAsync.h"

namespace bnet {

   class ReceiverModule : public dabc::ModuleAsync {
      protected:

         bool ProcessRecv(unsigned port) override;
         bool ProcessSend(unsigned port) override;
         void ProcessTimerEvent(unsigned timer) override;

      public:

         ReceiverModule(const std::string &name, dabc::Command cmd);

         virtual ~ReceiverModule();

         int ExecuteCommand(dabc::Command cmd) override;

         void BeforeModuleStart() override;

         void AfterModuleStop() override;
   };

}

#endif
