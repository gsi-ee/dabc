/* Generated by Together */

#include "DABCNode.h"

#include "DABCManager.h"
#include "DABCRegistry.h"

#include "dabc/Exception.h"
#include "DABCExceptions.h"



dabc::xd::Node::Node(xdaq::ApplicationStub* stub)
	throw (xdaq::exception::Exception)
	: dabc::xd::Application(stub),
   fxModMan(0), fiNumAllNodes(0),fxStateChangeCommand(0)
{

nodeId_=1;
workerTid_=42;
controllerTid_=41;

Registry()->RegisterSerializable("nodeId",    &nodeId_ ,  false);
Registry()->RegisterSerializable("workerTid", &workerTid_ , false);
Registry()->RegisterSerializable("controllerTid", &controllerTid_ , false);

Registry()->DefineDIMCommand(_DABC_COMMAND_MANAGER_);
InitManager();
}


dabc::xd::Node::~Node()
{
delete   fxModMan;

}

void dabc::xd::Node::Shutdown()
{
//std::cout <<"********* dabc::xd::Node::Shutdown()..." << std::endl;
Manager()->HaltManager(); // clean up all core objects
dabc::xd::Application::Shutdown(); // terminate xdaq
}



void dabc::xd::Node::InitManager()
{
//std::cout <<"********* ENTER  InitManager before lockguard" << std::endl;
if(fxModMan==0)
   {
      std::string mgrname=Registry()->GetDIMServerName();
      //std::cout <<"********* creating modules manager "<<mgrname << std::endl;
      fxModMan= new   dabc::xd::Manager(this, mgrname.c_str());
      fxModMan->CreateApplication();
   }
//std::cout <<"********* LEAVE  InitManager" << std::endl;
}

void dabc::xd::Node::ExecuteManagerCommand(std::string parameter)
{
         Manager()->ProcessRemoteCommand(parameter.c_str());
}


bool dabc::xd::Node::SendManagerCommand(std::string managername, std::string parameter, bool async)
{
try {
   std::string commandname=_DABC_COMMAND_MANAGER_;
   int nodeid=FindNodeID(managername);
//std::cout <<"SendManagerCommand with string:"<<parameter << std::endl;
   xdaq::ApplicationDescriptor* app=FindApplication(workerTid_,nodeid);
   if(app==0) app=FindApplication(controllerTid_,nodeid);
   if(app==0){
      std::cout <<"SendManagerCommand could not find nodeid "<<nodeid << std::endl;
      return false;
   }
   if(async)
      Registry()->SendDIMCommandAsync(app,commandname,parameter);
   else
      Registry()->SendDIMCommand(app,commandname,parameter);
      // we may wait here until some dim service has been switched?
    }
catch(xdaq::exception::Exception& e)
   {
      std::cout <<"could not find application descriptor for manager:"<<managername << std::endl;
      return false;
   }
catch(std::exception& e)
   {
          std::cout <<"SendManagerCommand std exception "<<e.what()<<" for manager:"<<managername <<std::endl;
          return false;
   }
catch(...)
   {
      std::cout <<"SendManagerCommand Unexpected exception for manager:"<<managername << std::endl;
      return false;
   }
return true;

}




void dabc::xd::Node::ExecuteLocalCommand(const std::string& com, const std::string& par)
{
std::string rname=com; // command was already reduced before!
//std::cout <<"reduced command: "<< rname << std::endl;
if(com==_DABC_COMMAND_MANAGER_)
   {
      //std::cout <<"ModuleCommand string:"<<par <<":"<< std::endl;
      ExecuteManagerCommand(par);
   }
else if(Registry()->FindModuleCommand(rname))
   {
      //std::string par=com->getString();
      std::cout <<"Execute Registered ModuleCommand "<< rname <<", string="<<par <<":"<< std::endl;
      std::string modname;
      std::string varname;
      Registry()->ParseFullParameterName(rname,modname, varname);
      dabc::Command* dabccom= new dabc::Command(varname.c_str());
      std::cout <<" -  commandname is "<<varname << std::endl;
      std::cout <<" -  modulename is "<<modname << std::endl;
      std::string fullmod="Modules/";
      fullmod+=modname;
      // TODO: fill xml par string to dabccom (special ctor later)
      CommandClient dummyclient;
      Manager()->SubmitLocal(dummyclient, dabccom, fullmod.c_str());
   }
else
   {
      dabc::xd::Application::ExecuteLocalCommand(com,par);
   }


}





