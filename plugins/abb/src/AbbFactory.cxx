#include "abb/AbbFactory.h"
#include "abb/AbbReadoutModule.h"
#include "abb/AbbWriterModule.h"
#include "abb/AbbDevice.h"

#include "dabc/Command.h"
#include "dabc/logging.h"


dabc::AbbFactory dabc::AbbFactory::gAbbFactory;

 dabc::AbbFactory::AbbFactory()
 : dabc::Factory(ABB_NAME_PLUGIN)
{
   DOUT1(("-----------AbbFactory::CONSTRUCTOR!"));

}



dabc::Module* dabc::AbbFactory::CreateModule(const char* classname, const char* modulename, dabc::Command* cmd)
{
     DOUT1(("AbbFactory::CreateModule called for class:%s, module:%s", classname, modulename));

    if ((classname==0) || (cmd==0)) return 0;
    if (strcmp(classname,"AbbReadoutModule")==0)
       {
           dabc::Module* mod= new dabc::AbbReadoutModule(modulename,cmd);
           unsigned int boardnum=cmd->GetInt(ABB_PAR_BOARDNUM, 0);
           DOUT1(("AbbFactory::CreateModule - Created ABBReadout module %s for /dev/fpga%d", modulename, boardnum));
           return mod;
         }
    else if (strcmp(classname,"AbbWriterModule")==0)
       {
           dabc::Module* mod= new dabc::AbbWriterModule(modulename,cmd);
           unsigned int boardnum=cmd->GetInt(ABB_PAR_BOARDNUM, 0);
           DOUT1(("AbbFactory::CreateModule - Created ABBWriter module %s for /dev/fpga%d", modulename, boardnum));
           return mod;
         }
    return 0;
}


dabc::Device* dabc::AbbFactory::CreateDevice(dabc::Basic* parent, const char* classname, const char* devname, dabc::Command* cmd)
{
   if (strcmp(classname,"AbbDevice")!=0) return 0;
   // advanced device parameters contained in factory command object:
   unsigned int boardnum= cmd->GetInt(ABB_PAR_BOARDNUM, 0);
   bool dmamode=true; // put this to parameter later? now always use DMA
   dabc::AbbDevice* dev= new  dabc::AbbDevice(parent, devname, dmamode, boardnum);
   unsigned int bar=cmd->GetInt(ABB_PAR_BAR, 1);
   unsigned int addr=cmd->GetInt(ABB_PAR_ADDRESS, (0x8000 >> 2));
   unsigned int size=cmd->GetInt(ABB_PAR_LENGTH, 8192);
   dev->SetReadBuffer(bar, addr, size);
   dev->SetWriteBuffer(bar, addr, size); // for testing, use same values as for reading; later different parameters in setup!

   DOUT1(("AbbFactory::CreateDevice - Created ABB device %s for /dev/fpga%d", devname, boardnum));
   return dev;
}








