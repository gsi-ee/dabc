#include "bnet/BuilderModule.h"

#include "dabc/logging.h"
#include "dabc/PoolHandle.h"
#include "dabc/Command.h"
#include "dabc/Port.h"
#include "dabc/Parameter.h"

#include "bnet/WorkerApplication.h"

bnet::BuilderModule::BuilderModule(dabc::Manager* m, const char* name, WorkerApplication* factory) :
   dabc::ModuleSync(m, name),
   fInpPool(0),
   fOutPool(0),
   fNumSenders(1)
{
   SetSyncCommands(true); 
    
   fOutPool = CreatePool(factory->EventPoolName());

   CreateOutput("Output", fOutPool, BuilderOutQueueSize);

   fInpPool = CreatePool(factory->TransportPoolName());

   CreateInput("Input", fInpPool, BuilderInpQueueSize, sizeof(bnet::SubEventNetHeader));

   fOutBufferSize = factory->EventBufferSize();
   
   new dabc::StrParameter(this, "SendMask", "xxxx");
}

bnet::BuilderModule::~BuilderModule()
{
   for (unsigned n=0; n<fBuffers.size(); n++)
      dabc::Buffer::Release(fBuffers[n]);
   fBuffers.clear();
}

void bnet::BuilderModule::MainLoop()
{
   DOUT3(("In builder %u buffers collected",  fBuffers.size()));
   
   fNumSenders = bnet::NodesVector(GetParCharStar("SendMask")).size();
    
   while (TestWorking()) {
      
      bool iseol = false;
      
      // read N buffers from input
      while (fBuffers.size() < (unsigned) fNumSenders) {
         dabc::Buffer* buf = Recv(Input(0));
         if (buf==0)
            EOUT(("Cannot read buffer from input")); 
         else {
            fBuffers.push_back(buf);
            
            if (buf->GetTypeId() == dabc::mbt_EOL) {
               iseol = true;
               if (fBuffers.size() > 1) EOUT(("EOL buffer is not first !!!!"));
               break;
            }
         }
      }
      
      if (iseol) {
         DOUT1(("Send EOL buffer to the output")); 
         dabc::BufferGuard eolbuf = fOutPool->TakeEmptyBuffer();
         eolbuf()->SetTypeId(dabc::mbt_EOL);
         Send(Output(0), eolbuf);
      } else 
         // produce output
         DoBuildEvent(fBuffers);
      
      // release N buffers
      for (unsigned n=0;n<fBuffers.size();n++) 
         dabc::Buffer::Release(fBuffers[n]);
      fBuffers.clear();
   }

}
