#ifndef BNET_SenderModule
#define BNET_SenderModule

#include "dabc/ModuleAsync.h"
#include "dabc/logging.h"
#include "dabc/statistic.h"
#include "dabc/Buffer.h"

#include "bnet/common.h"

#include <map>
#include <list>
#include <vector>

namespace bnet {

   class WorkerApplication;

   class SenderModule : public dabc::ModuleAsync {
      protected:

         struct SenderRec {
            dabc::Buffer* buf;
            int tgtnode;
            SenderRec(dabc::Buffer* b) : buf(b), tgtnode(-1) {}
            SenderRec(const SenderRec& src) : buf(src.buf), tgtnode(src.tgtnode) {}
         };

         typedef std::map<bnet::EventId, SenderRec> SenderMap;
         typedef std::pair<bnet::EventId, SenderRec> SenderPair;
         typedef std::list<bnet::EventId> SenderList;

         WorkerApplication*     fPlugin;

         dabc::PoolHandle*  fPool;

         SenderMap     fMap;
         SenderList    fNewEvents;
         SenderList    fReadyEvents;

         int                 fCfgNumNodes;
         int                 fCfgNodeId;
         bool                fStandalone;
         NodesVector         fRecvNodes;

         dabc::PoolHandle*  fCtrlPool;
         dabc::Port*          fCtrlPort;
         dabc::BufferSize_t   fCtrlBufSize;
         double               fCtrlOutTime;

         dabc::Ratemeter      fSendRate;

         void StandaloneProcessEvent(dabc::ModuleItem* item, uint16_t evid);

         void CheckNewEvents();
         void CheckReadyEvents();

         void ReactOnDisconnect(dabc::Port* port);

      public:
         SenderModule(const char* name, WorkerApplication* factory);

         virtual ~SenderModule();

         virtual void ProcessUserEvent(dabc::ModuleItem* item, uint16_t evid);

         virtual void ProcessInputEvent(dabc::Port* port);
         virtual void ProcessOutputEvent(dabc::Port* port);
         virtual int ExecuteCommand(dabc::Command* cmd);

         virtual void BeforeModuleStart();
         virtual void AfterModuleStop();
   };
}

#endif
