#include "mbs/Factory.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"

#include "mbs/EventAPI.h"
#include "mbs/LmdInput.h"
#include "mbs/LmdOutput.h"
#include "mbs/Device.h"

mbs::Factory mbsfactory("mbs");

dabc::Device* mbs::Factory::CreateDevice(const char* classname, const char* devname, dabc::Command*)
{
   if (strcmp(classname, "mbs::Device")!=0) return 0;

   DOUT5(("Creating MBS device with name %s", devname));

   return new mbs::Device(dabc::mgr()->GetDevicesFolder(true), devname);
}

dabc::DataInput* mbs::Factory::CreateDataInput(const char* typ, const char* name, dabc::Command* cmd)
{
   if ((typ==0) || (strlen(typ)==0)) return 0;

   DOUT3(("Factory::CreateDataInput %s", typ));

   if (strcmp(typ, typeLmdInput)==0) {
      return new mbs::LmdInput();
   } else
   if (strcmp(typ, xmlEvapiType) == 0) {
      return new mbs::EvapiInput(xmlEvapiFile, name);
   } else
   if (strcmp(typ, xmlEvapiFile) == 0) {
      return new mbs::EvapiInput(xmlEvapiFile, name);
   } else
   if (strcmp(typ, xmlEvapiRFIOFile) == 0) {
      return new mbs::EvapiInput(xmlEvapiRFIOFile, name);
   } else
   if (strcmp(typ, xmlEvapiTransportServer) == 0) {
      return new mbs::EvapiInput(xmlEvapiTransportServer, name);
   } else
   if (strcmp(typ, xmlEvapiStreamServer) == 0) {
      return new mbs::EvapiInput(xmlEvapiStreamServer, name);
   } else
   if (strcmp(typ, xmlEvapiEventServer) == 0) {
      return new mbs::EvapiInput(xmlEvapiEventServer, name);
   } else
   if (strcmp(typ, xmlEvapiRemoteEventServer) == 0) {
      return new mbs::EvapiInput(xmlEvapiRemoteEventServer, name);
   }

   return 0;
}

dabc::DataOutput* mbs::Factory::CreateDataOutput(const char* typ, const char* name, dabc::Command* cmd)
{

   if ((typ==0) || (strlen(typ)==0)) return 0;

   DOUT3(("Factory::CreateDataOutput typ:%s", typ));

   if (strcmp(typ, typeLmdOutput)==0) {
      return new mbs::LmdOutput();
   } else
   if (strcmp(typ, xmlEvapiOutFile) == 0) {
      return new mbs::EvapiOutput(name);
   }

   return 0;
}
