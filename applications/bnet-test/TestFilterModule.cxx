#include "TestFilterModule.h"

#include "dabc/logging.h"
#include "dabc/PoolHandle.h"
#include "dabc/Command.h"
#include "dabc/Port.h"

#include "bnet/common.h"

bnet::TestFilterModule::TestFilterModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleAsync(name, cmd)
{
   fPool = CreatePoolHandle(bnet::EventPoolName);

   CreateInput("Input", fPool, FilterInpQueueSize);

   CreateOutput("Output", fPool, FilterOutQueueSize);
}

void bnet::TestFilterModule::ProcessUserEvent(dabc::ModuleItem*, uint16_t)
{
//   DOUT1(("!!!!!!!! ProcessInputEvent port = %p", port));

   while (!Input(0)->InputBlocked() && !Output(0)->OutputBlocked()) {

      dabc::Buffer* buf = 0;
      Input(0)->Recv(buf);

      if (buf==0) {
         EOUT(("Fail to receive data from Input"));
         return;
      }

      uint64_t* mem = (uint64_t*) buf->GetDataLocation();
      uint64_t evid = mem[0];

      if (evid % 2) {
    //   DOUT1(("EVENT KILLED %llu", evid));
        dabc::Buffer::Release(buf);
      } else
          Output(0)->Send(buf);
   }
}
