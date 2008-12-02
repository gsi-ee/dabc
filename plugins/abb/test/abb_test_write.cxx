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
               std::cout <<"Warning: specified write size "<<readsize<<" byte exceeds ABB block ram size "<<MAXPRAMSIZE <<", limiting DMA." << std::endl;
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

   cmd = new dabc::CmdCreateModule("abb::WriterModule","ABB_Sender", "WriterThread");
   cmd->SetInt(ABB_COMPAR_BUFSIZE, readsize);
   cmd->SetInt(ABB_COMPAR_STALONE,1);
   cmd->SetInt(ABB_COMPAR_QLENGTH, 10);
   cmd->SetStr(ABB_COMPAR_POOL,"ABB-standalone-pool");
   cmd->SetStr(ABB_PAR_DEVICE,devname.c_str());

   res = manager.Execute(cmd);
   // test: use same thread for readout module as for device:
   //res=manager.CreateModule("abb::WriterModule","ABB_Sender","PCIBoardDeviceThread0",cmd);

   DOUT1(("Create ABB writer module = %s", DBOOL(res)));

   res= manager.CreateMemoryPools();
   DOUT1(("Create memory pools result=%s", DBOOL(res)));

   //// connect to ABB device:
   res=manager.CreateTransport("ABB_Sender/Output", devname.c_str());
   DOUT1(("Connected module to ABB device = %s", DBOOL(res)));

   //// TEST: connect with null transport
//    cmd = new dabc::Command("NullConnect");
//    cmd->SetStr("Port", "ABB_Sender/Output");
//    res=manager.Execute(cmd, 5);
//    DOUT1(("Connected module to NULL transport, result= %s", DBOOL(res)));
   /////////// end TEST
//   DOUT1(("Sleeping 3 s..."));
//   sleep(3); // wait until mappping is complete
   manager.StartModule("ABB_Sender");
   DOUT1(("Started writer module...."));
   sleep(5);
   manager.StopModule("ABB_Sender");
   DOUT1(("Stopped writer module."));
      manager.CleanupManager();
   DOUT1(("Finish"));
   return 0;
}
