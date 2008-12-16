#include "bnet/FormaterModule.h"

#include "dabc/logging.h"
#include "dabc/string.h"
#include "dabc/PoolHandle.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/Port.h"
#include "dabc/Parameter.h"

#include "bnet/common.h"

bnet::FormaterModule::FormaterModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleSync(name, cmd)
{
   fNumReadouts = GetCfgInt(xmlNumReadouts, 1, cmd);
   fModus = GetCfgInt(xmlCombinerModus, 0, cmd);

   fInpPool = CreatePoolHandle(GetCfgStr(CfgReadoutPool, ReadoutPoolName, cmd).c_str());

   fOutPool = CreatePoolHandle(bnet::TransportPoolName);

   fOutPort = CreateOutput("Output", fOutPool, SenderInQueueSize, sizeof(bnet::SubEventNetHeader));

   for (int n=0;n<NumReadouts();n++)
      CreateInput(FORMAT(("Input%d", n)), fInpPool, ReadoutQueueSize);
}
