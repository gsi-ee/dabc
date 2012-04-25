#include "verbs/Factory.h"

#include "dabc/Manager.h"

#include "verbs/BnetRunnable.h"
#include "verbs/Device.h"
#include "verbs/Thread.h"

dabc::FactoryPlugin verbsfactory(new verbs::Factory("verbs"));

dabc::Reference verbs::Factory::CreateObject(const char* classname, const char* objname, dabc::Command cmd)
{
   if (strcmp(classname, "verbs::BnetRunnable")==0)
      return new verbs::BnetRunnable(objname);

   return 0;
}

dabc::Device* verbs::Factory::CreateDevice(const char* classname,
                                                const char* devname,
                                                dabc::Command cmd)
{
   if (strcmp(classname, "verbs::Device")!=0)
      return dabc::Factory::CreateDevice(classname, devname, cmd);

   return new verbs::Device(devname);
}

dabc::Reference verbs::Factory::CreateThread(dabc::Reference parent, const char* classname, const char* thrdname, const char* thrddev, dabc::Command cmd)
{
   if ((classname==0) || (strcmp(classname, VERBS_THRD_CLASSNAME)!=0)) return dabc::Reference();

   if (thrddev==0) {
      EOUT(("Device name not specified to create verbs thread"));
      return dabc::Reference();
   }

   verbs::DeviceRef dev = dabc::mgr.FindDevice(thrddev);

   if (dev.null()) {
      EOUT(("Did not found verbs device with name %s", thrddev));
      return dabc::Reference();
   }

   verbs::Thread* thrd = new verbs::Thread(parent, dev()->IbContext(), thrdname);

   return dabc::Reference(thrd);
}
