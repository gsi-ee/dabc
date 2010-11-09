#include "ezca/Factory.h"

#include "dabc/Command.h"
#include "dabc/logging.h"

#include "ezca/Definitions.h"
#include "ezca/EpicsInput.h"
#include "mbs/CombinerModule.h"
#include "ezca/ReadoutApplication.h"


const char* ezca::typeEpicsInput 			= "ezca::EpicsInput";
const char* ezca::nameReadoutAppClass   		= "ezca::Readout";
const char* ezca::nameReadoutModuleClass   	= "ezca::MonitorModule";

const char* ezca::xmlEpicsName				= "EpicsIdentifier";

const char* ezca::xmlUpdateFlagRecord 			=  "EpicsFlagRec";

const char* ezca::xmlEventIDRecord				=  "EpicsEventIDRec";

const char* ezca::xmlNumLongRecords				= 	"EpicsNumLongRecs";

const char* ezca::xmlNameLongRecords				=	"EpicsLongRec-";

const char* ezca::xmlNumDoubleRecords				= 	"EpicsNumDoubleRecs";

const char* ezca::xmlNameDoubleRecords			=	"EpicsDoubleRec-";

const char* ezca::xmlTimeout					=  "EpicsPollingTimeout";

const char* ezca::xmlEpicsSubeventId= "EpicsSubeventId";

const char* ezca::xmlModuleName 				= "EpicsModuleName"; // Name of readout module instance
const char* ezca::xmlModuleThread 			= "EpicsModuleThread"; // Name of thread for readout module






ezca::Factory epicsfactory("ezca");

ezca::Factory::Factory(const char* name) :
   dabc::Factory(name)
{
}

dabc::Application* ezca::Factory::CreateApplication(const char* classname, dabc::Command* cmd)
{
   if (strcmp(classname, ezca::nameReadoutAppClass)==0)
      return new ezca::ReadoutApplication();

   return dabc::Factory::CreateApplication(classname, cmd);
}

dabc::Module* ezca::Factory::CreateModule(const char* classname, const char* modulename, dabc::Command* cmd)
{
   DOUT2(("ezca::Factory::CreateModule called for class:%s, module:%s", classname, modulename));

   if (strcmp(classname, ezca::nameReadoutModuleClass)==0) {
      DOUT0(("ezca::Factory::CreateModule - Created Mbs Combiner as Epics Monitor module %s ", modulename));
           return new mbs::CombinerModule(modulename,cmd);
   }

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}

dabc::DataInput* ezca::Factory::CreateDataInput(const char* typ)
{
   if ((typ==0) || (strlen(typ)==0)) return 0;
   DOUT3(("ezca::Factory::CreateDataInput %s", typ));

   if (strcmp(typ, typeEpicsInput) == 0)
      return new ezca::EpicsInput();

   return 0;
}

dabc::Device* ezca::Factory::CreateDevice(const char* classname, const char* devname, dabc::Command* cmd)
{

   return dabc::Factory::CreateDevice(classname, devname, cmd);
}