void dabc::xd::Node::DoConfigure(toolbox::Event::Reference e)
{
	//std::cout <<"--------- dabc::xd::Node::DoConfigure" << std::endl;
try
{
   if(!Manager()->DoStateTransition(dabc::Manager::stcmdDoConfigure))
          throw (toolbox::fsm::exception::Exception( "DoStateTransition failure", "Error in state transition DoConfigure" , __FILE__, __LINE__, __FUNCTION__ ));

   //std::cout <<"returned from DoStateTransition()" << std::endl;
  }
catch (dabc::Exception& e)
{
   StatusMessage(toolbox::toString("dabc CORE Exception in  %s: %s", __FUNCTION__, e.what()), dabc::xd::nameParser::ERR);
   throw (toolbox::fsm::exception::Exception( "CORE Exception", "Error in state transition" , __FILE__, __LINE__, __FUNCTION__ ));
}

}
void dabc::xd::Node::DoEnable(toolbox::Event::Reference )
{
//std::cout <<"--------- dabc::xd::Node::DoEnable" << std::endl;
try
{
    if(!Manager()->DoStateTransition(dabc::Manager::stcmdDoEnable))
           throw (toolbox::fsm::exception::Exception( "DoStateTransition failure", "Error in state transition DoEnable" , __FILE__, __LINE__, __FUNCTION__ ));

}
catch (dabc::Exception& e)
{
   StatusMessage(toolbox::toString("dabc CORE Exception in  %s: %s", __FUNCTION__, e.what()), dabc::xd::nameParser::ERR);
   throw (toolbox::fsm::exception::Exception( "CORE Exception", "Error in state transition" , __FILE__, __LINE__, __FUNCTION__ ));

 }

}
void dabc::xd::Node::DoHalt(toolbox::Event::Reference )
{
//std::cout <<"--------- dabc::xd::Node::DoHalt" << std::endl;
try
{
   Manager()->DoStateTransition(dabc::Manager::stcmdDoStop);
   if(!Manager()->DoStateTransition(dabc::Manager::stcmdDoHalt))
          throw (toolbox::fsm::exception::Exception( "DoStateTransition failure", "Error in state transition DoHalt" , __FILE__, __LINE__, __FUNCTION__ ));

}
catch (dabc::Exception& e)
{
   StatusMessage(toolbox::toString("dabc CORE Exception in  %s: %s", __FUNCTION__, e.what()), dabc::xd::nameParser::ERR);
   throw (toolbox::fsm::exception::Exception( "CORE Exception", "Error in state transition" , __FILE__, __LINE__, __FUNCTION__ ));

 }

}




void dabc::xd::Node::DoStop(toolbox::Event::Reference )
{
//std::cout <<"--------- dabc::xd::Node:::DoStop" << std::endl;
try
{
   if(!Manager()->DoStateTransition(dabc::Manager::stcmdDoStop))
          throw (toolbox::fsm::exception::Exception( "DoStateTransition failure", "Error in state transition DoStop" , __FILE__, __LINE__, __FUNCTION__ ));

}
catch (dabc::Exception& e)
{
   StatusMessage(toolbox::toString("dabc CORE Exception in  %s: %s", __FUNCTION__, e.what()), dabc::xd::nameParser::ERR);
   throw (toolbox::fsm::exception::Exception( "CORE Exception", "Error in state transition" , __FILE__, __LINE__, __FUNCTION__ ));

 }

}

void dabc::xd::Node::DoStart(toolbox::Event::Reference )
{
//std::cout <<"--------- dabc::xd::Node::DoResume" << std::endl;
try
{
   if(!Manager()->DoStateTransition(dabc::Manager::stcmdDoStart))
        throw (toolbox::fsm::exception::Exception( "DoStateTransition failure", "Error in state transition DoStart" , __FILE__, __LINE__, __FUNCTION__ ));

}
catch (dabc::Exception& e)
{
   StatusMessage(toolbox::toString("dabc CORE Exception in  %s: %s", __FUNCTION__, e.what()), dabc::xd::nameParser::ERR);
   throw (toolbox::fsm::exception::Exception( "CORE Exception", "Error in state transition" , __FILE__, __LINE__, __FUNCTION__ ));

 }



}

void dabc::xd::Node::DoError(toolbox::Event::Reference )
{
//std::cout <<"--------- dabc::xd::Node::DoError" << std::endl;
try
{
   if(!Manager()->DoStateTransition(dabc::Manager::stcmdDoError))
        throw (toolbox::fsm::exception::Exception( "DoStateTransition failure", "Error in state transition DoError" , __FILE__, __LINE__, __FUNCTION__ ));

}
catch (dabc::Exception& e)
{
   StatusMessage(toolbox::toString("dabc CORE Exception in  %s: %s", __FUNCTION__, e.what()), dabc::xd::nameParser::ERR);
   throw (toolbox::fsm::exception::Exception( "CORE Exception", "Error in state transition" , __FILE__, __LINE__, __FUNCTION__ ));

 }



}

