#include "dimc/DimcManager.h"


#include "dabc/logging.h"
//#include "dabc/timing.h"
#include "dabc/Configuration.h"
#include "dabc/CommandDefinition.h"
#include "dabc/Parameter.h"
#include "dabc/Module.h"


#include "dimc/DimcCommands.h"
#include "dimc/DimcRegistry.h"

#include <iostream>



/*************************************************************************************
 * The factory for creation of dim control manager:
 *
 */

dimc::ManagerFactory dimManagerFactory("dimcfactory"); // instantiate at library load time


dimc::ManagerFactory::ManagerFactory(const char* name) : dabc::Factory(name)
 {

 }

bool dimc::ManagerFactory::CreateManagerInstance(const char* kind, dabc::Configuration* cfg)
{
   if ((kind!=0) && (strcmp(kind,DIMC_MANAGERTYPE)==0)) {
      new dimc::Manager(cfg->MgrName(), false, cfg);
      return true;
   }
   return false;
}



/*************************************************************************************
 * The Manager class itself
 *
 **************************************************************************************/


dimc::Manager::Manager(const char* managername, bool usecurrentprocess, dabc::Configuration* cfg) :
   dabc::Manager(managername, usecurrentprocess, cfg), fIsMainMgr(false)
{
   // setup of nodes registry here or in base class?
   InitSMmodule();
   fRegistry=new dimc::Registry(this);
   // define commands here:
   fRegistry->DefineDIMCommand(dabc::Manager::stcmdDoConfigure);
   fRegistry->DefineDIMCommand(dabc::Manager::stcmdDoEnable);
   fRegistry->DefineDIMCommand(dabc::Manager::stcmdDoHalt);
   fRegistry->DefineDIMCommand(dabc::Manager::stcmdDoStart);
   fRegistry->DefineDIMCommand(dabc::Manager::stcmdDoStop);
   fRegistry->DefineDIMCommand(dabc::Manager::stcmdDoError);

   fRegistry->DefineDIMCommand(_DIMC_COMMAND_SHUTDOWN_);
   fRegistry->DefineDIMCommand(_DIMC_COMMAND_SETPAR_);
   fRegistry->DefineDIMCommand(_DIMC_COMMAND_MANAGER_);


   fNodeId=fCfg->MgrNodeId();
   if(fNodeId==0) fIsMainMgr=true; // TODO: specify main manager from other quality in config?

   fStatusRecord=new dabc::StatusParameter(this, _DIMC_SERVICE_FSMRECORD_,0);//CurrentState());
   init();
   //CreateApplication(); // do we need this here, or called by run executable?
   UpdateStatusRecord();
   fRegistry->StartDIMServer("",0); // 0 port means: use environment variables for dns
}

dimc::Manager::~Manager()
{
   DestroyPar(_DIMC_SERVICE_FSMRECORD_);
   CleanupManager(77);
   delete fRegistry;
   destroy();
}


void dimc::Manager::Shutdown()
{
    DoHaltManager();
    CancelCommands();
    DeleteChilds();
    DOUT0(("Normal exit after Shutdown"));
    exit(0);
}


bool dimc::Manager::CanSendCmdToManager(const char* mgrname)
{
   return true;
}



bool dimc::Manager::SendOverCommandChannel(const char* managername, const char* cmddata)
{
   std::string commandname=_DIMC_COMMAND_MANAGER_;
     std::cout <<"SendOverCommandChannel with string:"<<cmddata << std::endl;
     fRegistry->SendDIMCommand(managername,commandname,cmddata);
   return true;
}



