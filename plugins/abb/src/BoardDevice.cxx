#include "pci/BoardDevice.h"
#include "pci/BoardCommands.h"
#include "pci/Transport.h"

#include "dabc/Port.h"
#include "dabc/Manager.h"
#include "dabc/Module.h"
#include "dabc/MemoryPool.h"

#include "mprace/DMABuffer.h"
#include "mprace/Board.h"
#include "mprace/Exception.h"

#include <exception>

unsigned int pci::BoardDevice::fgThreadnum=0;

pci::BoardDevice::BoardDevice(Basic* parent, const char* name, bool usedma) :
   dabc::Device(parent, name),
   fEnableDMA(usedma),
   fBoard(0),
   fReadBAR(0), fReadAddress(0), fReadLength(0),
   fWriteBAR(0),fWriteAddress(0), fWriteLength(0),
   fDMARemapRequired(true),
   fMutex(true)


{
    fDMABuffers.clear();
}

pci::BoardDevice::~BoardDevice()
{
   DoDeviceCleanup(true);
   delete fBoard;
}


bool pci::BoardDevice::DoDeviceCleanup(bool full)
{
   CleanupDMABuffers(); // delete sgmapping before freeing memory in pool
   DOUT3(("_______pci::BoardDevice::DoDeviceCleanup after CleanupDMABuffers"));
   return(dabc::Device::DoDeviceCleanup(full));
}


int pci::BoardDevice::ExecuteCommand(dabc::Command* cmd)
{
   int cmd_res = cmd_true;
   if (cmd->IsName(DABC_PCI_COMMAND_SET_READ_REGION))
      {
          unsigned int bar=cmd->GetInt(DABC_PCI_COMPAR_BAR,1);
          unsigned int address=cmd->GetInt(DABC_PCI_COMPAR_ADDRESS, (0x8000 >> 2));
          unsigned int length=cmd->GetInt(DABC_PCI_COMPAR_SIZE, 1024);
          SetReadBuffer(bar, address, length);
          DOUT1(("Command %s  sets PCI READ region to bar:%d, address:%p, length:%d",DABC_PCI_COMMAND_SET_READ_REGION,bar,address,length));
      }
    else if (cmd->IsName(DABC_PCI_COMMAND_SET_WRITE_REGION))
      {
          unsigned int bar=cmd->GetInt(DABC_PCI_COMPAR_BAR,1);
          unsigned int address=cmd->GetInt(DABC_PCI_COMPAR_ADDRESS, (0x8000 >> 2));
          unsigned int length=cmd->GetInt(DABC_PCI_COMPAR_SIZE, 1024);
          SetWriteBuffer(bar, address, length);
          DOUT1(("Command %s  sets PCI WRITE region to bar:%d, address:%p, length:%d",DABC_PCI_COMMAND_SET_READ_REGION,bar,address,length));
     }
    else
     cmd_res = dabc::Device::ExecuteCommand(cmd);
return cmd_res;

}


int pci::BoardDevice::CreateTransport(dabc::Command* cmd, dabc::Port* port)
{
   dabc::LockGuard lock(fMutex);
   if(port==0)
      port = dabc::mgr()->FindPort(cmd->GetPar(DABC_PCI_COMPAR_PORT));
   if (port==0)
      {
          EOUT(("pci::BoardDevice::CreateTransport FAILED, port %s not found!",cmd->GetPar(DABC_PCI_COMPAR_PORT)));
          return cmd_false;
      }
   pci::Transport* transport = new pci::Transport(this, port);
   DOUT3(("pci::BoardDevice::CreateTransport creates new transport instance %p", transport));
   std::string thrname;
   dabc::formats(thrname,"PCIBoardDeviceThread%d", fgThreadnum++);

   dabc::mgr()->MakeThreadFor(transport, thrname.c_str());
   DOUT1(("pci::BoardDevice::CreateTransport uses thread %s", thrname.c_str()));

   return cmd_true;
}


