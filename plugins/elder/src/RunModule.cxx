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

#include "elderdabc/RunModule.h"
#include "elderdabc/VisConDabc.h"

#include "dabc/Manager.h"
#include "dabc/Factory.h"
#include "dabc/Iterator.h"
#include "dabc/Buffer.h"
#include "dabc/Publisher.h"
#include "dabc/Url.h"
#include "dabc/BinaryFile.h"

#include "mbs/Iterator.h"

#include <cstdlib>
#include <sys/time.h>



#include <elderpt.hpp>


// ==================================================================================

elderdabc::RunModule::RunModule(const std::string &name, dabc::Command cmd) :
      dabc::ModuleAsync(name, cmd), fViscon(nullptr), fAnalysis(nullptr), fElderConfigFile(), fAsf(), fTotalSize(
            0), fTotalEvnts(0), fTotalOutEvnts(0), fDefaultFill(5)
{
   EnsurePorts(1, 0); // we need one input and no outputs
   fDefaultFill = Cfg("FillColor", cmd).AsInt(3);
   fElderConfigFile = Cfg("ElderConfig", cmd).AsStr("./analysis");
   fWorkerHierarchy.Create("Worker");
   dabc::Hierarchy h;
   CreateTimer("Update", 1.);

   //fWorkerHierarchy.CreateHChild("Status").SetField("_hidden", "true");
   //fWorkerHierarchy.SetField("_player", "DABC.ElderControl");

   fAsf = Cfg("AutosaveFile", cmd).AsStr();
   // do not autosave is specified, module will not stop when data source disappears
   if (fAsf.empty())
      SetAutoStop(false);

   h = fWorkerHierarchy.CreateHChild("Control/Restart");
    h.SetField("_kind", "Command");
    h.SetField("_title", "Restart processing (reinit data source)");
    h.SetField("_icon", "dabc_plugins/elder/icons/restart.png");
    h.SetField("_fastcmd", "true");
    h.SetField("_numargs", "0");

   h = fWorkerHierarchy.CreateHChild("Control/Start");
   h.SetField("_kind", "Command");
   h.SetField("_title", "Start processing of data");
   h.SetField("_icon", "dabc_plugins/elder/icons/start.png");
   h.SetField("_fastcmd", "true");
   h.SetField("_numargs", "0");

   h = fWorkerHierarchy.CreateHChild("Control/Stop");
   h.SetField("_kind", "Command");
   h.SetField("_title", "Stop processing of data");
   h.SetField("_icon", "dabc_plugins/elder/icons/stop.png");
   h.SetField("_fastcmd", "true");
   h.SetField("_numargs", "0");

   h = fWorkerHierarchy.CreateHChild("Control/Clear");
   h.SetField("_kind", "Command");
   h.SetField("_title", "Clear all histograms in the server");
   h.SetField("_icon", "dabc_plugins/elder/icons/clear.png");
   h.SetField("_fastcmd", "true");
   h.SetField("_numargs", "0");

   h = fWorkerHierarchy.CreateHChild("Control/Save");
   h.SetField("_kind", "Command");
   h.SetField("_title", "Save all histograms in the dabc.root file");
   h.SetField("_icon", "dabc_plugins/elder/icons/save.png");
   h.SetField("_fastcmd", "true");
   h.SetField("_numargs", "0");

   // JAM24: keep these commands for future usage?
//   dabc::CommandDefinition cmddef = fWorkerHierarchy.CreateHChild("Control/StartRootFile");
//   cmddef.SetField(dabc::prop_kind, "DABC.Command");
//   // cmddef.SetField(dabc::prop_auth, true); // require authentication
//   cmddef.AddArg("fname", "string", true, "file.root");
//   cmddef.AddArg("kind", "int", false, "2");
//   cmddef.AddArg("maxsize", "int", false, "1900");
//
//   cmddef = fWorkerHierarchy.CreateHChild("Control/StopRootFile");
//   cmddef.SetField(dabc::prop_kind, "DABC.Command");
   // cmddef.SetField(dabc::prop_auth, true); // require authentication


   fViscon = new VisConDabc();
   fViscon->SetTop(fWorkerHierarchy, true);
   fViscon->SetDefaultFill(fDefaultFill);
   fAnalysis = new ::elderpt::control::Controller(fElderConfigFile, fViscon);

   if (fAnalysis->errors() == 0) {
      fAnalysis->create_histograms(*fViscon);
   } else {
      EOUT("There were %d errors while parsing file %s ! \n",
            fAnalysis->errors(), fElderConfigFile.c_str());
      //exit(); TODO proper error handling in dabc
   }

   fEvent = new ::elderpt::control::Event();
   CreatePar("Events").SetRatemeter(false, 3.).SetUnits("Ev");
   CreatePar("DataRate").SetRatemeter(false, 3.).SetUnits("MB");
   PublishPars(dabc::format("$CONTEXT$/%s", GetName())); // will also publish rest of hierarchy...
   //Publish(fWorkerHierarchy, dabc::format("$CONTEXT$/%s", GetName()));

   double interval = Cfg("AutosaveInterval", cmd).AsDouble(0);
   if (interval > 1)
      CreateTimer("AutoSave", interval);
   fViscon->AddRunLog("elderdabc::RunModule::RunModule is created.");
}

