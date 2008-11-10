#include "dabc/Command.h"
#include "dabc/StandaloneManager.h"
#include "dabc/logging.h"
#include "dabc/AbbFactory.h"
#include <iostream>


#define BOARD_NUM 0
// device number X of /dev/fpgaX

#define MAXPRAMSIZE 1024*1024
// 1Mb on bar 1

#define READADDRESS  (0x8000 >> 2)
#define READSIZE 16*1024
   // size of block to read from board





int main(int numc, char* args[])
{
   dabc::CommandClient cli;
   dabc::Command* cmd;
   bool res=false;
   int nodeid=0; // this node id
   int numnodes=1; // number of nodes in cluster
   unsigned int readsize=READSIZE;
    if(numc < 2) {
      std::cout <<"Using default buffer size "<<readsize<<" Byte ." << std::endl;
   }
   else
      {
         readsize=atoi(args[1]);
         if(readsize>MAXPRAMSIZE)
            {
               std::cout <<"Warning: specified readsize "<<readsize<<" byte exceeds ABB block ram size "<<MAXPRAMSIZE <<", limiting DMA." << std::endl;
               readsize=MAXPRAMSIZE;
            }
         std::cout <<"Using buffer size "<<readsize<<" Byte ." << std::endl;
      }
   dabc::SetDebugLevel(1);

   dabc::StandaloneManager manager(0, nodeid, numnodes);

   std::string devname="ABB";
   std::string fulldevname="Devices/"+devname;
   dabc::Command* dcom= new dabc::CmdCreateDevice("AbbDevice", devname.c_str());
      // set additional parameters for abb device here:
   dcom->SetInt(ABB_PAR_BOARDNUM, BOARD_NUM);
   dcom->SetInt(ABB_PAR_BAR, 1);
   dcom->SetInt(ABB_PAR_ADDRESS, READADDRESS);
   dcom->SetInt(ABB_PAR_LENGTH, readsize);
   res=manager.CreateDevice("AbbDevice",devname.c_str(),dcom);
   DOUT1(("CreateDevice = %s", DBOOL(res)));

   cmd = new dabc::CommandCreateModule("AbbReadoutModule","ABB_Readout");
   cmd->SetInt(ABB_COMPAR_BUFSIZE, readsize);
   cmd->SetInt(ABB_COMPAR_STALONE,1);
   cmd->SetInt(ABB_COMPAR_QLENGTH, 10);
   cmd->SetStr(ABB_COMPAR_POOL,"ABB-standalone-pool");
   cmd->SetStr(ABB_PAR_DEVICE,devname.c_str());

   res=manager.CreateModule("AbbReadoutModule","ABB_Readout","ReadoutThread",cmd);
   // test: use same thread for readout module as for device:
   //res=manager.CreateModule("AbbReadoutModule","ABB_Readout","PCIBoardDeviceThread0",cmd);

   DOUT1(("Create ABB readout module = %s", DBOOL(res)));

   cmd = new dabc::CommandCreateModule("AbbWriterModule","ABB_Sender");
   cmd->SetInt(ABB_COMPAR_BUFSIZE, readsize);
   cmd->SetInt(ABB_COMPAR_STALONE,1);
   cmd->SetInt(ABB_COMPAR_QLENGTH, 10);
   cmd->SetStr(ABB_COMPAR_POOL,"ABB-standalone-pool");
   cmd->SetStr(ABB_PAR_DEVICE,devname.c_str());
   res=manager.CreateModule("AbbWriterModule","ABB_Sender","WriterThread",cmd);
   // test: use same thread for readout module as for device:
   //res=manager.CreateModule("AbbWriterModule","ABB_Sender","PCIBoardDeviceThread0",cmd);

   DOUT1(("Create ABB writer module = %s", DBOOL(res)));


   res= manager.CreateMemoryPools();
   DOUT1(("Create memory pools result=%s", DBOOL(res)));

   //// connect to ABB device:
   res=manager.CreateTransport(fulldevname.c_str(),"ABB_Readout/Ports/Input");
   DOUT1(("Connected readout module to ABB device = %s", DBOOL(res)));

   res=manager.CreateTransport(fulldevname.c_str(),"ABB_Sender/Ports/Output");
   DOUT1(("Connected writer module to ABB device = %s", DBOOL(res)));


   manager.StartModule("ABB_Readout");
   manager.StartModule("ABB_Sender");
   DOUT1(("Started modules...."));
   sleep(60);
   manager.StopModule("ABB_Sender");
   manager.StopModule("ABB_Readout");
   DOUT1(("Stopped modules."));
      manager.CleanupManager();
   DOUT1(("Finish"));
   return 0;
}
