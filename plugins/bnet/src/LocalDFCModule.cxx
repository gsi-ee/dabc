#include "bnet/LocalDFCModule.h"

#include "bnet/WorkerApplication.h"

#include "dabc/Port.h"
#include "dabc/Buffer.h"

bnet::LocalDFCModule::LocalDFCModule(const char* name, WorkerApplication* factory) :
   dabc::ModuleAsync(name),
   fPool(0)
{
   fPool = CreatePool(factory->ControlPoolName());
}

