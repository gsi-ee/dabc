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
#ifndef DimcManager
#define DimcManager

#include "dabc/Manager.h"
#include "dabc/Factory.h"

#include "dabc/threads.h"

#include <vector>

#include <list>


#define DIMC_MANAGERTYPE "DimControl"


//class dabc::StateMachineModule;
//class dabc::Command;
//class dabc::Parameter;
//class dabc::Configuration;

namespace dimc {

   class Registry;
   /*
    * Manager for pure DIM control environment.
    *
    */
   class Manager : public dabc::Manager  {


      public:

         Manager(const char* managername,  bool usecurrentprocess=false, dabc::Configuration* cfg=0);
         virtual ~Manager();

         /** to be called by external shutdown command to properly end the application */
         void Shutdown();

         virtual bool HasClusterInfo() { return fRegistry!=0; }
         virtual int NumNodes();
         virtual int NodeId() const { return fNodeId; }
         virtual bool IsNodeActive(int num);
         virtual const char* GetNodeName(int num);

         virtual bool Subscribe(dabc::Parameter* par, int remnode, const char* remname);
         virtual bool Unsubscribe(dabc::Parameter* par);

         virtual bool IsMainManager() { return fIsMainMgr; }
         //void SetMainManager(bool on=true){fIsMainMgr=on;}


      protected:


         virtual bool CanSendCmdToManager(const char* mgrname);
         virtual bool SendOverCommandChannel(const char* managername, const char* cmddata);
         virtual void ParameterEvent(dabc::Parameter* par, int event);
         virtual void CommandRegistration(dabc::CommandDefinition* def, bool reg);

         virtual int ExecuteCommand(dabc::Command* cmd);

         /** update local status record for gui from baseclass state string*/
         void UpdateStatusRecord();

      private:
         bool                  fIsMainMgr;
         int                   fNodeId;


         Registry* fRegistry;

         /** shortcut to fsm status record for gui*/
         dabc::StatusParameter* fStatusRecord;


   };

      class ManagerFactory : public dabc::Factory {
         public:
         ManagerFactory(const char* name);

         protected:
            virtual bool CreateManagerInstance(const char* kind, dabc::Configuration* cfg);
      };





} // end namespace dimc

#endif
