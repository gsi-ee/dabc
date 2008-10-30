#include "bnet/LocalDFCModule.h"

#include "bnet/WorkerPlugin.h"

#include "dabc/Port.h"
#include "dabc/Buffer.h"

bnet::LocalDFCModule::LocalDFCModule(dabc::Manager* mgr, const char* name, WorkerPlugin* factory) : 
   dabc::ModuleAsync(mgr, name),
   fPool(0)
{
   fPool = CreatePool(factory->ControlPoolName());
}

