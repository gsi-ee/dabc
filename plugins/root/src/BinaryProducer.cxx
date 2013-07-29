// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "dabc_root/BinaryProducer.h"

#include <math.h>
#include <stdlib.h>


#include "dabc/logging.h"


#include "TMemFile.h"
#include "TStreamerInfo.h"
#include "TBufferFile.h"
#include "TROOT.h"
#include "TH1.h"
#include "TGraph.h"

extern "C" void R__zipMultipleAlgorithm(int cxlevel, int *srcsize, char *src, int *tgtsize, char *tgt, int *irep, int compressionAlgorithm);


dabc_root::BinaryProducer::BinaryProducer(const std::string& name, int compr) :
   dabc::Object(0, name),
   fCompression(compr),
   fMemFile(0),
   fSinfoSize(0)
{
}

dabc_root::BinaryProducer::~BinaryProducer()
{
   if (fMemFile!=0) { delete fMemFile; fMemFile = 0; }
}

void dabc_root::BinaryProducer::CreateMemFile()
{
   if (fMemFile!=0) return;

   TDirectory* olddir = gDirectory; gDirectory = 0;
   TFile* oldfile = gFile; gFile = 0;

   fMemFile = new TMemFile("dummy.file", "RECREATE");
   gROOT->GetListOfFiles()->Remove(fMemFile);

   TH1F* d = new TH1F("d","d", 10, 0, 10);
   fMemFile->WriteObject(d, "h1");
   delete d;

   TGraph* gr = new TGraph(10);
   gr->SetName("abc");
   //      // gr->SetDrawOptions("AC*");
   fMemFile->WriteObject(gr, "gr1");
   delete gr;

   fMemFile->WriteStreamerInfo();

   // make primary list of streamer infos
   TList* l = new TList();

   l->Add(gROOT->GetListOfStreamerInfo()->FindObject("TGraph"));
   l->Add(gROOT->GetListOfStreamerInfo()->FindObject("TH1F"));
   l->Add(gROOT->GetListOfStreamerInfo()->FindObject("TH1"));
   l->Add(gROOT->GetListOfStreamerInfo()->FindObject("TNamed"));
   l->Add(gROOT->GetListOfStreamerInfo()->FindObject("TObject"));

   fMemFile->WriteObject(l, "ll");
   delete l;

   fMemFile->WriteStreamerInfo();

   l = fMemFile->GetStreamerInfoList();
   // l->Print("*");
   fSinfoSize = l->GetSize();
   delete l;

   gDirectory = olddir;
   gFile = oldfile;
}


dabc::Buffer dabc_root::BinaryProducer::CreateBindData(TBufferFile* sbuf)
{
   if (sbuf==0) return dabc::Buffer();

   bool with_zip = true;

   const Int_t kMAXBUF = 0xffffff;
   Int_t noutot = 0;
   Int_t fObjlen = sbuf->Length();
   Int_t nbuffers = 1 + (fObjlen - 1)/kMAXBUF;
   Int_t buflen = TMath::Max(512,fObjlen + 9*nbuffers + 28);
   if (buflen<fObjlen) buflen = fObjlen;

   // TODO: in future one could acquire buffers from memory pool here
   dabc::Buffer buf = dabc::Buffer::CreateBuffer(sizeof(dabc::BinDataHeader)+buflen);

   if (buf.null()) {
      EOUT("Cannot request buffer of specified length");
      delete sbuf;
      return buf;
   }

   dabc::BinDataHeader* hdr = (dabc::BinDataHeader*) buf.SegmentPtr();
   hdr->reset();

   char* fBuffer = ((char*) buf.SegmentPtr()) + sizeof(dabc::BinDataHeader);

   if (with_zip) {
      Int_t cxAlgorithm = 0;

      char *objbuf = sbuf->Buffer();
      char *bufcur = fBuffer; // start writing from beginning

      Int_t nzip   = 0;
      Int_t bufmax = 0;
      Int_t nout = 0;

      for (Int_t i = 0; i < nbuffers; ++i) {
         if (i == nbuffers - 1) bufmax = fObjlen - nzip;
         else               bufmax = kMAXBUF;
         R__zipMultipleAlgorithm(fCompression, &bufmax, objbuf, &bufmax, bufcur, &nout, cxAlgorithm);
         if (nout == 0 || nout >= fObjlen) { //this happens when the buffer cannot be compressed
            DOUT0("Fail to zip buffer");
            noutot = 0;
            with_zip = false;
            break;
         }
         bufcur += nout;
         noutot += nout;
         objbuf += kMAXBUF;
         nzip   += kMAXBUF;
      }
   }

   if (with_zip) {
      hdr->zipped = (uint32_t) fObjlen;
      hdr->payload = (uint32_t) noutot;
   } else {
      memcpy(fBuffer, sbuf->Buffer(), fObjlen);
      hdr->zipped = 0;
      hdr->payload = (uint32_t) fObjlen;
   }

   buf.SetTotalSize(sizeof(dabc::BinDataHeader) + hdr->payload);

   delete sbuf;

   return buf;
}