void dabc::xd::Node::DoResetFailure(toolbox::Event::Reference)
{
//std::cout <<"--------- dabc::xd::Node::DoResetFailure" << std::endl;
try
{
      Manager()->DoStateTransition(dabc::Manager::stcmdDoHalt); // cluster plugin may forward the reset failure by this
}
catch (dabc::Exception& e)
{
   StatusMessage(toolbox::toString("dabc CORE Exception in  %s: %s", __FUNCTION__, e.what()), dabc::xd::nameParser::ERR);
   throw (toolbox::fsm::exception::Exception( "CORE Exception", "Error in state transition" , __FILE__, __LINE__, __FUNCTION__ ));

 }



}

void dabc::xd::Node::DoHandleFailure(toolbox::Event::Reference e)
{
//   std::cout <<"--------- dabc::xd::Node::DoHandleFailure" << std::endl;

}





void dabc::xd::Node::UpdateState()
{
    dabc::xd::Application::UpdateState();
    NotifyStateChangeDone(); // reply core command that triggered state transition
}


void dabc::xd::Node::NotifyStateChangeDone()
{
//std::cout <<"********* NotifyStateChangeDone" << std::endl;
if(fxStateChangeCommand==0) return;
//std::string sname=fsm_.getStateName( fsm_.getCurrentState());
if(GetState()==dabc::Manager::stFailure)
    dabc::Command::Reply(fxStateChangeCommand, false);
else
    dabc::Command::Reply(fxStateChangeCommand, true);
fxStateChangeCommand=0;
}


/////////////////////////////////////////////////
bool dabc::xd::Node::InvokeStateTransition(const char* state_transition_name, dabc::Command* cmd)
{
   // must check if transition allowed here, since we get no message later from fsm :(
if(CheckTransitionPossible(state_transition_name))
      {
         // found transition name allowed for this initial state, do it!
         fxStateChangeCommand=cmd; // do we need a queue here?
         ChangeState(state_transition_name);
         return true;
      }
else
      {
         //std::cout <<"InvokeStateTransition: No allowed transition from "<<fsm_.getCurrentState()<<" with command "<<state_transition_name << std::endl;
         dabc::Command::Reply(cmd, false);
         return false;
      }
}




std::string dabc::xd::Node::FindManagerName(unsigned int instance)
{
std::string result=FindContextName(workerTid_,instance);
if(result.empty())
      result=FindContextName(controllerTid_, instance);
std::string prefix=Registry()->GetServerPrefix();//"XDAQ/";
result=prefix+result;
//std::cout <<"FindManagerName for "<<instance<<" yields "<<result << std::endl;
return result;

}

int dabc::xd::Node::FindNodeID(std::string managername)
{
int result=-1;
std::string::size_type sep=managername.find("/")+1; // first separator
managername.replace(0, sep,""); // remove  dimserver prefix
managername="http://"+managername; // complete url expression
//std::cout <<"FindNodeID searching in context " <<managername << std::endl;
result=FindInstanceID(workerTid_, managername);
if(result<0)
   {
      //std::cout <<"did not find worker in that context, try controller" << std::endl;
      result=FindInstanceID(controllerTid_, managername);
   }
return result;
}

int dabc::xd::Node::NumberOfAllNodes()
{
if(fiNumAllNodes==0)
   {
		fiNumAllNodes=FindNumberOfNodes(workerTid_)+ FindNumberOfNodes(controllerTid_);
		std::cout <<"++++++++ + NumberOfAllNodes evaluates to :"<<fiNumAllNodes  << std::endl;
   }
return fiNumAllNodes;
}

std::string dabc::xd::Node::FindApplicationPrefix(unsigned int instance) throw (dabc::xd::Exception)
{
   xdaq::ApplicationDescriptor* app=FindApplication(workerTid_,instance);
   if(app==0) app=FindApplication(controllerTid_,instance);
   if(app==0){
      std::cout <<"FindApplicationPrefix could not find nodeid "<<instance << std::endl;
      XCEPT_RAISE(dabc::xd::Exception, "Node id not known");
      }
   xdaq::ContextDescriptor* cd=app->getContextDescriptor();
   // now build full command name:
   std::string pre=Registry()->CreateDIMPrefix(cd,app);
   return pre;
}

bool dabc::xd::Node::CheckApplicationActive(unsigned int instance)
{
   // activity check maybe by using soap commands?
// TODO later...

   //
   return true;

}