elderdabc::RunModule::~RunModule()
{
   if(fEvent) delete fEvent;
   if (fAnalysis) {
      delete fAnalysis;
      fAnalysis = nullptr;
   }
   if (fViscon) {
      fViscon->AddRunLog("elderdabc::RunModule::RunModule is terminated.");
         delete fViscon;
         fViscon = nullptr;
      }



   //printf("elderdabc::RunModule dtor ended \n");
}



void elderdabc::RunModule::OnThreadAssigned()
{
   dabc::ModuleAsync::OnThreadAssigned();

   // JAM24: optionally do something for the parallel setup here,  like in stream...

   DOUT0("!!!! Assigned to thread %s  !!!!!", thread().GetName());
}




int elderdabc::RunModule::ExecuteCommand(dabc::Command cmd)
{
//   fViscon->AddRunLog(
//         dabc::format("elderdabc::RunModule::ExecuteCommand is called for %s",
//               cmd.GetName()).c_str());

   std::string res = "true";
   std::string name = cmd.GetName();
   if (name == "ROOTCMD") {
      dabc::Hierarchy item = cmd.GetRef("item");
      if (item.null())
         return false;
      fViscon->AddRunLog(
              dabc::format("elderdabc::RunModule::ExecuteCommand is called for %s",
                    item.GetName()).c_str());

      if (item.IsName("Clear")) {
         DOUT0("Call CLEAR");
         fViscon->ClearAllHistograms();
      } else if (item.IsName("Save")) {
         DOUT0("Call SAVE");
         fViscon->SaveAllHistograms();
      } else if (item.IsName("Start")) {
         DOUT0("Call START");
         //Start(); // better start whole application here, see below
         dabc::mgr.app().ChangeState(dabc::Application::stRunning());
      } else if (item.IsName("Stop")) {
         DOUT0("Call STOP");
         //Stop(); - will stop only module and then application will terminate us by default
         dabc::mgr.app().ChangeState(dabc::Application::stReady()); // better: change overall running state
      } else if (item.IsName("Restart")) {
         DOUT0("Call RESTART");
//         dabc::mgr.app().ChangeState(dabc::Application::stHalted()); // shutdown
//         dabc::mgr.app().ChangeState(dabc::Application::stRunning()); // init again and run it
         //dabc::mgr.app().Execute(dabc::Application::stcmdDoHalt());
         //fRestart=true; // postpone this to our dtor?

         //dabc::mgr.app().Execute(dabc::Application::stcmdDoStart()); // we are gone before this is done :-) need to execute this in the applicatio thread...

         dabc::mgr.app().Submit(dabc::Command(dabc::Application::stcmdDoHalt()));
         dabc::mgr.app().Submit(dabc::Command(dabc::Application::stcmdDoStart()));

      } else {
         res = "false";
      }
      cmd.SetStrRawData(res);
      return res == "true" ? true : false;
   }
   return dabc::ModuleAsync::ExecuteCommand(cmd);
}




void elderdabc::RunModule::BeforeModuleStart()
{
   DOUT0("START ELDER MODULE %s inp %s", GetName(), DBOOL(IsInputConnected(0)));
}

