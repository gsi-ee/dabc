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

#include "dabc/BinaryFile.h"

#include "stream/RunModule.h"

#include "hadaq/Iterator.h"
#include "mbs/Iterator.h"
#include "hadaq/TdcProcessor.h"

#include "dabc/Manager.h"
#include "dabc/Factory.h"
#include "dabc/Iterator.h"
#include "dabc/Buffer.h"
#include "dabc/Publisher.h"

#include <stdlib.h>

#include "base/Buffer.h"
#include "base/StreamProc.h"
#include "stream/DabcProcMgr.h"

// ==================================================================================

stream::RunModule::RunModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fParallel(0),
   fInitFunc(0),
   fStopMode(0),
   fProcMgr(0),
   fAsf(),
   fDidMerge(false),
   fTotalSize(0),
   fTotalEvnts(0),
   fTotalOutEvnts(0)
{
   fParallel = Cfg("parallel",cmd).AsInt(0);

   // we need one input and no outputs
   EnsurePorts(1, fParallel<0 ? 0 : fParallel);

   fInitFunc = cmd.GetPtr("initfunc");

   if ((fParallel>=0) && (fInitFunc==0)) {
      // first generate and load init func

      if (fParallel>999) fParallel=999;

      // ensure that all histos on all branches present
      hadaq::TdcProcessor::SetAllHistos(true);

      const char* dabcsys = getenv("DABCSYS");
      const char* streamsys = getenv("STREAMSYS");
      if (dabcsys==0) {
         EOUT("DABCSYS variable not set, cannot run stream framework");
         dabc::mgr.StopApplication();
         return;
      }

      if (streamsys==0) {
         EOUT("STREAMSYS variable not set, cannot run stream framework");
         dabc::mgr.StopApplication();
         return;
      }

      bool second = system("ls second.C >/dev/null 2>/dev/null") == 0;

      std::string exec = dabc::format("g++ %s/plugins/stream/src/stream_engine.cpp -O2 -fPIC -Wall -I. -I%s/include %s"
            "-shared -Wl,-soname,librunstream.so -Wl,--no-as-needed -Wl,-rpath,%s/lib -Wl,-rpath,%s/lib  -o librunstream.so",
            dabcsys, streamsys, (second ? "-D_SECOND_ " : ""), dabcsys, streamsys);

      system("rm -f ./librunstream.so");

      DOUT0("Executing %s", exec.c_str());

      int res = system(exec.c_str());

      if (res!=0) {
         EOUT("Fail to compile first.C/second.C scripts. Abort");
         dabc::mgr.StopApplication();
         return;
      }

      if (!dabc::Factory::LoadLibrary("librunstream.so")) {
         EOUT("Fail to load generated librunstream.so library");
         dabc::mgr.StopApplication();
         return;
      }

      fInitFunc = dabc::Factory::FindSymbol("stream_engine");
      if (fInitFunc==0) {
         EOUT("Fail to find stream_engine function in librunstream.so library");
         dabc::mgr.StopApplication();
         return;
      }
   }


   if (fParallel>=0) {
      fAsf = Cfg("asf",cmd).AsStr();
      // do not autosave is specified, module will not stop when data source disappears
      if ((fAsf.length()==0) || (fParallel>0)) SetAutoStop(false);
      CreatePar("Events").SetRatemeter(false, 3.).SetUnits("Ev");
   } else {
      SetAutoStop(false);
   }

   fWorkerHierarchy.Create("Worker");

   Publish(fWorkerHierarchy, dabc::format("$CONTEXT$/%s", GetName()));
}

stream::RunModule::~RunModule()
{
   if (fProcMgr) {
      delete fProcMgr;
      fProcMgr = 0;
   }
}

typedef void* entryfunc();


void stream::RunModule::OnThreadAssigned()
{
   if ((fInitFunc!=0) && (fParallel<=0)) {

      entryfunc* func = (entryfunc*) fInitFunc;

      fProcMgr = new DabcProcMgr;
      fProcMgr->SetTop(fWorkerHierarchy);

      func();

      // remove pointer, let other modules to create and use it
      base::ProcMgr::ClearInstancePointer();

      if (fProcMgr->IsStreamAnalysis()) {
         EOUT("Stream analysis kind is not supported in DABC engine");
         dabc::mgr.StopApplication();
      }
   } else
   if ((fParallel>0) && (fInitFunc!=0))  {
      for (int n=0;n<fParallel;n++) {
         std::string mname = dabc::format("%s%03d", GetName(), n);
         dabc::CmdCreateModule cmd("stream::RunModule", mname);
         cmd.SetPtr("initfunc", fInitFunc);
         cmd.SetInt("parallel", -1);

         DOUT0("Create module %s", mname.c_str());

         dabc::mgr.Execute(cmd);

         dabc::ModuleRef m = dabc::mgr.FindModule(mname);

         DOUT0("Connect %s ->%s", OutputName(n,true).c_str(), m.InputName(0).c_str());
         dabc::Reference r = dabc::mgr.Connect(OutputName(n,true), m.InputName(0));
         DOUT0("Connect output %u connected %s", n, DBOOL(IsOutputConnected(n)));

         m.Start();
      }
   }
}