int pci::BoardDevice::WritePCI(dabc::Buffer* buf)
{
if(fBoard==0) return -3;
if(fEnableDMA)
   {
// set up dma receive
        mprace::DMABuffer* DMAbuf=GetDMABuffer(buf);
        if(DMAbuf==0)
         {
           EOUT(("abb::Device::WritePCI: GetDMABuffer gives ZERO, something is wrong!!!!!!!!"));
           return -3;
         }
        DOUT5(("Start Write DMA id %d mprace %p", buf->GetId(), DMAbuf));
        if(!WriteDMA(fWriteAddress, *DMAbuf, fWriteLength>>2)) return -4;
        // note: readDMA specifies read length in u32 units, thus factor 4!
        DOUT5(("Did WriteDMA"));
   }
else
   {
      fBoard->writeBlock(fWriteAddress, (unsigned int*) (buf->GetDataLocation()), fWriteLength>>2, false);
      // note: readBlock specifies read length in u32 units, thus factor 4!
   } //if(fEnableDMA)

//dabc::Buffer::Release(buf);
return fWriteLength;


}



int pci::BoardDevice::ReadPCI(dabc::Buffer* buf)
{
if(fBoard==0) return -3;
if(fEnableDMA)
   {
// set up dma receive
        mprace::DMABuffer* DMAbuf=GetDMABuffer(buf);
        if(DMAbuf==0)
         {
           EOUT(("abb::Device::ReadPCI: GetDMABuffer gives ZERO, something is wrong!!!!!!!!"));
           return -3;
         }
        DOUT5(("Start ReadDMA id %d mprace %p", buf->GetId(), DMAbuf));
        if(!ReadDMA(fReadAddress, *DMAbuf, fReadLength>>2)) return -4;
        // note: readDMA specifies read length in u32 units, thus factor 4!
        buf->SetDataSize(fReadLength); //pool buffer may be bigger than real dma readlength. set correct value for ratemeter!
        DOUT5(("Did ReadDMA"));
   }
else
   {
      fBoard->readBlock(fReadAddress, (unsigned int*) (buf->GetDataLocation()), fReadLength>>2, false);
      // note: readBlock specifies read length in u32 units, thus factor 4!
   } //if(fEnableDMA)


return fReadLength;
}


bool pci::BoardDevice::ReadPCIStart(dabc::Buffer* buf)
{
// asynchronous read to be implmented in subclass, we do nothing here
return true;

}

int pci::BoardDevice::ReadPCIComplete(dabc::Buffer* buf)
{
// asynchronous read to be implmented in subclass, we just do a blocking read here
return (ReadPCI(buf));
}



bool pci::BoardDevice::ReadDMA(const unsigned int address, mprace::DMABuffer& buf, const unsigned int count)
{
if(fBoard==0) return false;
dabc::LockGuard lock(fMutex); // lock dma buffer against deletion from cleanup!
//buf.syncUsermem(pciDriver::UserMemory::TO_DEVICE); // do we need this?
fBoard->readDMA(address, buf, count, 0, false, true);
//buf.syncUsermem(pciDriver::UserMemory::FROM_DEVICE);

return true;
}

bool pci::BoardDevice::WriteDMA(const unsigned int address, mprace::DMABuffer& buf, const unsigned int count)
{
if(fBoard==0) return false;
dabc::LockGuard lock(fMutex); // lock dma buffer against deletion from cleanup!
//buf.syncUsermem(pciDriver::UserMemory::TO_DEVICE); // do we need this?
fBoard->writeDMA(address, buf, count, 0, true, true);
//buf.syncUsermem(pciDriver::UserMemory::FROM_DEVICE);
return true;
}




