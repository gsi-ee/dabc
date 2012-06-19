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

#include "hadaq/CombinerModule.h"

#include <math.h>

#include "dabc/Port.h"
#include "dabc/PoolHandle.h"
#include "dabc/Application.h"
#include "dabc/Manager.h"

#include "hadaq/HadaqTypeDefs.h"
#include "mbs/MbsTypeDefs.h"


hadaq::CombinerModule::CombinerModule(const char* name,  dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fBufferSize(0),
   fInp(),
   fOut(),
   fFlushFlag(false),
   fFileOutput(false),
   fServOutput(false),
   fBuildCompleteEvents(true)
{

   fBufferSize = Cfg(dabc::xmlBufferSize,cmd).AsInt(16384);

   std::string poolname = Cfg(dabc::xmlPoolName, cmd).AsStdStr(dabc::xmlWorkPool);

   CreatePoolHandle(poolname.c_str());

   int numinp = Cfg(dabc::xmlNumInputs,cmd).AsInt(1);

   fDoOutput = Cfg(hadaq::xmlNormalOutput,cmd).AsBool(true);
   fFileOutput = Cfg(hadaq::xmlFileOutput,cmd).AsBool(false);
//   fServOutput = Cfg(hadaq::xmlServerOutput,cmd).AsBool(false);

//     fBuildCompleteEvents = Cfg(mbs::xmlCombineCompleteOnly,cmd).AsBool(true);
//
//
//     fEventIdTolerance = Cfg(mbs::xmlEvidTolerance,cmd).AsInt(0);
//
//     std::string ratesprefix = Cfg(mbs::xmlCombinerRatesPrefix, cmd).AsStdStr("Hadaq");
   std::string ratesprefix ="Hadaq";

     double flushtmout = Cfg(dabc::xmlFlushTimeout,cmd).AsDouble(1.);

     for (int n=0;n<numinp;n++) {
        CreateInput(FORMAT((hadaq::portInputFmt, n)), Pool(), 10);

  //      DOUT0(("  Port%u: Capacity %u", n, Input(n)->InputQueueCapacity()));

        fInp.push_back(ReadIterator());
        fCfg.push_back(InputCfg());
        fInp[n].Close();
        fCfg[n].Reset();
     }

//     fNumObligatoryInputs = NumInputs();
//
     if (fDoOutput) CreateOutput(hadaq::portOutput, Pool(), 5);
     if (fFileOutput) CreateOutput(hadaq::portFileOutput, Pool(), 5);
//     if (fServOutput) CreateOutput(hadaq::portServerOutput, Pool(), 5);

     if (flushtmout>0.) CreateTimer("FlushTimer", flushtmout, false);

     fEventRateName = ratesprefix+"Events";
     fDataRateName = ratesprefix+"Data";
     fInfoName = ratesprefix+"Info";

     CreatePar(fDataRateName).SetRatemeter(false, 3.).SetUnits("MB");
     CreatePar(fEventRateName).SetRatemeter(false, 3.).SetUnits("Ev");

     // must be configured in xml file
     //   fDataRate->SetDebugOutput(true);

//     CreateCmdDef(mbs::comStartFile).AddArg(mbs::xmlFileName, "string", true).AddArg(mbs::xmlSizeLimit, "int", false, "1000");
//
//     CreateCmdDef(mbs::comStopFile);
//
//     CreateCmdDef(mbs::comStartServer).AddArg(mbs::xmlServerKind, "string", true, mbs::ServerKindToStr(mbs::StreamServer));
//
//     CreateCmdDef(mbs::comStopServer);
//
     CreatePar(fInfoName, "info").SetSynchron(true, 2., false);
     SetInfo(dabc::format("HADAQ combiner module ready. Mode: flush:%3.1f" ,flushtmout), true);




}


void hadaq::CombinerModule::SetInfo(const std::string& info, bool forceinfo)
{

   Par(fInfoName).SetStr(info);
   if (forceinfo) Par(fInfoName).FireModified();
}




void hadaq::CombinerModule::ProcessTimerEvent(dabc::Timer* timer)
{
   if (fFlushFlag) FlushBuffer();
   fFlushFlag = true;
}

void hadaq::CombinerModule::ProcessInputEvent(dabc::Port* port)
{
   while (BuildEvent());
}

void hadaq::CombinerModule::ProcessOutputEvent(dabc::Port* port)
{
   while (BuildEvent());
}

void hadaq::CombinerModule::ProcessConnectEvent(dabc::Port* port)
{
   DOUT0(("HadaqCombinerModule ProcessConnectEvent %s", port->GetName()));
}

void hadaq::CombinerModule::ProcessDisconnectEvent(dabc::Port* port)
{
   DOUT0(("HadaqCombinerModule ProcessDisconnectEvent %s", port->GetName()));
}

bool hadaq::CombinerModule::FlushBuffer()
{
   if (fOut.IsEmpty() || !fOut.IsBuffer()) return false;

   if (!CanSendToAllOutputs()) return false;

   dabc::Buffer buf = fOut.Close();

   DOUT3(("Send buffer of size = %d", buf.GetTotalSize()));

   SendToAllOutputs(buf.HandOver());

   fFlushFlag = false; // indicate that next flush timeout one not need to send buffer

   return true;
}

void hadaq::CombinerModule::BeforeModuleStart()
{
   DOUT2(("hadaq::CombinerModule::BeforeModuleStart name: %s is calling first build event...", GetName()));

   // FIXME: why event processing already done here ???

   while (BuildEvent());

   DOUT2(("hadaq::CombinerModule::BeforeModuleStart name: %s is finished", GetName()));
}

void hadaq::CombinerModule::AfterModuleStop()
{
   // FIXME: we should process data which is remains in the input queues
   // probably, we could build incomplete events if they are allowed
}



bool hadaq::CombinerModule::ShiftToNextBuffer(unsigned ninp)
{
   fCfg[ninp].curr_evnt_num = 0;
   fCfg[ninp].selected = false;
   fCfg[ninp].valid = false;

   fInp[ninp].Close();
   Input(ninp)->SkipInputBuffers(1);

   return true;
}

bool hadaq::CombinerModule::ShiftToNextEvent(unsigned ninp)
{
   // always set event number to 0
//   fCfg[ninp].curr_evnt_num = 0;
//   fCfg[ninp].curr_evnt_special = false;
//   fCfg[ninp].valid = false;
//
//   bool foundevent(false);
//
//   while (!foundevent) {
//
//      if (!fInp[ninp].IsData()) {
//
//         if (Input(ninp)->InputPending()==0) return false;
//
//         if (!fInp[ninp].Reset(Input(ninp)->FirstInputBuffer())) {
//
//            // skip buffer and try again
//            fInp[ninp].Close();
//            Input(ninp)->SkipInputBuffers(1);
//            continue;
//          }
//      }
//
//      bool res = fInp[ninp].NextEvent();
//
//      if (!res || (fInp[ninp].evnt()==0)) {
//         fInp[ninp].Close();
//         Input(ninp)->SkipInputBuffers(1);
//         continue;
//      }
//
//      if (fCfg[ninp].real_hadaq && (fInp[ninp].evnt()->iTrigger>=fSpecialTriggerLimit)) {
//         // TODO: Probably, one should combine trigger events from all normal hadaq channels.
//         foundevent = true;
//         fCfg[ninp].curr_evnt_special = true;
//         fCfg[ninp].curr_evnt_num = fInp[ninp].evnt()->EventNumber();
//         DOUT1(("Found special event with trigger %d on input %u", fInp[ninp].evnt()->iTrigger, ninp));
//      } else
//      if (fCfg[ninp].real_evnt_num) {
//         foundevent = true;
//         fCfg[ninp].curr_evnt_num = fInp[ninp].evnt()->EventNumber() & fEventIdMask;
//
//         // DOUT1(("Find in input %u event %u", ninp, fCfg[ninp].curr_evnt_num));
//      } else
//      if (fCfg[ninp].no_evnt_num) {
//
//         // indicate that data in optional input was found, should be append to the next event
//         foundevent = true;
//
//      } else {
//         mbs::SubeventHeader* subevnt = fInp[ninp].evnt()->SubEvents();
//         fCfg[ninp].curr_evnt_num = 0;
//
//         while (subevnt!=0) {
//            // DOUT1(("Saw subevent fullid %u", subevnt->fFullId));
//            if (subevnt->fFullId == fCfg[ninp].evntsrc_fullid) break;
//            subevnt = fInp[ninp].evnt()->NextSubEvent(subevnt);
//         }
//
//         if (subevnt!=0) {
//            uint32_t* data = (uint32_t*) (((uint8_t*) subevnt->RawData()) + fCfg[ninp].evntsrc_shift);
//
//            if (fCfg[ninp].evntsrc_shift + sizeof(uint32_t) <= subevnt->RawDataSize()) {
//               foundevent = true;
//               fCfg[ninp].curr_evnt_num = *data & fEventIdMask; // take only required bits
//               //DOUT1(("Find in input %u event %u (in subevent)", ninp, fCfg[ninp].curr_evnt_num));
//            } else {
//               EOUT(("Subevent too small %u compare with required shift %u for id location", subevnt->RawDataSize(), fCfg[ninp].evntsrc_shift));
//            }
//         } else {
//            EOUT(("Did not found subevent for id location"));
//         }
//      }
//   }
//
//   // DOUT1(("Inp%u Event%d", ninp, fCfg[ninp].curr_evnt_num));
//
//   fCfg[ninp].valid = true;

   return true;
}



bool hadaq::CombinerModule::BuildEvent()
{
   hadaq::EventNumType mineventid(0), maxeventid(0), triggereventid(0);

   // indicate if some of main (non-optional) input queues are mostly full
   // if such queue will be found, incomplete event may be build when it is allowed

   int mostly_full(-1);
   bool required_missing(false);

//   for (unsigned ninp=0; ninp<fCfg.size(); ninp++) {
//      dabc::Port* port = Input(ninp);
//
////      DOUT0(("  Port%u: pending %u capacity %u", ninp, port->InputPending(), port->InputQueueCapacity()));
//
//      if (fCfg[ninp].no_evnt_num) continue;
//
//      if (!port->IsConnected()) required_missing = true;
//
//      if ((port->InputPending() + 1 >= port->InputQueueCapacity()) && (mostly_full<0))
//         mostly_full = (int) ninp;
//   }
//
//   if (required_missing && fBuildCompleteEvents) {
//      // if some of important input missing than we should clean our queues
//      // to let data flowing, no event will be produced to output
//
//      for (unsigned ninp=0; ninp<fCfg.size(); ninp++) {
//         dabc::Port* port = Input(ninp);
//         if ((port->InputPending() + 1 >= port->InputQueueCapacity()) ||
//             (fCfg[ninp].no_evnt_num && port->InputPending()>1)) {
//            ShiftToNextBuffer(ninp);
//            SetInfo(dabc::format("Skip buffer on input %u while some other input is disconnected", ninp));
//            // DOUT0(("Skip buffer on input %u",ninp));
//         }
//      }
//
//      // all queues now have at least one empty entry, one could wait for the next event
//      return false;
//   }
//
//   int hasTriggerEvent = -1;
//   int num_valid = 0;
//
//   for (unsigned ninp=0; ninp<fCfg.size(); ninp++) {
//
//      fCfg[ninp].selected = false;
//
//      if (fInp[ninp].evnt()==0)
//         if (!ShiftToNextEvent(ninp)) {
//            // if optional input is absent just continue
//            if (fCfg[ninp].no_evnt_num) continue;
//            // we can now exclude this input completely while some other is mostly full
//            if ((mostly_full>=0) && !fBuildCompleteEvents) continue;
//            return false;
//         }
//
//      if (fCfg[ninp].no_evnt_num) continue;
//
//      hadaq::EventNumType evid = fCfg[ninp].curr_evnt_num;
//
//      if (num_valid == 0)  {
//         mineventid = evid;
//         maxeventid = evid;
//      } else {
//         if (evid < mineventid) mineventid = evid; else
//            if (evid > maxeventid) maxeventid = evid;
//      }
//
//      num_valid++;
//
//      if (fCfg[ninp].curr_evnt_special && (hasTriggerEvent<0)) {
//         hasTriggerEvent = ninp;
//         triggereventid = evid;
//      }
//
//   } // for ninp
//
//   if (num_valid==0) return false;
//
//   // we always try to build event with minimum id
//   hadaq::EventNumType buildevid(mineventid);
//   hadaq::EventNumType diff = maxeventid - mineventid;
//
//   // if any trigger event found, it will be send as is
//   if (hasTriggerEvent>=0) {
//      buildevid = triggereventid;
//      fCfg[hasTriggerEvent].selected = true;
//      diff = 0;
//
//   } else {
//
//      // but due to event counter overflow one should build event with maxid
//      if (diff > fEventIdMask/2) {
//         buildevid = maxeventid;
//         diff = fEventIdMask - diff + 1;
//      }
//
//      if ((fEventIdTolerance > 0) && (diff > fEventIdTolerance)) {
//         SetInfo(dabc::format("Event id difference %u exceeding tolerance window %u, stopping dabc!", diff, fEventIdTolerance), true);
//         dabc::mgr.StopApplication();
//         return false; // need to return immediately after stop state is set
//      }
//
//      // select inputs which will be used for building
//      for (unsigned ninp = 0; ninp < fCfg.size(); ninp++)
//         if (fCfg[ninp].valid && ((fCfg[ninp].curr_evnt_num == buildevid) || fCfg[ninp].no_evnt_num))
//            fCfg[ninp].selected = true;
//   }
//
//   // calculated result event size and define if hadaq header is available
//   // also check here if all subids are unique
//   uint32_t subeventssize = 0;
//   // define number of input which will be used to copy hadaq header
//   int copyMbsHdrId = -1;
//   std::map<uint32_t, bool> subid_map;
//   unsigned numusedinp = 0;
//
//   // check of unique subevent ids:
//   bool duplicatefound = false;
//
//   int firstselected = -1;
//
//   for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
//      if (!fCfg[ninp].selected) continue;
//
//      if (!fCfg[ninp].no_evnt_num) {
//         // take into account only events with "normal" event number
//         if (firstselected<0) firstselected = ninp;
//         numusedinp++;
//         if (fCfg[ninp].real_hadaq && (copyMbsHdrId<0)) copyMbsHdrId = ninp;
//      }
//
//      subeventssize += fInp[ninp].evnt()->SubEventsSize();
//
//      if (fCheckSubIds)
//         while (fInp[ninp].NextSubEvent()) {
//            uint32_t fullid = fInp[ninp].subevnt()->fFullId;
//            if (subid_map.find(fullid) != subid_map.end()) {
//               EOUT(("Duplicate fullid = 0x%x", fullid));
//               duplicatefound = true;
//            }
//            subid_map[fullid] = true;
//         }
//   }
//
//   if (fBuildCompleteEvents && (numusedinp < NumObligatoryInputs()) && (hasTriggerEvent<0)) {
//      SetInfo(dabc::format("Skip incomplete event %u, found inputs %u required %u diff %u", buildevid, numusedinp, NumObligatoryInputs(), diff));
//   } else
//   if (duplicatefound && (hasTriggerEvent<0)) {
//      SetInfo(dabc::format("Skip event %u while duplicates subevents found", buildevid));
//   } else {
//
//      if (numusedinp < NumObligatoryInputs()) {
//         SetInfo(dabc::format("Build incomplete event %u, found inputs %u required %u first %d diff %u mostly_full %d", buildevid, numusedinp, NumObligatoryInputs(), firstselected, diff, mostly_full));
//         EOUT(("%s Build incomplete event %u, found inputs %u required %u first %d diff %u mostly_full %d", GetName(), buildevid, numusedinp, NumObligatoryInputs(), firstselected, diff, mostly_full));
//      } else
//         SetInfo(dabc::format("Build event %u with %u inputs", buildevid, numusedinp));
//
//      // if there is no place for the event, flush current buffer
//      if (fOut.IsBuffer() && !fOut.IsPlaceForEvent(subeventssize))
//         if (!FlushBuffer()) return false;
//
//      if (!fOut.IsBuffer()) {
//
//         dabc::Buffer buf = Pool()->TakeBufferReq(fBufferSize);
//         if (buf.null()) return false;
//
//         if (!fOut.Reset(buf)) {
//            SetInfo("Cannot use buffer for output - hard error!!!!", true);
//
//            buf.Release();
//
//            dabc::mgr.StopApplication();
//            return false;
//         }
//      }
//
//      if (!fOut.IsPlaceForEvent(subeventssize)) {
//         EOUT(("Event size %u too big for buffer %u, skip event %u completely", subeventssize+ sizeof(hadaq::EventHeader), fBufferSize, buildevid));
//      } else {
//
//         if (copyMbsHdrId<0) {
//            // SetInfo("No mbs eventid found in mbs event number mode, stop dabc", true);
//            // dabc::mgr.StopApplication();
//         }
//
//         DOUT4(("Building event %u num_valid %u", buildevid, num_valid));
//         fOut.NewEvent(buildevid); // note: this header id may be overwritten due to mode
//
//         for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
//
//            if (fCfg[ninp].selected) {
//
//               // if header id still not defined, used first
//               if (copyMbsHdrId<0) copyMbsHdrId = ninp;
//
//               if (!fInp[ninp].IsData())
//                  throw dabc::Exception("Input has no buffer but used for event building");
//
//               dabc::Pointer ptr;
//               fInp[ninp].AssignEventPointer(ptr);
//
//               ptr.shift(sizeof(mbs::EventHeader));
//
//               if (ptr.segmid()>100)
//                  throw dabc::Exception("Bad segment id");
//
//               fOut.AddSubevent(ptr);
//            }
//         }
//
//         fOut.evnt()->CopyHeader(fInp[copyMbsHdrId].evnt());
//
//         fOut.FinishEvent();
//
//         DOUT4(("Produced event %d subevents %u", buildevid, subeventssize));
//
//         Par(fEventRateName).SetInt(1);
//         Par(fDataRateName).SetDouble((subeventssize + sizeof(mbs::EventHeader))/1024./1024.);
//
//         // if output buffer filled already, flush it immediately
//         if (!fOut.IsPlaceForEvent(0))
//            FlushBuffer();
//      }
//   } // end of incomplete event

   for (unsigned ninp = 0; ninp < fCfg.size(); ninp++)
      if (fCfg[ninp].selected) ShiftToNextEvent(ninp);

   // return true means that method can be called again immediately
   // in all places one requires while loop
   return true;
}


int hadaq::CombinerModule::ExecuteCommand(dabc::Command cmd)
{
//   if (cmd.IsName(hadaq::comStartFile)) {
//      dabc::Port* port = FindPort(hadaq::portFileOutput);
//      if (port==0) port = CreateOutput(hadaq::portFileOutput, Pool(), 5);
//
//      std::string filename = port->Cfg(hadaq::xmlFileName, cmd).AsStdStr();
//      int sizelimit = port->Cfg(hadaq::xmlSizeLimit,cmd).AsInt(1000);
//
//      SetInfo(dabc::format("Start hadaq file %s sizelimit %d mb", filename.c_str(), sizelimit), true);
//
//      dabc::CmdCreateTransport dcmd(port->ItemName(), mbs::typeLmdOutput, "MbsFileThrd");
//      dcmd.SetStr(mbs::xmlFileName, filename);
//      dcmd.SetInt(mbs::xmlSizeLimit, sizelimit);
//      return dabc::mgr.Execute(dcmd) ? dcmd.GetResult() : dabc::cmd_false;
//   } else
//   if (cmd.IsName(mbs::comStopFile)) {
//      dabc::Port* port = FindPort(mbs::portFileOutput);
//      if (port!=0) port->Disconnect();
//
//      SetInfo("Stop file", true);
//
//      return dabc::cmd_true;
//   } else
//   if (cmd.IsName(mbs::comStartServer)) {
//      dabc::Port* port = FindPort(mbs::portServerOutput);
//      if (port==0) port = CreateOutput(mbs::portServerOutput, Pool(), 5);
//
//      std::string serverkind = port->Cfg(mbs::xmlServerKind, cmd).AsStdStr();
//      int kind = StrToServerKind(serverkind.c_str());
//      if (kind==mbs::NoServer) kind = mbs::TransportServer;
//      serverkind = ServerKindToStr(kind);
//
//      dabc::CmdCreateTransport dcmd(port->ItemName(), mbs::typeServerTransport, "MbsServThrd");
//      dcmd.SetStr(mbs::xmlServerKind, serverkind);
//      dcmd.SetInt(mbs::xmlServerPort, mbs::DefualtServerPort(kind));
//      bool res = dabc::mgr.Execute(dcmd);
//
//      SetInfo(dabc::format("%s: %s mbs server %s port %d",GetName(),
//            (res ? "Start" : " Fail to start"), serverkind.c_str(), DefualtServerPort(kind)), true);
//
//      return res ? dcmd.GetResult() : dabc::cmd_false;
//   } else
//   if (cmd.IsName(mbs::comStopServer)) {
//      dabc::Port* port = FindPort(mbs::portServerOutput);
//      if (port!=0) port->Disconnect();
//
//      SetInfo("Stop server", true);
//      return dabc::cmd_true;
//   } else
//   if (cmd.IsName("ConfigureInput")) {
//      unsigned ninp = cmd.GetUInt("Port", 0);
////      DOUT0(("Start input configure %u size %u", ninp, fCfg.size()));
//      if (ninp<fCfg.size()) {
//
////         DOUT0(("Do0 input configure %u size %u", ninp, fCfg.size()));
//
//         fCfg[ninp].real_mbs = cmd.GetBool("RealMbs", fCfg[ninp].real_mbs);
//         fCfg[ninp].real_evnt_num = cmd.GetBool("RealEvntNum", fCfg[ninp].real_evnt_num);
//         fCfg[ninp].no_evnt_num = cmd.GetBool("NoEvntNum", fCfg[ninp].no_evnt_num);
//
////         DOUT0(("Do1 input configure %u size %u", ninp, fCfg.size()));
//         fCfg[ninp].evntsrc_fullid = cmd.GetUInt("EvntSrcFullId", fCfg[ninp].evntsrc_fullid);
//         fCfg[ninp].evntsrc_shift = cmd.GetUInt("EvntSrcShift", fCfg[ninp].evntsrc_shift);
//
////         DOUT0(("Do2 input configure %u size %u", ninp, fCfg.size()));
//
//         std::string ratename = cmd.GetStdStr("RateName", "");
//         if (!ratename.empty())
//            Input(ninp)->SetInpRateMeter(CreatePar(ratename).SetRatemeter(false,1.));
//
////         DOUT0(("Do3 input configure %u size %u", ninp, fCfg.size()));
//
//         if (fCfg[ninp].no_evnt_num) {
//            fCfg[ninp].real_mbs = false;
//            if (fNumObligatoryInputs>1) fNumObligatoryInputs--;
//         }
//
//         DOUT1(("Configure input%u of module %s: RealMbs:%s RealEvntNum:%s EvntSrcFullId: 0x%x EvntSrcShift: %u",
//               ninp, GetName(),
//               DBOOL(fCfg[ninp].real_mbs), DBOOL(fCfg[ninp].real_evnt_num),
//               fCfg[ninp].evntsrc_fullid, fCfg[ninp].evntsrc_shift));
//
////         DOUT0(("Do4 input configure %u size %u", ninp, fCfg.size()));
//
//      }
//
//      return dabc::cmd_true;
//   }
//


   return dabc::ModuleAsync::ExecuteCommand(cmd);

}





extern "C" void InitHadaqEvtbuilder()
{
   if (dabc::mgr.null()) {
      EOUT(("Manager is not created"));
      exit(1);
   }

   DOUT0(("Create Hadaq combiner module"));

   dabc::mgr.CreateMemoryPool(dabc::xmlWorkPool);

   hadaq::CombinerModule* m = new hadaq::CombinerModule("Combiner");
   dabc::mgr()->MakeThreadFor(m);

   for (unsigned n=0;n<m->NumInputs();n++){
      if (!dabc::mgr.CreateTransport(FORMAT(("Combiner/%s%u", hadaq::portInput, n)), hadaq::typeUdpInput, "UdpThrd")) {
         EOUT(("Cannot create Netmem client transport %d",n));
         exit(131);
      }
   }
   if (m->IsServOutput()) {
         DOUT0(("Create Mbs transmitter module for server"));
         dabc::mgr.CreateModule("hadaq::MbsTransmitterModule", "OnlineServer", "OnlineThrd");
         dabc::mgr.CreateTransport(FORMAT(("OnlineServer/%s", hadaq::portInput)), FORMAT(("Combiner/%s", hadaq::portServerOutput)), "OnlineThrd");
         dabc::mgr.CreateTransport(FORMAT(("OnlineServer/%s", hadaq::portOutput)), mbs::typeServerTransport, "MbsTransport");
   }

   if (m->IsFileOutput())
      if (!dabc::mgr.CreateTransport("Combiner/FileOutput", hadaq::typeHldOutput, "HldFileThrd")) {
         EOUT(("Cannot create HLD file output"));
         exit(133);
      }

   //    m->Start();

   //    DOUT0(("Start MBS combiner module done"));
}











