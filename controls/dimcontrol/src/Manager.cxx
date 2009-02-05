#include "dimc/Manager.h"


#include "dabc/logging.h"
#include "dabc/Configuration.h"
#include "dabc/CommandDefinition.h"
#include "dabc/Parameter.h"
#include "dabc/Module.h"


#include "dimc/Commands.h"
#include "dimc/Registry.h"

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
      new dimc::Manager(cfg->MgrName(), false, cfg); // NOTE: manager name will be overwritten to match dim prefix later
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
   fRegistry=new dimc::Registry(this, cfg);
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

   fNodeId = fCfg->MgrNodeId();
   if(fNodeId==0) fIsMainMgr = true; // TODO: specify main manager from other quality in config?

   InitSMmodule();

   fStatusRecord=new dabc::StatusParameter(this, _DIMC_SERVICE_FSMRECORD_,0);//CurrentState());
   init();
   UpdateStatusRecord();
   unsigned int dimport=2505;
   if(fCfg->MgrDimPort()!=0) dimport = fCfg->MgrDimPort();
   fRegistry->StartDIMServer(fRegistry->GetDIMServerName(fNodeId), fCfg->MgrDimNode(), dimport); // 0 port means: use environment variables for dns
   DOUT1(("+++++++++++++++++ Manager ctor starting dim on dim dns node:%s, port%d",fCfg->MgrDimNode(),fCfg->MgrDimPort()));
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
   return fRegistry->IsManagerActive(mgrname);
}



bool dimc::Manager::SendOverCommandChannel(const char* managername, const char* cmddata)
{
   fRegistry->SendDIMCommand(managername, _DIMC_COMMAND_MANAGER_, cmddata);
   return true;
}



int dimc::Manager::ExecuteCommand(dabc::Command* cmd)
{
   dimc::Command* dimcom=dynamic_cast<dimc::Command*>(cmd);
   if(dimcom)
      {
         //DOUT3(("ExecuteCommand sees command %s",cmd->GetName()));
         if(dimcom->IsName(_DIMC_COMMAND_SHUTDOWN_))
            {
               DOUT0(("ExecuteCommand sees Shutdown.."));
               Shutdown();
            }
         else if(dimcom->IsName(_DIMC_COMMAND_SETDIMPAR_))
            {
               DOUT0(("ExecuteCommand sees set par:%s..",_DIMC_COMMAND_SETPAR_));
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
         return dabc::Manager::ExecuteCommand(cmd);
      }
   return cmd_true;
}

void dimc::Manager::ParameterEvent(dabc::Parameter* par, int event)
{
   std::string pname = par->GetName();
   //DOUT5(("dimc::Manager::ParameterEvent %d for %s \n",event,pname.c_str()));
   dabc::Manager::ParameterEvent(par, event);
   switch(event) {
      case dabc::parCreated: // creation of new parameter
         fRegistry->RegisterParameter(par, BuildControlName(par));
         break;
      case dabc::parModified: // parameter is updated in module
         fRegistry->ParameterUpdated(par);
         // update state record for gui display from core state string:
         if(pname==dabc::Manager::stParName) UpdateStatusRecord();
         break;
      case dabc::parDestroy: // parameter is deleted
         fRegistry->UnregisterParameter(par);
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


void dimc::Manager::CommandRegistration(dabc::CommandDefinition* def, bool reg)
{
   std::string registername = BuildControlName(def);

   DOUT5(("dimc::Manager::CommandRegistration %s %s", (reg ? "registers" : "unregisters"), registername.c_str()));

   if(reg)
      fRegistry->RegisterUserCommand(registername, def);
   else
      fRegistry->UnRegisterUserCommand(registername);
}


bool dimc::Manager::Subscribe(dabc::Parameter* par, int remnode, const char* remname)
{
   std::string fulldimname = fRegistry->GetDIMPrefix(remnode)+remname;
   return fRegistry->SubscribeParameter(par,fulldimname);
}

bool dimc::Manager::Unsubscribe(dabc::Parameter* par)
{
   return (fRegistry->UnsubscribeParameter(par));
}


int dimc::Manager::NumNodes()
{
   return fRegistry->NumNodes();
}

const char* dimc::Manager::GetNodeName(int num)
{
   return fRegistry->GetManagerName(num);
}

bool dimc::Manager::IsNodeActive(int num)
{
   return fRegistry->IsNodeActive(num);
}
