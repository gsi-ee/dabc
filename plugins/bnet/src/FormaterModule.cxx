#include "bnet/FormaterModule.h"

#include "dabc/logging.h"
#include "dabc/string.h"
#include "dabc/PoolHandle.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/Port.h"
#include "dabc/Parameter.h"

#include "bnet/WorkerApplication.h"

bnet::FormaterModule::FormaterModule(dabc::Manager* mgr, 
                                     const char* name, 
                                     WorkerApplication* plugin) : 
   dabc::ModuleSync(mgr, name),
   fNumReadout(1),
   fModus(0)
{
   fModus = plugin->CombinerModus(); 
   fNumReadout = plugin->NumReadouts();

   fInpPool = CreatePool(plugin->ReadoutPoolName());

   fOutPool = CreatePool(plugin->TransportPoolName());

   fOutPort = CreateOutput("Output", fOutPool, plugin->CombinerOutQueueSize(), sizeof(bnet::SubEventNetHeader));
   
   for (int n=0;n<fNumReadout;n++)
      CreateInput(FORMAT(("Input%d", n)), fInpPool, plugin->CombinerInQueueSize());
}

bnet::FormaterModule::~FormaterModule()
{
}

