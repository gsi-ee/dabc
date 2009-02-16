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
#ifndef BNET_WorkerApplication
#define BNET_WorkerApplication

#include "dabc/Application.h"
#include "dabc/Basic.h"
#include "dabc/threads.h"
#include "dabc/MemoryPool.h"
#include "dabc/Manager.h"
#include "dabc/Module.h"

#include "bnet/common.h"

namespace bnet {

   class WorkerApplication : public dabc::Application {
      public:
         WorkerApplication(const char* classname);

         int   CombinerModus() const { return GetParInt(xmlCombinerModus, 0); }
         int   NumReadouts() const { return GetParInt(xmlNumReadouts, 1); }
         std::string ReadoutPar(int nreadout = 0) const;
         bool  IsGenerator() const { return GetParBool(xmlIsGenerator, false); }
         bool  IsSender() const { return GetParBool(xmlIsSender, false); }
         bool  IsReceiver() const { return GetParBool(xmlIsReceiever, false); }
         bool  IsFilter() const { return GetParBool(xmlIsFilter, false); }

         virtual int ExecuteCommand(dabc::Command* cmd);

         virtual bool CreateReadout(const char* portname, int portnumber) { return false; }

         virtual bool CreateCombiner(const char* name) { return false; }
         virtual bool CreateBuilder(const char* name) { return false; }
         virtual bool CreateFilter(const char* name) { return false; }

         virtual bool CreateStorage(const char* portname);

         virtual bool CreateAppModules();
         virtual int IsAppModulesConnected();

         virtual bool IsModulesRunning();

         static std::string ReadoutParName(int nreadout);

      protected:

         virtual void DiscoverNodeConfig(dabc::Command* cmd);
         void ApplyNodeConfig(dabc::Command* cmd);
         bool CheckWorkerModules();
   };
}

#endif
