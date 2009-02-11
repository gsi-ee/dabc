#include "roc/Factory.h"
#include "roc/CombinerModule.h"
#include "roc/CalibrationModule.h"
#include "roc/Device.h"
#include "roc/TreeOutput.h"
#include "roc/ReadoutApplication.h"

#include "dabc/Command.h"
#include "dabc/logging.h"

roc::Factory rocfactory("roc");

roc::Factory::Factory(const char* name) :
   dabc::Factory(name)
{
}

dabc::Application* roc::Factory::CreateApplication(const char* classname, dabc::Command* cmd)
{
   if (strcmp(classname, xmlReadoutAppClass)==0)
      return new roc::ReadoutApplication();

   return dabc::Factory::CreateApplication(classname, cmd);
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

dabc::Device* roc::Factory::CreateDevice(const char* classname, const char* devname, dabc::Command* cmd)
{
   if (strcmp(classname,"roc::Device")!=0) return 0;

   DOUT1(("roc::Factory::CreateDevice - Creating  ROC device %s ...", devname));
   roc::Device* dev = new  roc::Device(dabc::mgr()->GetDevicesFolder(true), devname);
   return dev;
}

dabc::DataOutput* roc::Factory::CreateDataOutput(const char* typ)
{
   if (strcmp(typ, "roc::TreeOutput")!=0) return 0;

   return new roc::TreeOutput();
}
