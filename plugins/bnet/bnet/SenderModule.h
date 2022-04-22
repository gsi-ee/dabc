#ifndef BNET_SenderModule
#define BNET_SenderModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_eventsapi
#include "dabc/eventsapi.h"
#endif

namespace bnet {

   class SenderModule : public dabc::ModuleAsync {
      protected:

         struct OutputRec {
            dabc::EventsProducerRef iter;
         };

         dabc::EventsIteratorRef fIter;
         std::vector<OutputRec> fOut;

         bool ProcessData();

         bool ProcessRecv(unsigned) override { return ProcessData(); }
         bool ProcessSend(unsigned) override { return ProcessData(); }
         void ProcessTimerEvent(unsigned) override;

      public:

         SenderModule(const std::string &name, dabc::Command cmd);

         virtual ~SenderModule();

         int ExecuteCommand(dabc::Command cmd) override;

         void BeforeModuleStart() override;

         void AfterModuleStop() override;
   };

}

#endif
