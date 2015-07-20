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

         virtual bool ProcessRecv(unsigned port) { return ProcessData(); }
         virtual bool ProcessSend(unsigned port) { return ProcessData(); }
         virtual void ProcessTimerEvent(unsigned timer);

      public:

         SenderModule(const std::string& name, dabc::Command cmd);

         virtual ~SenderModule();

         virtual int ExecuteCommand(dabc::Command cmd);

         virtual void BeforeModuleStart();

         virtual void AfterModuleStop();
   };

}

#endif