void stream::RunModule::ProduceMergedHierarchy()
{
   if (fDidMerge || (fParallel<=0)) return;

   fDidMerge = true;

   dabc::PublisherRef publ = GetPublisher();

   dabc::Hierarchy main;
   int nhist = 0;

   DOUT0("Can now merge histograms");
   for (int n=0;n<fParallel;n++) {

      std::string mname = dabc::format("%s%03d", GetName(), n);

      dabc::ModuleRef m = dabc::mgr.FindModule(mname);

      m.Stop();

      dabc::Command cmd("GetHierarchy");
      m.Execute(cmd);

      dabc::Hierarchy h = cmd.GetRef("hierarchy");

      if (main.null()) { main = h; continue; }

      dabc::Iterator iter1(main), iter2(h);
      bool miss = false;
      while (iter1.next()) {
         if (!iter2.next()) { miss = true; break; }
         if (strcmp(iter1.name(),iter2.name())!=0) { miss = true; break; }

         // merge histograms till the end
         dabc::Hierarchy item1 = iter1.ref();
         dabc::Hierarchy item2 = iter2.ref();

         if (item1.HasField("_dabc_hist") && item2.HasField("_dabc_hist") &&
               (item1.GetFieldPtr("bins")!=0) && (item2.GetFieldPtr("bins")!=0) &&
               (item1.GetFieldPtr("bins")->GetArraySize() == item2.GetFieldPtr("bins")->GetArraySize())) {
            double* arr1 = item1.GetFieldPtr("bins")->GetDoubleArr();
            double* arr2 = item2.GetFieldPtr("bins")->GetDoubleArr();
            int indx = item1.GetField("_kind").AsStr()=="ROOT.TH1D" ? 2 : 5;
            int len = item1.GetField("bins").GetArraySize();
            if (n==1) nhist++;

            if (arr1 && arr2)
              while (++indx<len) arr1[indx]+=arr2[indx];
         }
      }

      if (miss) {
         EOUT("!!!!!!!!!!!!!! MISMATCH - CANNOT MERGE HISTOGRAMS !!!!!!!!!!!!");
         dabc::mgr.StopApplication();
      } else {
         DOUT0("Merged %d histograms", nhist);
      }


   }

   if (fAsf.length()>0) SaveHierarchy(main.SaveToBuffer());
}


