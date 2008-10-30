#include "dabc/logging.h"

#include "dabc/FileIO.h"
#include "dabc/ModuleAsync.h"
#include "dabc/ModuleSync.h"
#include "dabc/Port.h"
#include "dabc/PoolHandle.h"
#include "dabc/Buffer.h"
#include "dabc/Manager.h"
#include "dabc/timing.h"

#include "mbs/MbsEventAPI.h"
#include "mbs/MbsTypeDefs.h"
#include "mbs/MbsDataInput.h"
#include "mbs/MbsTransport.h"
#include "mbs/MbsDevice.h"

#define BUFFERSIZE 16*1024


class GeneratorModule : public dabc::ModuleSync {
   protected:
      dabc::PoolHandle*       fPool; 
      int                       fEventCnt;
      int                       fBufferCnt;
    
   public:
      GeneratorModule(dabc::Manager* mgr, const char* name) :
         dabc::ModuleSync(mgr, name),
         fPool(0),
         fEventCnt(1),
         fBufferCnt(0)
         
      {
         SetSyncCommands(true); 
    
         fPool = CreatePool("Pool", 10, BUFFERSIZE);
   
         CreateOutput("Output", fPool, 5);
      }


      virtual void AfterModuleStop()
      {
         DOUT1(("GeneratorModule %s stoped, generated %d bufs %d evnts", GetName(), fBufferCnt, fEventCnt));
      }

      virtual void MainLoop()
      {
         while (TestWorking()) {
      
            dabc::Buffer* buf = TakeBuffer(fPool, BUFFERSIZE);
            
//            mbs::GenerateMbsPacket(buf, 11, fEventCnt, 120, 8);

            mbs::GenerateMbsPacketForGo4(buf, fEventCnt);
            
            Send(Output(0), buf);
            
            DOUT4(("Send packet %d with event %d", fBufferCnt, fEventCnt));
            
            fBufferCnt++;
         }
      }
};   

class TestModuleAsync : public dabc::ModuleAsync {
   protected:
      dabc::PoolHandle* fPool; 
      dabc::Port*         fInput;
      long                fCounter;
      dabc::Ratemeter     fRate;
      
   public:
      TestModuleAsync(dabc::Manager* mgr, const char* name) : 
         dabc::ModuleAsync(mgr, name),
         fPool(0),
         fInput(0)
      {
         fPool = CreatePool("Pool1", 5, BUFFERSIZE);
          
         fInput = CreateInput("Input", fPool, 5);
         
         fCounter = 0;
         
         fRate.Reset();
      }
      
      void ProcessInputEvent(dabc::Port* port) 
      { 
         dabc::Buffer* buf = 0;
         fInput->Recv(buf);
         
         fRate.Packet(buf->GetTotalSize());
         dabc::Buffer::Release(buf);
         fCounter++;
      }
      
      virtual void AfterModuleStop()
      {
         DOUT1(("TestModuleAsync %s stops %ld Rate: %5.1f MB/s", GetName(), fCounter, fRate.GetRate()));
      }
};

void small_test()
{
   for (int n=0;n<100;n++) {
      
      DOUT1(("Test %d", n));
      
      dabc::CommandsQueue q;
      
      dabc::Command* cmd = 0; 
       
      for (int k=0;k<100;k++) {
         cmd = new dabc::Command("Test");
         q.Push(cmd);
      }
        
      while ((cmd = q.Pop()) != 0)
         dabc::Command::Finalise(cmd);
   }
}

int main(int numc, char* args[])
{
   dabc::SetDebugLevel(1);
   
   //small_test();
   
   dabc::Manager mgr("local");

   if (numc==1) {
      DOUT1(("Start as server"));
      
      if (!mgr.CreateDevice("MbsDevice", "MBS")) {
         EOUT(("MbsDevice cannot be created - halt"));
         return 1;   
      }

      dabc::Module* m = new GeneratorModule(&mgr, "Generator");
      mgr.MakeThreadForModule(m);
      mgr.CreateMemoryPools();
      
      dabc::Command* cmd = new dabc::CmdCreateTransport();
      cmd->SetInt("ServerKind", mbs::TransportServer);
//      cmd->SetInt("PortMin", 6002);
//      cmd->SetInt("PortMax", 7000);
      cmd->SetUInt("BufferSize", BUFFERSIZE);
      if (!mgr.CreateTransport("MBS", "Modules/Generator/Ports/Output", cmd)) {
         EOUT(("Cannot create MBS transport server"));
         return 0;
      }
      
//      if (!mgr.Execute(mgr.LocalCmd(cmd, "Devices/MBS"))) {  
//         EOUT(("Cannot create MBS transport server"));
//         return 0;
//      }
      
      m->Start();

      DOUT1(("Start endless loop"));
      
      while (true)
         dabc::LongSleep(30);
      
//      m->Stop();
      
   } else {
      const char* hostname = args[1];
      int nport = numc>2 ? atoi(args[2]) : -1;

      DOUT1(("Start as client %s:%d", hostname, nport));
      
      dabc::Module* m = new TestModuleAsync(&mgr, "Receiever");

      mgr.MakeThreadForModule(m);

      mgr.CreateMemoryPools();
      
      dabc::Command* cmd = new dabc::CmdCreateDataTransport();
      if (nport>0) cmd->SetInt("Port", nport);
      
      if (!mgr.CreateDataInputTransport("Modules/Receiever/Ports/Input", "MBSInp",
                                        /*"MbsNewTransport"*/ "MbsTransport" /* "MbsStream" */, hostname, cmd)) {
         EOUT(("Cannot create data input for receiever"));
         return 1;  
      }
      
      m->Start();
      
      dabc::LongSleep(3);
      
//      m->Stop();
   }

//   dabc::SetDebugLevel(10);
   
//   mgr.CleanupManager();
   
   return 0;
}