mprace::DMABuffer* pci::BoardDevice::GetDMABuffer(dabc::Buffer* buf)
{
if(buf==0) return 0;
dabc::LockGuard lock(fMutex);
try
   {
   bool mapsuccess=true;
   if(fDMARemapRequired)
      {
         DOUT1(("pci::BoardDevice::GetDMABuffer calls MapDMABuffers since remap required!"));
         mapsuccess=MapDMABuffers(buf->Pool());
      }
   if(!mapsuccess) return 0;
   mprace::DMABuffer* dbuf=fDMABuffers[buf->GetId()];
   // check if buffers are matching address:
   if(buf->GetDataLocation() != dbuf->getPointer())
      {
         EOUT(("pci::BoardDevice::GetDMABuffer buffer address MISMATCH for id:%d ! poolbuffer:%p!= dmabuffer%p",buf->GetId(),buf->GetDataLocation(),dbuf->getPointer()));
         return 0;
      }

   return dbuf;
   }
catch(std::exception& e)
   {
      EOUT(("std Exception in GetDMABuffer: %s",e.what()));
      return 0;
   }
catch(...)
   {
      EOUT(("Unexpected Exception in GetDMABuffer"));
      return 0;
   }
}

bool pci::BoardDevice::MapDMABuffers(dabc::MemoryPool* pool)
{
   dabc::LockGuard lock(fMutex);
   CleanupDMABuffers();
   if(pool==0) {
     EOUT(("MapDMABuffers with no memory pool!!!"));
     return false;
     // TODO: error handling?
   }

   dabc::BlockNum_t numblocks=pool->NumMemBlocks();
   std::size_t expectedsize=0;
   dabc::BufferId_t counter=0;
   dabc::BufferSize_t lastlen=0;

DOUT1(("MapDMABuffers pool %p %s numblocks %d numbufs %d", pool, pool->GetName(), numblocks, pool->GetNumBuffersInBlock(0)));

for(dabc::BlockNum_t blocknum=0; blocknum<numblocks; ++blocknum)
   {
      dabc::BufferNum_t numbufs= pool->GetNumBuffersInBlock(blocknum);
      expectedsize+=numbufs;
      fDMABuffers.resize(expectedsize);
      for(dabc::BufferNum_t bufnum=0; bufnum<numbufs; ++bufnum)
      {
         dabc::BufferId_t id=dabc::CodeBufferId( blocknum,  bufnum) ;
         dabc::BufferSize_t len= pool->GetBufferSize(id);
         if(len!=lastlen)
            {
               DOUT1(("******** MapDMABuffers maps buffers of length %u from block %u", len, blocknum));
               lastlen=len;
            }
         mprace::DMABuffer* dbuf=0;
         try
         {
         unsigned int* ptr= (unsigned int*) pool->GetBufferLocation(id);
         DOUT5(("Block %u buf %u ptr %p", blocknum, bufnum, ptr));
         dbuf= new mprace::DMABuffer (*fBoard, len, ptr);
         DOUT5(("Block %u buf %u mpracebuf %p", blocknum, bufnum, dbuf));
         DOUT3(("      mapping user mem dma buffer #%d of size %d", id, dbuf->size()));
         fDMABuffers[id]=dbuf;
         counter++;
         }
       catch (mprace::Exception& e)
         {
            EOUT(("Failed to map DMA buffer for id:%u ! %s", id, e.what()));
            EOUT(("Continue with other buffers in DMA pool."));
            delete dbuf;
         }
       catch(dabc::Exception& e)
         {
            EOUT(("dabc Exception on mapping DMA buffer id:%u ! %s", id, e.what()));
            EOUT(("Continue with other buffers in DMA pool."));
            delete dbuf;
         }
       catch(...)
         {
            EOUT(("Unexpected Exception on mapping DMA buffer id:%u", id));
            EOUT(("Continue with other buffers in DMA pool."));
            delete dbuf;
         }
      }// for bufnum
   } // for blocknum
   DOUT1(("******** MapDMABuffers has mapped %u memory blocks for DMA from pool %s", counter, pool->GetName()));
fDMARemapRequired=false;
return true;
}

void pci::BoardDevice::CleanupDMABuffers()
{
   dabc::LockGuard lock(fMutex);
   std::vector<mprace::DMABuffer*>::const_iterator iter;
   for(iter=fDMABuffers.begin(); iter!=fDMABuffers.end(); ++iter)
      {
         mprace::DMABuffer* buf=*iter;
         DOUT3(("pci::BoardDevice::CleanupDMABuffers is deleting dma buffer %p",buf));
         delete buf;
      }// for
   fDMABuffers.clear();
   fDMARemapRequired=true;
}


