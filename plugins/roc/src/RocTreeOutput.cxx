#include "roc/RocTreeOutput.h"

#include "dabc/logging.h"
#include "mbs/MbsTypeDefs.h"

#include "roc/RocDevice.h"

#include "SysCoreData.h"

#ifdef DABC_ROOT

roc::RocTreeOutput::RocTreeOutput(const char* name, int sizelimit_mb) : 
   dabc::DataOutput(),
   fTree(0),
   fFile(0)
{
   fFile = TFile::Open(name,"recreate");
   if (fFile==0) {
      EOUT(("Cannot open root file %s", name));   
      return; 
   }
   
   if (sizelimit_mb>0) 
      TTree::SetMaxTreeSize(((ULong64_t) sizelimit_mb) * 1024 * 1024);
   
   fTree = new TTree("RocTree","Simple list of hits");
      
   fTree->Branch("rocid", &f_rocid, "rocid/b");
   fTree->Branch("nxyter", &f_nxyter, "nxyter/b");
   fTree->Branch("id", &f_id, "id/b");
   fTree->Branch("timestamp", &f_timestamp, "timestamp/l");
   fTree->Branch("value", &f_value, "value/i");

   for (unsigned n=0; n<8; n++) {
     fLastEpoch[n] = 0;
     fValidEpoch[n] = 0;
   }
}

void roc::RocTreeOutput::WriteNextData(SysCoreData* data)
{
   f_rocid = data->getRocNumber();
   
   if (data->getMessageType() == ROC_MSG_HIT) { // hit
      if (fValidEpoch[f_rocid]) {
         f_nxyter = data->getNxNumber();
         f_id = data->getId();
         f_timestamp = SysCoreData::FullTimeStamp(fLastEpoch[f_rocid], data->getNxTs());
         f_value = data->getAdcValue();
         fTree->Fill();
      }
   } else 
   if (data->getMessageType() == ROC_MSG_EPOCH) { // epoch
      
      fLastEpoch[f_rocid] = data->getEpoch();
      
      fValidEpoch[f_rocid] = true;
   } else 
   if (data->getMessageType() == ROC_MSG_SYNC) { // sync
      if (fValidEpoch[f_rocid]) {
         f_nxyter = 0xff; // mark of sync signal
         f_id = data->getSyncChNum();
         f_timestamp = SysCoreData::FullTimeStamp(fLastEpoch[f_rocid], data->getSyncTs());
         f_value = data->getSyncEvNum();
         fTree->Fill();
      }
   } else 
   if (data->getMessageType() == ROC_MSG_AUX) { // aux
      if (fValidEpoch[f_rocid]) {
         f_nxyter = 0xfe; // mark aux signal
         f_id = data->getAuxChNum();
         f_timestamp = SysCoreData::FullTimeStamp(fLastEpoch[f_rocid], data->getAuxTs());
         f_value = data->getAuxFalling();
         fTree->Fill();
      }
   }
}


bool roc::RocTreeOutput::WriteBuffer(dabc::Buffer* buf)
{
   if (buf->GetTypeId() == roc::rbt_RawRocData)
   {
      dabc::Pointer dataptr(buf);
      
      while (dataptr.fullsize()>=6) {
         
         WriteNextData((SysCoreData*) dataptr.ptr());
         
         dataptr.shift(6);
      }
       
      return true;
   }

   if (buf->GetTypeId() == mbs::mbt_MbsEvs10_1) {
       
      dabc::Pointer evptr(buf); 
       
      while (evptr.fullsize()>sizeof(mbs::EventHeader) + sizeof(mbs::SubeventHeader)) {
          
         mbs::EventHeader* evhdr = (mbs::EventHeader*) evptr();
 
         mbs::SubeventHeader* subevhdr = 0;

         while ((subevhdr = evhdr->NextSubEvent(subevhdr)) != 0) {
            if (subevhdr->iProcId == roc::proc_ErrEvent) {
               EOUT(("CALIBR: Find error in subcrate %u", subevhdr->iSubcrate));
               continue;
            }
            
            dabc::Pointer dataptr(subevhdr->RawData(), subevhdr->RawDataSize());
            
            while (dataptr.fullsize()>=0) {
               WriteNextData((SysCoreData*) dataptr());
               dataptr.shift(6);   
            }
         }
         
         evptr.shift(evhdr->FullSize());
      }
       
      return true;
   }
   
   EOUT(("Buffer type %d not supported", buf->GetTypeId()));
   
   return false;
}

roc::RocTreeOutput::~RocTreeOutput()
{
   fTree = 0; 
   if (fFile) {
      fFile->Write();
      fFile->Close();
      delete fFile;
      fFile = 0;
   }
}

#else

roc::RocTreeOutput::RocTreeOutput(const char* name, int sizelimit_mb) : 
   dabc::DataOutput(),
   fTree(0),
   fFile(0)
{
}

bool roc::RocTreeOutput::WriteBuffer(dabc::Buffer* buf)
{
   return false;
}

roc::RocTreeOutput::~RocTreeOutput()
{
}

void roc::RocTreeOutput::WriteNextData(SysCoreData* data)
{
}


#endif

