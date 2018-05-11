#ifndef BNET_GeneratorModule
#define BNET_GeneratorModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_eventsapi
#include "dabc/eventsapi.h"
#endif

namespace bnet {

   class GeneratorModule : public dabc::ModuleAsync {
      protected:
         uint64_t            fEventCnt;
         int                 fUniqueId;
         int                 fNumIds;

         dabc::EventsProducerRef  fEventsProducer;

      public:
         GeneratorModule(const std::string &name, dabc::Command cmd = nullptr);

         virtual void BeforeModuleStart();

         virtual bool ProcessSend(unsigned port);
   };
}

#endif
