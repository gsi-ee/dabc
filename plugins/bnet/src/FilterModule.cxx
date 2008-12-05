#include "bnet/FilterModule.h"

#include "bnet/common.h"

bnet::FilterModule::FilterModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleSync(name, cmd)
{
   fPool = CreatePool(bnet::EventPoolName);

   CreateInput("Input", fPool, FilterInpQueueSize);

   CreateOutput("Output", fPool, FilterOutQueueSize);
}

void bnet::FilterModule::MainLoop()
{
   while (TestWorking()) {
      dabc::BufferGuard buf = Recv(Input(0));

      bool dosend = false;
      bool dostop = false;

      // we just retranslate EOL buffer and stop e

      if (buf()!=0)
         if (buf()->GetTypeId()==dabc::mbt_EOL) {
            dosend = true;
            dostop = true;
         }
         else
           if (buf()->GetTypeId()!=dabc::mbt_EOF)
              dosend = TestBuffer(buf());

      if (dosend)
         Send(Output(0), buf);
      else
         buf.Release();

      if (dostop) {
         DOUT1(("Stop filter execution until restart"));

         StopUntilRestart();
      }
   }
}