int dimc::Manager::ExecuteCommand(dabc::Command* cmd)
{
   dimc::Command* dimcom=dynamic_cast<dimc::Command*>(cmd);
   if(dimcom)
      {
         if(dimcom->IsName(_DIMC_COMMAND_SHUTDOWN_))
            {
               DOUT0(("ExecuteCommand sees Shutdown.."));
               Shutdown();
            }
         else if(dimcom->IsName(_DIMC_COMMAND_SETPAR_))
            {
               fRegistry->SetDIMVariable(dimcom->DimPar());
            }
         else if(dimcom->IsName(_DIMC_COMMAND_MANAGER_))
            {
               //std::cout <<"ModuleCommand string:"<<par <<":"<< std::endl;
               RecvOverCommandChannel(dimcom->DimPar());
            }
         else
            {
               // all other commands should be state transitions by name
               InvokeStateTransition(dimcom->GetName());
            }
      }
   else
      {
         return Manager::ExecuteCommand(cmd);
      }
   return cmd_true;
}

void dimc::Manager::ParameterEvent(dabc::Parameter* par, int event)
{
   std::string pname=par->GetName();
    switch(event)
    {
       case 0: // creation of new parameter
          fRegistry->RegisterParameter(par,true);
          break;
       case 1: // parameter is updated in module
          fRegistry->ParameterUpdated(par);
          // update state record for gui display from core state string:
          if(pname==dabc::Manager::stParName) UpdateStatusRecord();
          break;
       case 2: // parameter is deleted
           fRegistry->UnregisterParameter(par);
           //fxRegistry->RemoveControlParameter(registername);
        break;
       default:
          EOUT(("dimc::Manager::ParameterEvent - UNKNOWN EVENT ID: %d \n",event));
        break;

    } // switch

}


void dimc::Manager::UpdateStatusRecord()
{
   std::string theState=CurrentState();
   std::string color="Red";
   if(theState==dabc::Manager::stHalted)
      {
         color="Red";
      }
   else if (theState==dabc::Manager::stConfigured)
      {
         color="Yellow";
      }
   else if (theState==dabc::Manager::stReady)
      {
         color="Cyan";
      }
   else if (theState==dabc::Manager::stRunning)
      {
         color="Green";
      }
   else if (theState==dabc::Manager::stFailure)
      {
         color="Magenta";
      }
   else if (theState==dabc::Manager::stError)
      {
         color="Magenta";
      }
   else
      {
         color="Blue"; // never come here!
         EOUT(("dimc::Manager::UpdateStatusRecord - NEVER COME HERE - UNKNOWN STATE descriptor: %s \n",theState.c_str()));
      }
   fStatusRecord->SetStatus(theState.c_str(), color.c_str());

}





void dimc::Manager::CommandRegistration(dabc::Module* m, dabc::CommandDefinition* def, bool reg)
{
  std::string registername=fRegistry->CreateFullParameterName(m->GetName(),def->GetName());
  if(reg)
   {
      //std::cout <<"******** Manager::CommandRegistration register "<<registername << std::endl;
      fRegistry->RegisterModuleCommand(registername,def);
   }
  else
   {
      //std::cout <<"******** Manager::CommandRegistration unregister "<<registername << std::endl;
      fRegistry->UnRegisterModuleCommand(registername);
   }

}




bool dimc::Manager::Subscribe(dabc::Parameter* par, int remnode, const char* remname)
{
   std::string nodename=fCfg->ContextName(remnode);
   std::string fulldimname= fRegistry->CreateDIMPrefix(nodename.c_str())+remname;
   return (fRegistry->SubscribeParameter(par,fulldimname));
}

bool dimc::Manager::Unsubscribe(dabc::Parameter* par)
{
   return (fRegistry->UnsubscribeParameter(par));
}


int dimc::Manager::NumNodes()
{
   return fCfg->NumNodes();
}

const char* dimc::Manager::GetNodeName(int num)
{
   std::string name=GetName();
   if(fCfg!=0) name=fRegistry->GetDIMServerName(fCfg->ContextName(num).c_str()); // already adds dabc prefix
   return name.c_str();
}

bool dimc::Manager::IsNodeActive(int num)
{
   return true;
}
