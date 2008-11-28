#include "abb/Device.h"

#include "dabc/MemoryPool.h"
#include "dabc/Buffer.h"
#include "dabc/Pointer.h"
#include "dabc/PoolHandle.h"

#include "mprace/ABB.h"
#include "mprace/DMAEngineWG.h"
#include "abb/Factory.h"

#define  USE_INTERRUPTS 1


abb::Device::Device(Basic* parent, const char* name, bool enabledma, dabc::Command* cmd) :
   pci::BoardDevice(parent, name, enabledma),
   fDMAengine(0),
   fReadDMAbuf(0),
   fEventCnt(0),
   fUniqueId(0)

{
   unsigned int devicenum = GetCfgInt(ABB_PAR_BOARDNUM, 0, cmd);
   unsigned int bar = GetCfgInt(ABB_PAR_BAR, 1, cmd);
   unsigned int addr = GetCfgInt(ABB_PAR_ADDRESS, (0x8000 >> 2), cmd);
   unsigned int size = GetCfgInt(ABB_PAR_LENGTH, 8192, cmd);

   DOUT1(("Create ABB device %s for /dev/fpga%u", name, devicenum));

   fBoard = new mprace::ABB(devicenum);
   fDMAengine = & (static_cast<mprace::DMAEngineWG&>(fBoard->getDMAEngine()) );

   #ifdef USE_INTERRUPTS
   	fDMAengine->setUseInterrupts(true);
   #endif

   fUniqueId=88; // get from ABB class later?

   SetReadBuffer(bar, addr, size);
   SetWriteBuffer(bar, addr, size); // for testing, use same values as for reading; later different parameters in setup!
}

abb::Device::~Device()
{
}

bool abb::Device::DoDeviceCleanup(bool full)
{
   DOUT3(("_______abb::Device::DoDeviceCleanup... "));
   bool res = pci::BoardDevice::DoDeviceCleanup(full);
   if(res) SetEventCount(0);

   DOUT3(("_______abb::Device::DoDeviceCleanup done res=%s", DBOOL(res)));

   return res;
}


bool  abb::Device::WriteBnetHeader(dabc::Buffer* buf)
{
   //if(buf==0) return false;
   // write header for bnet test into buffer; later to be done in ABB
   fEventCnt++;
   dabc::Pointer ptr(buf);
   ptr.copyfrom(&fEventCnt, sizeof(fEventCnt));
   ptr+=sizeof(fEventCnt);
   ptr.copyfrom(&fUniqueId, sizeof(fUniqueId));
   ptr+=sizeof(fUniqueId);
   return true;


}

int abb::Device::ReadPCI(dabc::Buffer* buf)
{
   int result=pci::BoardDevice::ReadPCI(buf);
   if(result>0) WriteBnetHeader(buf);
   return result;
}


bool abb::Device::ReadPCIStart(dabc::Buffer* buf)
{
if(fBoard==0) return false;
if(fEnableDMA)
   {
// set up dma receive
         LockDMABuffers(); // do we need to protect buffer list here?
         if(fReadDMAbuf!=0)
         {
           EOUT(("abb::Device::ReadPCIStart: previous read buffer not yet processed, something is wrong!!!!!!!!"));
           return false;
         }
        fReadDMAbuf=GetDMABuffer(buf);
        if(fReadDMAbuf==0)
         {
           EOUT(("abb::Device::ReadPCIStart: GetDMABuffer gives ZERO, something is wrong!!!!!!!!"));
           return false;
         }
        DOUT5(("Start ReadDMA id %d mprace %p, non blocking.", buf->GetId(), fReadDMAbuf));
        buf->SetDataSize(fReadLength); //pool buffer may be bigger than real dma readlength. set correct value for ratemeter!
        // note: each sync will cost 4us delay!
        //fReadDMAbuf->syncUsermem(pciDriver::UserMemory::TO_DEVICE); // do we need this?
        fBoard->readDMA(fReadAddress, *fReadDMAbuf, fReadLength>>2, 0, false, false); // start reading with no blocking wait
        // note: readDMA specifies read length in u32 units, thus factor 4!
        DOUT5(("Started readDMA for buffer %p", fReadDMAbuf));
   }
else
   {
      // in PIO mode, the reading will be done in one blocking operation
      // at ReadPCIComplete
   } //if(fEnableDMA)
return true;

}

int abb::Device::ReadPCIComplete(dabc::Buffer* buf)
{
if(fBoard==0) return -3;
if(fEnableDMA)
   {
       if(fReadDMAbuf==0)
         {
           EOUT(("abb::Device::ReadPCIComplete: no active DMA buffer, something is wrong!!!!!!!!"));
           return -3;
         }

        // wait here until read is done...
        fDMAengine->waitReadComplete();
        //fReadDMAbuf->syncUsermem(pciDriver::UserMemory::FROM_DEVICE);
         // note: each sync will cost 4us delay!
        DOUT5(("Completed readDMA for buffer %p", fReadDMAbuf));
        fReadDMAbuf=0;
        UnlockDMABuffers(); // do we need to protect buffer list here?
   }
else
   {
       //if(buf==0) return -3;
       fBoard->readBlock(fReadAddress, (unsigned int*) (buf->GetDataLocation()), fReadLength>>2, false);
   } //if(fEnableDMA)
WriteBnetHeader(buf);
return fReadLength;
}

