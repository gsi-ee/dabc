#include "bnet/SplitterModule.h"

#include "dabc/Port.h"
#include "dabc/Buffer.h"

dabc::SplitterModule::SplitterModule(const char* name, Command cmd) :
   ModuleAsync(name, cmd)
{
   int numout = Cfg(xmlNumOutputs,cmd).AsInt(2);

   CreatePoolHandle(Cfg(xmlPoolName, cmd).AsStr("Pool"));

   CreateInput("Input", Pool(0));

   for (int n=0; n<numout; n++)
      CreateOutput(FORMAT(("Output%d", n)), Pool(0));
}

void dabc::SplitterModule::ProcessItemEvent(dabc::ModuleItem*, uint16_t)
{
   while (Input(0)->CanRecv()) {
      int numconn = 0;

      for (unsigned n=0;n<NumOutputs();n++) {
         if (!Output(n)->CanSend()) return;
         if (Output(n)->IsConnected()) numconn++;
      }

      dabc::Buffer* buf = Input(0)->Recv();
      if (buf==0) {
         EOUT(("Cannot recv buffer from input when expected"));
         return;
      }

      if (numconn>0)
         for (unsigned n=0;n<NumOutputs();n++) {
            if (!Output(n)->IsConnected()) continue;

            dabc::Buffer* sendbuf = 0;

            if (--numconn==0) { sendbuf = buf; buf = 0; }
                         else sendbuf = buf->MakeReference();

            Output(n)->Send(sendbuf);
         }

      dabc::Buffer::Release(buf);
   }
}