dabc::Buffer dabc_root::BinaryProducer::GetStreamerInfoBinary()
{
   CreateMemFile();

   TDirectory* olddir = gDirectory; gDirectory = 0;
   TFile* oldfile = gFile; gFile = 0;

   fMemFile->WriteStreamerInfo();
   TList* l = fMemFile->GetStreamerInfoList();
   //l->Print("*");

   fSinfoSize = l->GetSize();

   // TODO: one could reuse memory from dabc::MemoryPool here
   //       now keep as it is and copy data at least once
   TBufferFile* sbuf = new TBufferFile(TBuffer::kWrite, 100000);
   sbuf->SetParent(fMemFile);
   sbuf->MapObject(l);
   l->Streamer(*sbuf);

   delete l;

   gDirectory = olddir;
   gFile = oldfile;

   return CreateBindData(sbuf);

}

dabc::Buffer dabc_root::BinaryProducer::GetBinary(TObject* obj)
{
   CreateMemFile();

   TDirectory* olddir = gDirectory; gDirectory = 0;
   TFile* oldfile = gFile; gFile = 0;

   TList* l1 = fMemFile->GetStreamerInfoList();

   // TODO: one could reuse memory from dabc::MemoryPool here
   //       now keep as it is and copy data at least once
   TBufferFile* sbuf = new TBufferFile(TBuffer::kWrite, 100000);
   sbuf->SetParent(fMemFile);
   sbuf->MapObject(obj);
   obj->Streamer(*sbuf);

   bool believe_not_changed = false;

   if ((fMemFile->GetClassIndex()==0) || (fMemFile->GetClassIndex()->fArray[0] == 0)) {
      DOUT0("SEEMS to be, streamer info was not changed");
      believe_not_changed = true;
   }

   fMemFile->WriteStreamerInfo();
   TList* l2 = fMemFile->GetStreamerInfoList();

   DOUT0("STREAM LIST Before %d After %d", l1->GetSize(), l2->GetSize());

   //DOUT0("=================== BEFORE ========================");
   //l1->Print("*");
   //DOUT0("=================== AFTER ========================");
   //l2->Print("*");

   if (believe_not_changed && (l1->GetSize() != l2->GetSize())) {
      EOUT("StreamerInfo changed when we were expecting no changes!!!!!!!!!");
      exit(444);
   }

   if (believe_not_changed && (l1->GetSize() == l2->GetSize())) {
      DOUT0("++ STREAMER INFO NOT CHANGED AS EXPECTED +++ ");
   }

   fSinfoSize = l2->GetSize();

   delete l1; delete l2;

   gDirectory = olddir;
   gFile = oldfile;

   return CreateBindData(sbuf);
}


