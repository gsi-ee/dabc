#include "pci/Transport.h"
#include "pci/BoardDevice.h"

#include "dabc/Port.h"
#include "dabc/logging.h"


pci::Transport::Transport(pci::BoardDevice* dev, dabc::Port* port) :
   dabc::DataTransport(dev, port, true, true),
   fPCIDevice(dev)
   // provide input and output buffers
{
   port->AssignTransport(this);
}

//dabc::TimeStamp_t times1[10000];
//dabc::TimeStamp_t times2[10000];
//int cnt=0;


pci::Transport::~Transport()
{
//   for (int n=0; n<cnt-1; n++)
//      DOUT0(("cnt %05d  start:%7.1f  duration:%7.1f tonext:%7.1f", n, times1[n], times2[n] - times1[n], times1[n+1] - times1[n]));
}

unsigned pci::Transport::Read_Size()
{
   int res = fPCIDevice->GetReadLength();
   return res>0 ? res : dabc::di_Error;
}

unsigned pci::Transport::Read_Start(dabc::Buffer* buf)
{
   return fPCIDevice->ReadPCIStart(buf) ? dabc::di_Ok : dabc::di_Error;
//return true;
}


unsigned pci::Transport::Read_Complete(dabc::Buffer* buf)
{
   return fPCIDevice->ReadPCIComplete(buf) > 0 ? dabc::di_Ok : dabc::di_Error;
//return fPCIDevice->ReadPCI(buf);
}



bool pci::Transport::WriteBuffer(dabc::Buffer* buf)
{
   //if (cnt<1000) times1[cnt] = TimeStamp();

   bool res = fPCIDevice->WritePCI(buf);

   //if (cnt<1000) times2[cnt++] = TimeStamp();

   return res;

}


void pci::Transport::ProcessPoolChanged(dabc::MemoryPool* pool)
{
DOUT1(("############## pci::Transport::ProcessPoolChanged for memory pool %x",pool));
fPCIDevice->MapDMABuffers(pool);

}



