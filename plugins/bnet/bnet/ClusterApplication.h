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
#ifndef BNET_ClusterApplication
#define BNET_ClusterApplication

#include "dabc/Application.h"

#include "bnet/common.h"

#include <vector>

namespace bnet {

   class ClusterApplication : public dabc::Application {

      friend class ClusterModule;

      public:
         ClusterApplication(const char* clname = 0);
         virtual ~ClusterApplication();

         virtual bool CreateAppModules();

         virtual bool BeforeAppModulesDestroyed();

         std::string NetDevice() const { return Par(xmlNetDevice).AsStdStr(dabc::typeSocketDevice); }
         bool IsRunning() const { return Par(xmlIsRunning).AsBool(); }
         bool WithController() const { return Par(xmlWithController).AsBool(false); }

         virtual int ExecuteCommand(dabc::Command cmd);

         void DiscoverCommandCompleted(dabc::Command cmd);

      protected:

         /** Return number of workers in configuration */
         int NumWorkers();

         /** Indicates if worker is really active - has either receiver or sender */
         bool IsWorkerActive(int nodeid);

         /** return names of worker node name, which can be specified for command receiving */
         const char* GetWorkerNodeName(int nodeid);

         std::string NodeCurrentState(int nodeid);

         bool ActualTransition(const std::string& trans_name);

         enum NodeMaskValues {
            mask_Sender = 0x1,
            mask_Receiever = 0x2 };

         bool ExecuteClusterSMCommand(const std::string& smcmdname);

         bool StartDiscoverConfig(dabc::Command mastercmd);
         bool StartClusterSMCommand(dabc::Command mastercmd);
         bool StartModulesConnect(dabc::Command mastercmd);
         bool StartConfigureApply(dabc::Command mastercmd);

         std::vector<int> fNodeMask; // info about items on each node
         std::vector<std::string> fNodeNames; // info about names of currently configured nodes
         std::vector<std::string> fSendMatrix; // info about send connections
         std::vector<std::string> fRecvMatrix; // info about recv connections

         int         fNumNodes; // number of nodes when app started

   };
}

extern "C" dabc::Application* ClusterPluginsInit(dabc::Manager* mgr);

#endif
