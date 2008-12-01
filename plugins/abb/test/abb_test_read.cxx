#include "dabc/Command.h"
#include "dabc/StandaloneManager.h"
#include "dabc/logging.h"
#include "abb/Factory.h"
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
   dabc::Command* dcom= new dabc::CmdCreateDevice("abb::Device", devname.c_str());
      // set additional parameters for abb device here:
   dcom->SetInt(ABB_PAR_BOARDNUM, BOARD_NUM);
   dcom->SetInt(ABB_PAR_BAR, 1);
   dcom->SetInt(ABB_PAR_ADDRESS, READADDRESS);
   dcom->SetInt(ABB_PAR_LENGTH, readsize);
   res = manager.Execute(dcom);
   DOUT1(("CreateDevice = %s", DBOOL(res)));

   cmd = new dabc::CommandCreateModule("abb::ReadoutModule","ABB_Readout", "ReadoutThread");
   cmd->SetInt(ABB_COMPAR_BUFSIZE, readsize);
   cmd->SetInt(ABB_COMPAR_STALONE,1);
   cmd->SetInt(ABB_COMPAR_QLENGTH, 10);
   cmd->SetStr(ABB_COMPAR_POOL,"ABB-standalone-pool");
   cmd->SetStr(ABB_PAR_DEVICE,devname.c_str());

   res=manager.Execute(cmd);
   // test: use same thread for readout module as for device:
   //res=manager.CreateModule("abb::ReadoutModule","ABB_Readout","PCIBoardDeviceThread0",cmd);

   DOUT1(("Create ABB readout module = %s", DBOOL(res)));

   res= manager.CreateMemoryPools();
   DOUT1(("Create memory pools result=%s", DBOOL(res)));

   res=manager.CreateTransport("ABB_Readout/Input", devname.c_str());
   DOUT1(("Connected module to ABB device = %s", DBOOL(res)));
   manager.StartModule("ABB_Readout");
   DOUT1(("Started readout module...."));
   sleep(5);
   manager.StopModule("ABB_Readout");
   DOUT1(("Stopped readout module."));
      manager.CleanupManager();
   DOUT1(("Finish"));
   return 0;
}
