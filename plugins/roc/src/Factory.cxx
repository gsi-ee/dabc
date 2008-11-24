#include "roc/Factory.h"
#include "roc/CombinerModule.h"
#include "roc/CalibrationModule.h"
#include "roc/Device.h"
#include "roc/TreeOutput.h"
#include "roc/ReadoutApplication.h"

#include "dabc/Command.h"
#include "dabc/logging.h"


roc::Factory rocfactory("roc");

dabc::Application* roc::Factory::CreateApplication(const char* classname, const char* appname, dabc::Command* cmd)
{
   if (strcmp(classname,"RocReadoutApp")==0)
      return new roc::ReadoutApplication(appname);

   return dabc::Factory::CreateApplication(classname, appname, cmd);
}

dabc::Module* roc::Factory::CreateModule(const char* classname, const char* modulename, dabc::Command* cmd)
{
   DOUT1(("roc::Factory::CreateModule called for class:%s, module:%s", classname, modulename));

   if (strcmp(classname, "roc::CombinerModule")==0) {
      dabc::Module* mod= new roc::CombinerModule(modulename,cmd);
      DOUT1(("roc::Factory::CreateModule - Created RocCombiner module %s ", modulename));
      return mod;
   } else
   if (strcmp(classname, "roc::CalibrationModule")==0) {
      dabc::Module* mod = new roc::CalibrationModule(modulename, cmd);
      DOUT1(("roc::Factory::CreateModule - Created roc::CalibrationModule module %s ", modulename));
      return mod;
   }

   return 0;
}

dabc::Device* roc::Factory::CreateDevice(dabc::Basic* parent, const char* classname, const char* devname, dabc::Command* cmd)
{
   if (strcmp(classname,"roc::Device")!=0) return 0;

   DOUT1(("roc::Factory::CreateDevice - Creating  ROC device %s ...", devname));
   roc::Device* dev = new  roc::Device(parent, devname);
   return dev;
}

dabc::DataOutput* roc::Factory::CreateDataOutput(const char* typ, const char* name, dabc::Command* cmd)
{
   if (strcmp(typ, "roc::TreeOutput")!=0) return 0;

   roc::TreeOutput* out = new roc::TreeOutput(name, cmd->GetInt("SizeLimit", 0));

   if (!out->IsOk()) { delete out; out = 0; }

   return out;
}
