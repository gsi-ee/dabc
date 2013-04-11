#include "mbs_root/RootTreeOutput.h"

#include "TTree.h"
#include "TFile.h"
#include "mbs_root/DabcEvent.h"
#include "mbs_root/DabcSubEvent.h"

#include "mbs/Iterator.h"
#include "dabc/Worker.h"

mbs_root::RootTreeOutput::RootTreeOutput() :
   DataOutput(),
   fTree(0),fFile(0),fEvent(0)
{

   // TODO: create event structure here:
   fEvent=new DabcEvent(0,0,10);
   fFileName="mytree.root";
   fSplit=99;
   fTreeBuf=65536;
   fCompression=5;
   fMaxSize=1000000000;

}

mbs_root::RootTreeOutput::~RootTreeOutput()
{
   // TODO: better use Close() function here which is also used in case of input EOF   
   Close();   
   delete fEvent;
}

bool mbs_root::RootTreeOutput::Close()
{
   fTree->Write();
   fFile=fTree->GetCurrentFile(); // for file split after 1.8 Gb!
   delete fFile; // note that we must not delete fxTree, since it is removed from mem when ROOT file is closed!
   fFile=0;
   fTree=0;
   return true;
}

bool mbs_root::RootTreeOutput::Write_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{   
   if (!dabc::DataOutput::Write_Init(wrk, cmd)) return false;

   // TODO: member variables to contain properties for tree output   
   fFileName = wrk.Cfg("RootOutputFile", cmd).AsStdStr(fFileName); // parameter name is xml tag name
   fSplit = wrk.Cfg("RootSplitlevel", cmd).AsInt(fSplit);
   DOUT0("################# Create root file %s , split=%d",fFileName.c_str(),fSplit);
   fTreeBuf = wrk.Cfg("RootTreeBufsize", cmd).AsInt(fTreeBuf);
   fCompression = wrk.Cfg("RootCompression", cmd).AsInt(fCompression);
   fMaxSize = wrk.Cfg("RootMaxFileSize", cmd).AsInt(fMaxSize);
   // fTreeBuf = ...       
   // fMaxSize= ...
   // TODO: open file and create tree here:   
   fFile = new TFile(fFileName.c_str(),"recreate","Mbs tree file",fCompression);
   fTree = new TTree("DabcTree","Dabc Mbs Tree format");   
   fTree->Branch("mbs_root::DabcEvent", &fEvent,fTreeBuf ,fSplit);
   TTree::SetMaxTreeSize(fMaxSize); // after filling this number of bytes, tree will write into next file with name fFileName+"_i"    
   fTree->Write();
   return true;
}


unsigned mbs_root::RootTreeOutput::Write_Buffer(dabc::Buffer& buf)
{
   // example how it could work JAM
   //if (buf==0) return false;

   // some checks if input is of correct format, stolen from lmdoutput class JAM:
   if (buf.GetTypeId() == dabc::mbt_EOF) {
      Close(); // implement Close() function to immediately close outut file here if input comes to end
      return dabc::do_Error;
   }

   if (buf.GetTypeId() != mbs::mbt_MbsEvents) {
      EOUT("Buffer must contain mbs event(s) 10-1, but has type %u", buf.GetTypeId());
      return dabc::do_Error;
   }

   //    if (buf->NumSegments()>1) {
   //       EOUT("Segmented buffer not (yet) supported");
   //       return false;
   //    }

   mbs::ReadIterator iter(buf); // a helper class to iterate over the buffer with mbs data
   iter.Reset(buf);
   while (iter.NextEvent()) // jump to event header in buf
   {
      mbs::EventHeader* evhead= iter.evnt();
      // evaluate event header and put it into mbs_root evntclass:
      Int_t evcount=evhead->iEventNumber;
      Int_t trig=evhead->iTrigger;
      Int_t dummy=evhead->iDummy;


      fEvent->Clear("C");
      fEvent->SetCount(evcount);
      fEvent->SetTrigger(trig);
      fEvent->SetDummy(dummy);
      // probably you have to add properties from mbs event header to DabcEvent class

      while (iter.NextSubEvent()) //
      {

         mbs::SubeventHeader* subev = iter.subevnt();
         // get subevent header values:
         Int_t procid=subev->iProcId;
         Int_t subcrate=subev->iSubcrate;
         Int_t control=subev->iControl;
         Int_t datasize=subev->RawDataSize(); // size of data payload in bytes
         //DOUT0("################# procid:%d subcrate=%d ctrl:%d size=%d \n",procid,subcrate,control,datasize);
         //for(int x=0;x<500;++x){printf("%d \t", *((Int_t*) (subev->RawData()) + x)); }
         mbs_root::DabcSubEvent* mysub=fEvent->AddSubEvent(subcrate, control, procid);

         // TODO: mysub->SetSubCrate(subcrate);
         // etc.
         // probably you have to add properties from mbs event header to DabcSubEvent class

         mysub->AddData((Int_t*) subev->RawData(),datasize/sizeof(Int_t)); // AddData expects size of integers=4bytes, not size in bytes

      }
      // for each event in input buffer, we have to fill the tree:
      fTree->Fill();
   }

   // write tree buffers to file for each input buffer here? or
   //fTree->Write();

   return dabc::do_Ok;
}
