#include "DABCManager.h"
#include "DABCNode.h"
#include "DABCRegistry.h"

#include "DABCExceptions.h"

#include "dabc/Parameter.h"
#include "dabc/Module.h"
#include "dabc/CommandDefinition.h"
#include "dabc/Application.h"


 
dabc::xd::Manager::Manager(dabc::xd::Node* app,  const char* managername) : 
   dabc::Manager(managername), fxApplication(app), fxRegistry(0), fbMainManager(false)
{
   if(fxApplication) fxRegistry=fxApplication->Registry();
   init(); 
}


dabc::xd::Manager::~Manager()
{
   destroy();
}

void dabc::xd::Manager::ModuleExecption(dabc::Module* m, const char* msg)
{
   LOG4CPLUS_WARN(fxApplication->getApplicationLogger(), toolbox::toString("dabc::xd::Manager has exception %s in module %s",msg,m->GetName()));
   
}
 
bool dabc::xd::Manager::SendOverCommandChannel(const char* managername, const char* cmddata)
{
bool rev=fxApplication->SendManagerCommand(managername,cmddata,true); // also try here async dim!
//std::cout <<"leaving dabc::Manager::SendOverCommandChannel() to "<<managername <<std::endl;
return rev;      
   
} 
        

void dabc::xd::Manager::ProcessRemoteCommand(const char* cmddata)
{
   // need this to access from xdaq application
   //std::cout <<"dabc::Manager::ProcessRemoteCommand :"<<cmddata << std::endl;
   RecvOverCommandChannel(cmddata);
}


void dabc::xd::Manager::ParameterEvent(dabc::Parameter* par, int event)
{
   // handle here creation and update of new parameters
  //std::cout <<"dabc::xd::Manager::ParameterEvent for par:" << std::hex << par<<", e="<<event << std::endl;
   if(par==0) return;
   dabc::Manager::ParameterEvent(par,event); // need this for init timeouts?
   std::string recname="";
   dabc::Basic* holder=par->GetHolder();
   if(holder)
      { 
         if(holder==this)
            {
               //std::cout <<"hhhhh ParameterEvent: manager is holder. no prefix"<< std::endl;  
            }
         else
            {
               recname=holder->GetName();
            }
      }
//   std::cout <<"got holder name:"<<recname <<": for parameter "<<par->GetName()<< std::endl;
////// optional/debug: test type of holder. may use this for more precise naming later?
//   dabc::Application* apl= dynamic_cast<dabc::Application*>(holder);
//   dabc::GenericDevice* dev= dynamic_cast<dabc::GenericDevice*>(holder);; 
//   dabc::Module* module=dynamic_cast<dabc::Module*>(holder);  
//   if(apl)  std::cout <<" -holder is Application."<<std::endl;
//   if(dev)  std::cout <<" -holder is GenericDevice."<<std::endl;
//   if(module)  std::cout <<" -holder is Module."<<std::endl;
//////
   std::string registername=fxRegistry->CreateFullParameterName(recname.c_str(),par->GetName());
   //std::cout <<"ParameterEvent register name is "<<registername << std::endl;  
 

 dabc::IntParameter* ipar=dynamic_cast<dabc::IntParameter*>(par);
  dabc::DoubleParameter* dpar=dynamic_cast<dabc::DoubleParameter*>(par);
  dabc::StrParameter* spar=dynamic_cast<dabc::StrParameter*>(par);
  dabc::RateParameter* rate=dynamic_cast<dabc::RateParameter*>(par);
  dabc::StatusParameter* status=dynamic_cast<dabc::StatusParameter*>(par);
  dabc::InfoParameter* info=dynamic_cast<dabc::InfoParameter*>(par);
 
  dabc::HistogramParameter* his=dynamic_cast<dabc::HistogramParameter*>(par);
  
  switch(event)
  {
     case 0: // creation of new parameter
     //std::cout <<"ParameterEvent on creation of "<<registername << std::endl;  
  
      if(ipar)
         {
            //std::cout <<"dabc::Manager::ParameterEvent on creating int par:" << registername << std::endl;
            fxRegistry->AddSimpleRecord(registername,ipar->GetInt(),true, par);  
            
         }
      else if (dpar)
         {
            //std::cout <<"dabc::Manager::ParameterEvent on creating double par:" << registername << std::endl;
            fxRegistry->AddSimpleRecord(registername,dpar->GetDouble(),true, par);  
   
         }
      else if (spar)
         {
            //std::cout <<"dabc::Manager::ParameterEvent on creating string par:" << registername << std::endl;
            dabc::String sval;
            spar->GetStr(sval);
            fxRegistry->AddSimpleRecord(registername,sval.c_str(),true, par);  
   
         }
      else if (rate)
      {
         //std::cout <<"dabc::Manager::ParameterEvent on creating rate par:" << registername << std::endl;
         fxRegistry->AddRateRecord(registername, rate->GetRateRec(), par);
      }   
      else if (status)
      {
         //std::cout <<"dabc::Manager::ParameterEvent on creating status par:" << registername << std::endl;
         fxRegistry->AddStatusRecord(registername, status->GetStatusRec(), par);
      }
      else if (info)
      {
         //std::cout <<"dabc::Manager::ParameterEvent on creating status par:" << registername << std::endl;
         fxRegistry->AddInfoRecord(registername, info->GetInfoRec(), par);
      }     
      else if (his)
      {
         //std::cout <<"dabc::Manager::ParameterEvent on creating histogram par:" << registername << std::endl;
         fxRegistry->AddHistogramRecord(registername, his->GetHistogramRec(), par);
      }
      else
      {
         //std::cout <<"dabc::Manager::ParameterEvent for creating parameter" << registername<<"of unknown type"<<  std::endl;
  
      }         
      
      break;
     
     case 1: // parameter is updated in module
          //std::cout <<"dabc::Manager::ParameterEvent on update par:" << registername << std::endl;
          //return; // disabled update for testing
          if(ipar)
            {
               fxRegistry->UpdateSimpleRecord(registername,ipar->GetInt());  
            }
           else if (dpar)
            {
               fxRegistry->UpdateSimpleRecord(registername,dpar->GetDouble());  
      
            }
         else if (spar)
            {
               dabc::String sval;
               spar->GetStr(sval);
               fxRegistry->UpdateSimpleRecord(registername,sval.c_str());  
            }
         else if (rate)
            {
               //std::cout <<"dabc::Manager::ParameterEvent on creating rate par:" << registername << std::endl;
               fxRegistry->UpdateRateRecord(registername, rate->GetRateRec());
            }   
         else if (status)
            {
               //std::cout <<"dabc::Manager::ParameterEvent on creating status par:" << registername << std::endl;
               fxRegistry->UpdateStatusRecord(registername, status->GetStatusRec());
            }   
         else if (his)
            {
               //std::cout <<"dabc::Manager::ParameterEvent on creating histogram par:" << registername << std::endl;
               fxRegistry->UpdateHistogramRecord(registername, his->GetHistogramRec());
            }        
         else
            {
               std::cout <<"unknown parameter when updating in ParameterEvent" << std::endl;
            }   
      break;
     
     case 2: // parameter is deleted
         //std::cout <<"dabc::Manager::ParameterEvent on deleting par:" << registername << std::endl;
 
         fxRegistry->RemoveRecord(registername);   
         //fxRegistry->RemoveControlParameter(registername);
      break;
     
     default:
        std::cout <<"dabc::xd::Manager::ParameterEvent UNKNOWN EVENT: e=" << event << std::endl;
      break;
          
  } // switch
  
  
}

