#include "pci/PCITransport.h"
#include "pci/PCIBoardDevice.h"

#include "dabc/Port.h"
#include "dabc/logging.h"

//#include <iostream>


dabc::PCITransport::PCITransport(PCIBoardDevice* dev, Port* port) :
   DataTransport(dev, port, true, true), fPCIDevice(dev)
   // provide input and output buffers
{
   port->AssignTransport(this);
}

//dabc::TimeStamp_t times1[10000];
//dabc::TimeStamp_t times2[10000];
//int cnt=0;


dabc::PCITransport::~PCITransport()
{
//   for (int n=0; n<cnt-1; n++)
//      DOUT0(("cnt %05d  start:%7.1f  duration:%7.1f tonext:%7.1f", n, times1[n], times2[n] - times1[n], times1[n+1] - times1[n]));
}

unsigned dabc::PCITransport::Read_Size()
{
   int res = fPCIDevice->GetReadLength();
   return res>0 ? res : DataInput::di_Error;
}

unsigned dabc::PCITransport::Read_Start(Buffer* buf)
{
   return fPCIDevice->ReadPCIStart(buf) ? dabc::DataInput::di_Ok : dabc::DataInput::di_Error;
//return true;
}


unsigned dabc::PCITransport::Read_Complete(Buffer* buf)
{
   return fPCIDevice->ReadPCIComplete(buf) > 0 ? dabc::DataInput::di_Ok : dabc::DataInput::di_Error;
//return fPCIDevice->ReadPCI(buf);
}



bool dabc::PCITransport::WriteBuffer(Buffer* buf)
{
   //if (cnt<1000) times1[cnt] = TimeStamp();

   bool res = fPCIDevice->WritePCI(buf);

   //if (cnt<1000) times2[cnt++] = TimeStamp();

   return res;

}


void dabc::PCITransport::ProcessPoolChanged(MemoryPool* pool)
{
DOUT1(("############## PCITransport::ProcessPoolChanged for memory pool %x",pool));
fPCIDevice->MapDMABuffers(pool);

}



