#include "dabc/AbbWriterModule.h"
#include "dabc/PCIBoardCommands.h"


#include "dabc/logging.h"
#include "dabc/PoolHandle.h"
#include "dabc/MemoryPool.h"
#include "dabc/Command.h"
#include "dabc/Port.h"
#include "dabc/Pointer.h" 
#include "dabc/Manager.h" 

dabc::AbbWriterModule::AbbWriterModule(dabc::Manager* mgr, const char* name, 
                                         dabc::Command* cmd) 
   : dabc::ModuleSync(mgr, name), fPool(0), fStandalone(true), fBufferSize(0)
{
         fBufferSize = cmd->GetInt(ABB_COMPAR_BUFSIZE, 16384);
         int queuelen = cmd->GetInt(ABB_COMPAR_QLENGTH, 10);
         fStandalone = (bool) cmd->GetInt(ABB_COMPAR_STALONE, 1);
         const char* poolname=cmd->GetStr(ABB_COMPAR_POOL,"ABBWriterPool");
         DOUT1(("new AbbWriterModule %s buff %d queue %d, poolname=%s, standalone=%d", GetName(), fBufferSize, queuelen,poolname, fStandalone));
         if(fStandalone)
            fPool = CreatePool(poolname, 200, fBufferSize); // specify pool
         else
            fPool = CreatePool(poolname); // use external pool of connected data sender      
   CreateOutput("Output", fPool, queuelen);
   CreateRateParameter("DMAWriter", false, 1., "Output","");
   
   if(!fStandalone) CreateInput("Input", fPool, queuelen);
   
   fWriteRate.Reset();
   
   
}

void dabc::AbbWriterModule::BeforeModuleStart()
{
    DOUT1(("\n\nAbbWriterModule::BeforeModuleStart, fStandalone = %d", fStandalone));
 
}

void dabc::AbbWriterModule::AfterModuleStop()
{
   DOUT1(("\nAbbWriterModule finish Rate %5.1f numoper:%7ld  tm:%7.5f \n\n", fWriteRate.GetRate(), fWriteRate.GetNumOper(), fWriteRate.GetTotalTime()));
}


void dabc::AbbWriterModule::MainLoop()
{
while (TestWorking()) 
   {
   dabc::Buffer* ref = 0;   
   try
      {
         if(fStandalone)
               {
                  ref=TakeBuffer(fPool, fBufferSize);
               }
            else
               {   
                   ref=Recv(Input(0));
               }
          int length=ref->GetDataSize(); // ref is gone after Send...      
          Send(Output(0),ref);
          fWriteRate.Packet(length);      
      }
   catch(dabc::Exception& e)
      {
          DOUT1(("AbbWriterModule::MainLoop - raised dabc exception %s", e.what()));
          dabc::Buffer::Release(ref); 
          // how do we treat this?
          Stop();
      }
   catch(std::exception& e)
      {
          DOUT1(("AbbWriterModule::MainLoop - raised std exception %s ", e.what()));
          dabc::Buffer::Release(ref);
          Stop();                      
      }
   catch(...)
      {
          DOUT1(("AbbWriterModule::MainLoop - Unexpected exception!!!"));
          throw;                     
      }   
   } // end while
}




