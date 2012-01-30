#include "ezca/Factory.h"

#include "dabc/Command.h"
#include "dabc/logging.h"

#include "ezca/Definitions.h"
#include "ezca/EpicsInput.h"
#include "mbs/CombinerModule.h"
#include "ezca/ReadoutApplication.h"


const char* ezca::typeEpicsInput 			= EZCA_typeEpicsInput;
const char* ezca::nameReadoutAppClass   		= EZCA_nameReadoutAppClass;
const char* ezca::nameReadoutModuleClass   	= EZCA_nameReadoutModuleClass;

const char* ezca::xmlEpicsName				= EZCA_xmlEpicsName;

const char* ezca::xmlUpdateFlagRecord 			=  EZCA_xmlUpdateFlagRecord;

const char* ezca::xmlEventIDRecord				=  EZCA_xmlEventIDRecord;

const char* ezca::xmlNumLongRecords				= 	EZCA_xmlNumLongRecords;

const char* ezca::xmlNameLongRecords				=	EZCA_xmlNameLongRecords;

const char* ezca::xmlNumDoubleRecords				= 	EZCA_xmlNumDoubleRecords;

const char* ezca::xmlNameDoubleRecords			=	EZCA_xmlNameDoubleRecords;

const char* ezca::xmlTimeout					=  EZCA_xmlTimeout;

const char* ezca::xmlEpicsSubeventId= EZCA_xmlEpicsSubeventId;

const char* ezca::xmlModuleName 				= EZCA_xmlModuleName; // Name of readout module instance
const char* ezca::xmlModuleThread 			= EZCA_xmlModuleName; // Name of thread for readout module






ezca::Factory epicsfactory("ezca");

ezca::Factory::Factory(const char* name) :
   dabc::Factory(name)
{
}

dabc::Application* ezca::Factory::CreateApplication(const char* classname, dabc::Command cmd)
{
   if (strcmp(classname, ezca::nameReadoutAppClass)==0)
      return new ezca::ReadoutApplication();

   return dabc::Factory::CreateApplication(classname, cmd);
}

dabc::Module* ezca::Factory::CreateModule(const char* classname, const char* modulename, dabc::Command cmd)
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

dabc::Device* ezca::Factory::CreateDevice(const char* classname, const char* devname, dabc::Command cmd)
{
   return dabc::Factory::CreateDevice(classname, devname, cmd);
}
