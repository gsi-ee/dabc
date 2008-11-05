#ifndef BNET_ReceiverModule
#define BNET_ReceiverModule

#include "dabc/ModuleAsync.h"
#include "dabc/logging.h"

#include "bnet/common.h"

#include <vector>
#include <map>

namespace bnet {

   class WorkerApplication;

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

         dabc::PoolHandle* fPool;
         dabc::Ratemeter     fRecvRate;
         int                 fCfgNumNodes;
         int                 fCfgNodeId;
         NodesVector         fSendNodes;
         unsigned            fInpCounter;

         typedef std::map<EventId, unsigned> EventsMap;

         std::vector<RecvEventData*> fEventsPool;
         unsigned            fPoolTail; // last used event
         unsigned            fPoolHead; // first used event
         unsigned            fPoolSize; // number of used items in pool
         EventsMap           fEventsMap; // correlation between evid and index
         std::vector<EventId> fLastEvents;
         int                 fPoolTailSending; // indicates if we are sending buffers from pool tail

         unsigned             fSendingCounter;
         std::vector<EventId> fCurrEvent;
         bool                 fStopSending;

         bool DoInputRead(int nodeid);
         RecvEventData* ProvideEventData(EventId evid, bool force = false);

         EventId DefineMinimumLastEvent();

      public:
         ReceiverModule(const char* name, WorkerApplication* factory);
         virtual ~ReceiverModule();

         virtual int ExecuteCommand(dabc::Command* cmd);

         virtual void ProcessEvent2(dabc::ModuleItem* item, uint16_t evid);
         virtual void ProcessEventNew(dabc::ModuleItem* item, uint16_t evid);
         virtual void ProcessUserEvent(dabc::ModuleItem* item, uint16_t evid);

         virtual void BeforeModuleStart();
         virtual void AfterModuleStop();
   };
}

#endif
