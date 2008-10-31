#include "abb/AbbDevice.h"

#include "dabc/MemoryPool.h"
#include "dabc/Buffer.h"
#include "dabc/Pointer.h"
#include "dabc/PoolHandle.h"

#include "mprace/ABB.h"
#include "mprace/DMAEngineWG.h"

#define  USE_INTERRUPTS 1


dabc::AbbDevice::AbbDevice(Basic* parent, const char* name, bool enabledma, unsigned int devicenum) :
   dabc::PCIBoardDevice(parent, name, enabledma),fDMAengine(0),fReadDMAbuf(0),
   fEventCnt(0),fUniqueId(0)

{

fBoard=new mprace::ABB(devicenum);
fDMAengine = & (static_cast<mprace::DMAEngineWG&>(fBoard->getDMAEngine()) );

#ifdef USE_INTERRUPTS
	fDMAengine->setUseInterrupts(true);
#endif

fUniqueId=88; // get from ABB class later?

}

dabc::AbbDevice::~AbbDevice()
{
}

bool dabc::AbbDevice::DoDeviceCleanup(bool full)
{
   DOUT3(("_______AbbDevice::DoDeviceCleanup... "));
   bool res = dabc::PCIBoardDevice::DoDeviceCleanup(full);
   if(res) SetEventCount(0);

   DOUT3(("_______AbbDevice::DoDeviceCleanup done res=%s", DBOOL(res)));

   return res;
}


bool  dabc::AbbDevice::WriteBnetHeader(dabc::Buffer* buf)
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

int dabc::AbbDevice::ReadPCI(dabc::Buffer* buf)
{
   int result=dabc::PCIBoardDevice::ReadPCI(buf);
   if(result>0) WriteBnetHeader(buf);
   return result;
}


bool dabc::AbbDevice::ReadPCIStart(dabc::Buffer* buf)
{
if(fBoard==0) return false;
if(fEnableDMA)
   {
// set up dma receive
         LockDMABuffers(); // do we need to protect buffer list here?
         if(fReadDMAbuf!=0)
         {
           EOUT(("AbbDevice::ReadPCIStart: previous read buffer not yet processed, something is wrong!!!!!!!!"));
           return false;
         }
        fReadDMAbuf=GetDMABuffer(buf);
        if(fReadDMAbuf==0)
         {
           EOUT(("AbbDevice::ReadPCIStart: GetDMABuffer gives ZERO, something is wrong!!!!!!!!"));
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

int dabc::AbbDevice::ReadPCIComplete(dabc::Buffer* buf)
{
if(fBoard==0) return -3;
if(fEnableDMA)
   {
       if(fReadDMAbuf==0)
         {
           EOUT(("AbbDevice::ReadPCIComplete: no active DMA buffer, something is wrong!!!!!!!!"));
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

