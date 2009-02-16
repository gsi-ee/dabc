#ifndef BNET_ReceiverModule
#define BNET_ReceiverModule

#include "dabc/ModuleAsync.h"
#include "dabc/logging.h"
#include "dabc/statistic.h"

#include "bnet/common.h"

#include <vector>
#include <map>

namespace bnet {

   class ReceiverModule : public dabc::ModuleAsync {
      protected:

         class RecvEventData {
            public:
               EventId  evid;
               std::vector<dabc::Buffer*> bufs;

               RecvEventData(unsigned sz) :
                  evid(0),
                  bufs(sz, 0)
               {
               }
         };

         dabc::PoolHandle*   fPool;
         dabc::Ratemeter     fRecvRate;
         int                 fCfgNumNodes;
         int                 fCfgNodeId;
         NodesVector         fSendNodes;
         unsigned            fInpCounter;

         unsigned             fSendingCounter;
         std::vector<EventId> fCurrEvent;
         bool                 fStopSending;

      public:
         ReceiverModule(const char* name, dabc::Command* cmd = 0);
         virtual ~ReceiverModule();

         virtual int ExecuteCommand(dabc::Command* cmd);

         virtual void ProcessUserEvent(dabc::ModuleItem* item, uint16_t evid);

         virtual void BeforeModuleStart();
         virtual void AfterModuleStop();
   };
}

#endif
