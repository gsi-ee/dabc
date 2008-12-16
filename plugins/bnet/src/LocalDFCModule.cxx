#include "bnet/LocalDFCModule.h"

#include "dabc/Port.h"
#include "dabc/Buffer.h"

#include "bnet/common.h"

bnet::LocalDFCModule::LocalDFCModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleAsync(name, cmd),
   fPool(0)
{
   fPool = CreatePoolHandle(bnet::ControlPoolName);
}

