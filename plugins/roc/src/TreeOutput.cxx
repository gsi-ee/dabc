/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#include "roc/TreeOutput.h"

#include "dabc/logging.h"
#include "mbs/MbsTypeDefs.h"

#include "roc/Board.h"

#include "nxyter/Data.h"

#ifdef DABC_ROOT

roc::TreeOutput::TreeOutput(const char* name, int sizelimit_mb) :
   dabc::DataOutput(),
   fFileName(name ? name : ""),
   fSizeLimit(sizelimit_mb),
   fTree(0),
   fFile(0)
{
}

bool roc::TreeOutput::Write_Init(dabc::Command* cmd, dabc::WorkingProcessor* port)
{
   dabc::ConfigSource cfg(cmd, port);

   fFileName = cfg.GetCfgStr(mbs::xmlFileName, fFileName);
   fSizeLimit = cfg.GetCfgInt(mbs::xmlSizeLimit, fSizeLimit);

   fFile = TFile::Open(fFileName.c_str(),"recreate");
   if (fFile==0) {
      EOUT(("Cannot open root file %s", fFileName.c_str()));
      return false;
   }

   if (fSizeLimit>0)
      TTree::SetMaxTreeSize(((ULong64_t) fSizeLimit) * 1024 * 1024);

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

   return true;
}



void roc::TreeOutput::WriteNextData(nxyter::Data* data)
{
   f_rocid = data->rocNumber();

   if (data->getMessageType() == ROC_MSG_HIT) { // hit
      if (fValidEpoch[f_rocid]) {
         f_nxyter = data->getNxNumber();
         f_id = data->getId();
         f_timestamp = nxyter::Data::FullTimeStamp(fLastEpoch[f_rocid], data->getNxTs());
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
         f_timestamp = nxyter::Data::FullTimeStamp(fLastEpoch[f_rocid], data->getSyncTs());
         f_value = data->getSyncEvNum();
         fTree->Fill();
      }
   } else
   if (data->getMessageType() == ROC_MSG_AUX) { // aux
      if (fValidEpoch[f_rocid]) {
         f_nxyter = 0xfe; // mark aux signal
         f_id = data->getAuxChNum();
         f_timestamp = nxyter::Data::FullTimeStamp(fLastEpoch[f_rocid], data->getAuxTs());
         f_value = data->getAuxFalling();
         fTree->Fill();
      }
   }
}


bool roc::TreeOutput::WriteBuffer(dabc::Buffer* buf)
{
   if (buf->GetTypeId() == roc::rbt_RawRocData)
   {
      dabc::Pointer dataptr(buf);

      while (dataptr.fullsize()>=6) {

         WriteNextData((nxyter::Data*) dataptr.ptr());

         dataptr.shift(6);
      }

      return true;
   }

   if (buf->GetTypeId() == mbs::mbt_MbsEvents) {

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
               WriteNextData((nxyter::Data*) dataptr());
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

roc::TreeOutput::~TreeOutput()
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

roc::TreeOutput::TreeOutput(const char* name, int sizelimit_mb) :
   dabc::DataOutput(),
   fFileName(name ? name : ""),
   fSizeLimit(sizelimit_mb),
   fTree(0),
   fFile(0)
{
}

bool roc::TreeOutput::Write_Init(dabc::Command* cmd, dabc::WorkingProcessor* port)
{
   return false;
}


bool roc::TreeOutput::WriteBuffer(dabc::Buffer* buf)
{
   return false;
}

roc::TreeOutput::~TreeOutput()
{
}

void roc::TreeOutput::WriteNextData(nxyter::Data*)
{
}


#endif