void dabc::xd::Manager::CommandRegistration(dabc::Module* m, dabc::CommandDefinition* def, bool reg)
{
  std::string registername=fxRegistry->CreateFullParameterName(m->GetName(),def->GetName());
  if(reg)
   {
      //std::cout <<"******** Manager::CommandRegistration register "<<registername << std::endl;
      fxRegistry->RegisterModuleCommand(registername,def);  
   }  
  else
   {
      //std::cout <<"******** Manager::CommandRegistration unregister "<<registername << std::endl;
      fxRegistry->UnRegisterModuleCommand(registername); 
   }    
   
}

int dabc::xd::Manager::NumNodes()
{
return(fxApplication->NumberOfAllNodes());
}
        
int dabc::xd::Manager::NodeId() const
{
return (fxApplication->GetLocalID());   
}
        
        
const char* dabc::xd::Manager::GetNodeName(int id)
{
   return (fxApplication->FindManagerName(id)).c_str();
}
        

bool dabc::xd::Manager::HasClusterInfo()
{
return true; // all xdaq nodes have configuration info. do we need runtime info?   
}
         
bool dabc::xd::Manager::IsNodeActive(int num)
{
return (fxApplication->CheckApplicationActive(num));   
}
         
bool dabc::xd::Manager::Subscribe(Parameter* par, int remnode, const char* remname)
{
try
{   
   std::string fulldimname=fxApplication->FindApplicationPrefix(remnode)+remname;
   return (fxRegistry->SubscribeParameter(par,fulldimname));   
}
catch(dabc::xd::Exception& e)
   {
      std::cout <<"SubscribeParameter could not find application prefix for node id :"<<remnode <<", got exception "<<e.what()<< std::endl;  
      return false;
   }
   
   
      
}
bool dabc::xd::Manager::Unsubscribe(Parameter* par)
{
return (fxRegistry->UnsubscribeParameter(par));     
}
         
    
bool dabc::xd::Manager::CanSendCmdToManager(const char*)
{
return true;
//return (fxApplication->CanSendManagerCommand());      
}



bool dabc::xd::Manager::InvokeStateTransition(const char* state_transition_name, Command* cmd)
{
return (fxApplication->InvokeStateTransition(state_transition_name, cmd));         
}

