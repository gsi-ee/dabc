#include "bnet/FilterModule.h"

#include "bnet/common.h"

bnet::FilterModule::FilterModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleSync(name, cmd)
{
   fPool = CreatePoolHandle(bnet::EventPoolName);

   CreateInput("Input", fPool, FilterInpQueueSize);

   CreateOutput("Output", fPool, FilterOutQueueSize);
}

void bnet::FilterModule::MainLoop()
{
   while (ModuleWorking()) {

      DOUT4(("Start recv"));

      dabc::BufferGuard buf = Recv(Input(0));

      DOUT4(("Did recv %p", buf()));


      bool dosend = false;
      bool dostop = false;

      // we just retranslate EOL buffer and stop execution

      if (buf()!=0){
         if (buf()->GetTypeId()==dabc::mbt_EOL) {
            dosend = true;
            dostop = true;
         } else
         if (buf()->GetTypeId()!=dabc::mbt_EOF)
            dosend = TestBuffer(buf());
      }
      if (dosend) {
         DOUT4(("Start send"));
         Send(Output(0), buf);
         DOUT4(("Did send"));
      } else
         buf.Release();

      if (dostop) {
         DOUT1(("Stop filter execution until restart"));

         StopUntilRestart();
      }
   }
}