void elderdabc::RunModule::SaveHierarchy(dabc::Buffer buf)
{
   if (buf.GetTotalSize() == 0) return;

   DOUT0("store hierarchy size %d in temporary h.bin file", buf.GetTotalSize());
   {
      dabc::BinaryFile f;
      std::system("rm -f h.bin");
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

   int res = std::system(args.c_str());

   if (res != 0) EOUT("Fail to convert DABC histograms in ROOT file, check h.bin file");
            else std::system("rm -f h.bin");
}

void elderdabc::RunModule::AfterModuleStop()
{
   DOUT0("STOP ELDER MODULE %s data %lu evnts %lu outevents %lu  %s", GetName(), fTotalSize, fTotalEvnts, fTotalOutEvnts, (fTotalEvnts == fTotalOutEvnts ? "ok" : "MISSMATCH"));

  if (fAsf.length() > 0) {
      SaveHierarchy(fWorkerHierarchy.SaveToBuffer());
   }
}

bool elderdabc::RunModule::ProcessNextEvent(mbs::ReadIterator& iter)
{
   if (!fAnalysis)
      return false;
   DOUT5("elderdabc::RunModule::ProcessNextEvent() for event %lu", fTotalEvnts);
   fTotalEvnts++;
   Par("Events").SetValue(1);
// first get event header
   struct timeval tv;
   gettimeofday(&tv,0);
   static uint64_t first_timestamp_ = tv.tv_sec*1000 + tv.tv_usec/1000;
   uint64_t now = (tv.tv_sec*1000 + tv.tv_usec/1000);
   uint64_t timestamp = now-first_timestamp_;

   int      time      = timestamp/1000;
   int      msec      = timestamp%1000;
   // TODO: access to mbs buffer header time?

   mbs::EventHeader* head=iter.evnt();

   // convert the MBS event structure into a elder Event class
//    elderpt::control::Event event(head->EventNumber(),
//                                head->Type(),
//                                head->TriggerNumber(),
//                                time,
//                                msec,
//                                timestamp);
   fEvent->clear();
   fEvent->set_number(head->EventNumber());
   fEvent->set_type(head->Type());
   fEvent->set_trigger(head->TriggerNumber());
   fEvent->set_time(time);
   fEvent->set_msec(msec);
   fEvent->set_timestamp(timestamp);

   while (iter.NextSubEvent())
   {
      // scan subevent here
      // loop over subevents and convert them into Elder-PT format
      mbs::SubeventHeader* subhead = iter.subevnt();
      fEvent->push_back(::elderpt::control::Subevent(subhead->ProcId(),
                                              subhead->Type(),
                                              subhead->SubType(),
                                              subhead->Control(),
                                              subhead->Subcrate(),
                                              iter.rawdatasize()/sizeof(uint32_t), //subevt->GetIntLen(),
                                              reinterpret_cast<const uint32_t*>(iter.rawdata())
                                              ));
   }

   fAnalysis->unpack(*fViscon , *fEvent);
   fAnalysis->process(*fViscon);

   return true;
}


bool elderdabc::RunModule::ProcessNextBuffer()
{

   //printf("elderdabc::RunModule enters ProcessNextBuffer... fAnalysis=0x%x\n",fAnalysis);
   if (!fAnalysis) return false;
   dabc::Buffer buf = Recv();
   //printf("elderdabc::RunModule::ProcessNextBuffer after rcv =0x%x\n",fAnalysis);
   Par("DataRate").SetValue(buf.GetTotalSize()/1024./1024.);
   fTotalSize += buf.GetTotalSize();
   DOUT5("elderdabc::RunModule::ProcessNextBuffer() has total size %lu", fTotalSize);

   if (buf.GetTypeId() == mbs::mbt_MbsEvents) {
      DOUT5("elderdabc::RunModule::RunModule::ProcessNextBuffer() sees MBS type id %d", buf.GetTypeId());
      mbs::ReadIterator iter(buf);
      while (iter.NextEvent()) {
         ProcessNextEvent(iter);
      }
      }
      else if(buf.GetTypeId() == dabc::mbt_EOF) {
         DOUT0("elderdabc::RunModule::RunModule::ProcessNextBuffer() sees end of all files.");
         //Stop();
         return false;
      }
      else {
         EOUT("elderdabc::RunModule::ProcessNextBuffer() sees OTHER type id %d", buf.GetTypeId());
         return false;
      }

   return true;
}



bool elderdabc::RunModule::ProcessRecv(unsigned)
{
     DOUT5("elderdabc::RunModule::ProcessRecv() entering...");
      return ProcessNextBuffer();
}

void elderdabc::RunModule::ProcessTimerEvent(unsigned timer)
{
   if (TimerName(timer) == "AutoSave") {
      if (fAsf.length() > 0) {
            // overwrite single autosave file
            SaveHierarchy(fWorkerHierarchy.SaveToBuffer());
        }
      else
      {
         // dump histograms into file with timestamp
         if (fViscon) fViscon->SaveAllHistograms();
      }
      return;
   }



   // rest is for update timer JAM
//   dabc::Hierarchy folder = fWorkerHierarchy.FindChild("Status");
//   folder.SetField("EventsRate", Par("Events").GetField("value").AsDouble());
//   folder.SetField("EventsCount", (int64_t) fTotalEvnts);
   //folder.SetField("StoreInfo", fProcMgr->GetStoreInfo());

}

