/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#ifndef BNET_GlobalDFCModule
#define BNET_GlobalDFCModule

#include "dabc/ModuleAsync.h"
#include "dabc/Buffer.h"

#include "bnet/common.h"

#include <vector>
#include <map>
#include <list>

namespace bnet {

   class GlobalDFCModule : public dabc::ModuleAsync {
      protected:
         struct ControllerRec {
            int tgtnode;
            unsigned nsizes;
            std::vector<int64_t> sizes;

            ControllerRec(unsigned number) : tgtnode(-1), nsizes(0), sizes(number, -1) {}
         };

         typedef std::map<EventId, ControllerRec> ControllerMap;
         typedef std::pair<EventId, ControllerRec> ControllerPair;
         typedef std::list<EventId> ControllerList;

         dabc::PoolHandle*   fPool;

         ControllerMap         fMap;
         ControllerList        fReadyEvnts;
         dabc::BufferSize_t    fBufferSize;
         uint64_t              fTargetCounter;
         dabc::TimeStamp       fLastSendTime;

         int                   fCfgNumNodes;
         NodesVector           fSendNodes;
         NodesVector           fRecvNodes;

         void TrySendEventsAssignment(bool force);

      public:
         GlobalDFCModule(const char* name, dabc::Command cmd = 0);
         virtual ~GlobalDFCModule();

         virtual void ProcessInputEvent(dabc::Port* port);
         virtual void ProcessOutputEvent(dabc::Port* port);
         virtual void ProcessTimerEvent(dabc::Timer* timer);
         virtual int ExecuteCommand(dabc::Command cmd);

   };
}

#endif