int stream::RunModule::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("SlaveFinished")) {
      if (--fStopMode == 0) {
         ProduceMergedHierarchy();
         DOUT0("Stop ourself");
         Stop();
      }

      return dabc::cmd_true;
   } else
   if (cmd.IsName("GetHierarchy")) {
      cmd.SetRef("hierarchy", fWorkerHierarchy);
      return dabc::cmd_true;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

void stream::RunModule::BeforeModuleStart()
{
   DOUT0("START STREAM MODULE %s inp %s", GetName(), DBOOL(IsInputConnected(0)));

   if (fProcMgr) fProcMgr->UserPreLoop();
}

void stream::RunModule::SaveHierarchy(dabc::Buffer buf)
{
   if (buf.GetTotalSize()==0) return;

   DOUT0("store hierarchy size %d in temporary h.bin file", buf.GetTotalSize(), buf.NumSegments());
   {
      dabc::BinaryFile f;
      system("rm -f h.bin");
      if (f.OpenWriting("h.bin")) {
         if (f.WriteBufHeader(buf.GetTotalSize(), buf.GetTypeId()))
            for (unsigned n=0;n<buf.NumSegments();n++)
               f.WriteBufPayload(buf.SegmentPtr(n), buf.SegmentSize(n));
         f.Close();
      }
   }

   std::string args("dabc_root -skip-zero -h h.bin -o ");
   args += fAsf;

   DOUT0("Calling: %s", args.c_str());

   int res = system(args.c_str());

   if (res!=0) EOUT("Fail to convert DABC histograms in ROOT file, check h.bin file");
          else system("rm -f h.bin");
}

void stream::RunModule::AfterModuleStop()
{
   if (fProcMgr) fProcMgr->UserPostLoop();

   DOUT0("STOP STREAM MODULE %s len %lu evnts %lu out %lu", GetName(), fTotalSize, fTotalEvnts, fTotalOutEvnts);

   if (fParallel>0) {
      ProduceMergedHierarchy();
   } else
   if (fAsf.length()>0) SaveHierarchy(fWorkerHierarchy.SaveToBuffer());

   DestroyPar("Events");
}

bool stream::RunModule::ProcessNextEvent(void* evnt, unsigned evntsize)
{
   if (fProcMgr==0) return false;

   fTotalEvnts++;

   if (fParallel==0) Par("Events").SetValue(1);

   // TODO - later we need to use DABC buffer here to allow more complex
   // analysis when many dabc buffers required at the same time to analyze data

   base::Buffer bbuf;

   bbuf.makereferenceof(evnt, evntsize);

   bbuf().kind = base::proc_TRBEvent;
   bbuf().boardid = 0;
   bbuf().format = 0;

   fProcMgr->ProvideRawData(bbuf);

   // scan new data
   fProcMgr->ScanNewData();

   if (fProcMgr->IsRawAnalysis()) {

      fProcMgr->SkipAllData();

   } else {

      //TGo4Log::Info("Analyze data");

      // analyze new sync markers
      if (fProcMgr->AnalyzeSyncMarkers()) {

         // get and redistribute new triggers
         fProcMgr->CollectNewTriggers();

         // scan for new triggers
         fProcMgr->ScanDataForNewTriggers();
      }
   }

   base::Event* outevent = 0;

   // now producing events as much as possible
   while (fProcMgr->ProduceNextEvent(outevent)) {

      fTotalOutEvnts++;

      bool store = fProcMgr->ProcessEvent(outevent);

      if (store) {
         // implement events store later
      }
   }

   delete outevent;

   return true;
}


bool stream::RunModule::ProcessNextBuffer()
{
   dabc::Buffer buf = Recv();

   if (buf.GetTypeId() == dabc::mbt_EOF) {
      if (fParallel<0) {
         std::string main = GetName();
         main.resize(main.length()-3);
         dabc::mgr.FindModule(main).Submit(dabc::Command("SlaveFinished"));
      }
      return true;
   }

   fTotalSize += buf.GetTotalSize();

   if (buf.GetTypeId() == mbs::mbt_MbsEvents) {
      mbs::ReadIterator iter(buf);
      while (iter.NextEvent()) {
         if (iter.NextSubEvent())
            ProcessNextEvent(iter.rawdata(), iter.rawdatasize());
      }
   } else {
      hadaq::ReadIterator iter(buf);
      while (iter.NextEvent()) {
         ProcessNextEvent(iter.evnt(), iter.evntsize());
      }
   }

   return true;
}

bool stream::RunModule::RedistributeBuffers()
{
   while (CanRecv()) {

      unsigned indx(0), max(0), min(10);
      for (unsigned n=0;n<NumOutputs();n++) {
         unsigned can = NumCanSend(n);
         if (can>max) { max = can; indx = n; }
         if (can<min) min = can;
      }
      if (max==0) return false;

      if ((min==0) && (RecvQueueItem().GetTypeId() == dabc::mbt_EOF)) return false;

      dabc::Buffer buf = Recv();

      if (buf.GetTypeId() == dabc::mbt_EOF) {
         fStopMode = fParallel;
         SendToAllOutputs(buf);
         DOUT0("END of FILE, DO SOMETHING");
         return false;
      }

      fTotalSize += buf.GetTotalSize();

      int cnt = 0;

      if (buf.GetTypeId() == mbs::mbt_MbsEvents) {
         mbs::ReadIterator iter(buf);
         while (iter.NextEvent()) cnt++;
      } else {
         hadaq::ReadIterator iter(buf);
         while (iter.NextEvent()) cnt++;
      }

      fTotalEvnts+=cnt;

      Par("Events").SetValue(cnt);

      //   DOUT0("Send buffer to output %d\n", indx);

      Send(indx, buf);
   }

   return false;
}

bool stream::RunModule::ProcessRecv(unsigned port)
{
   if (fParallel<=0) {
      if (CanRecv()) return ProcessNextBuffer();
      return true;
   }

   return RedistributeBuffers();
}

