#include "abb/AbbReadoutModule.h"
#include "pci/PCIBoardCommands.h"


#include "dabc/logging.h"
#include "dabc/PoolHandle.h"
#include "dabc/MemoryPool.h"
#include "dabc/Command.h"
#include "dabc/Port.h"
#include "dabc/Pointer.h"
#include "dabc/Manager.h"

dabc::AbbReadoutModule::AbbReadoutModule(dabc::Manager* mgr, const char* name,
                                         dabc::Command* cmd)
   : dabc::ModuleAsync(mgr, name), fPool(0), fStandalone(true)
{
         int buffsize = cmd->GetInt(ABB_COMPAR_BUFSIZE, 16384);
         int queuelen = cmd->GetInt(ABB_COMPAR_QLENGTH, 10);
         fStandalone = (bool) cmd->GetInt(ABB_COMPAR_STALONE, 1);
         const char* poolname=cmd->GetStr(ABB_COMPAR_POOL,"ABBReadPool");
         DOUT1(("new AbbReadoutModule %s buff %d queue %d, poolname=%s, standalone=%d", GetName(), buffsize, queuelen,poolname, fStandalone));
         if(fStandalone)
            fPool = CreatePool(poolname, 200, buffsize); // specify pool
         else
            fPool = CreatePool(poolname); // use external pool of bnet readout
   CreateInput("Input", fPool, queuelen);
   CreateRateParameter("DMAReadout", false, 1., "Input","");

   if(!fStandalone) CreateOutput("Output", fPool, queuelen);


}

void dabc::AbbReadoutModule::BeforeModuleStart()
{
    DOUT1(("\n\nAbbReadoutModule::BeforeModuleStart, fStandalone = %d", fStandalone));

}

void dabc::AbbReadoutModule::AfterModuleStop()
{
   DOUT1(("\nAbbReadoutModule finish Rate %5.1f numoper:%7ld \n\n", fRecvRate.GetRate(), fRecvRate.GetNumOper()));
}


void dabc::AbbReadoutModule::ProcessUserEvent(ModuleItem* , uint16_t id )
{
dabc::Buffer* ref = 0;
try
   {
   if(id==evntInput || id==evntOutput)
      {
      dabc::Port* output=Output(0);
      if(output && output->OutputBlocked()) return; // leave immediately if we cannot send further
      Input(0)->Recv(ref);
      if (ref)
         {
            fRecvRate.Packet(ref->GetDataSize());
            if(fStandalone)
               {
                  dabc::Buffer::Release(ref);
               }
            else
               {
                   output->Send(ref);
               }
         }
      }
   else
      {
         DOUT3(("AbbReadoutModule::ProcessUserEvent gets event id:%d, ignored.", id));
      }


   }
catch(dabc::Exception& e)
   {
       DOUT1(("AbbReadoutModule::ProcessUserEvent - raised dabc exception %s at event id=%d", e.what(), id));
       dabc::Buffer::Release(ref);
       // how do we treat this?
   }
catch(std::exception& e)
   {
       DOUT1(("AbbReadoutModule::ProcessUserEvent - raised std exception %s at event id=%d", e.what(), id));
       dabc::Buffer::Release(ref);
   }
catch(...)
   {
       DOUT1(("AbbReadoutModule::ProcessInputEvent - Unexpected exception!!!"));
       throw;
   }

}




