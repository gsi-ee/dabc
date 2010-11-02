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

#include "mbs/CombinerModule.h"

#include "dabc/Port.h"
#include "dabc/PoolHandle.h"
#include "dabc/Manager.h"
#include "dabc/CommandDefinition.h"

#include <map>

mbs::CombinerModule::CombinerModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleAsync(name, cmd),
   fPool(0),
   fBufferSize(0),
   fInp(),
   fOut(0),
   fOutBuf(0),
   fTmCnt(0),
   fFileOutput(false),
   fServOutput(false),
   fBuildCompleteEvents(false),
   fCheckSubIds(false)
{

   fBufferSize = GetCfgInt(dabc::xmlBufferSize, 16384, cmd);

   std::string poolname = GetCfgStr(dabc::xmlPoolName, dabc::xmlWorkPool, cmd);

   fPool = CreatePoolHandle(poolname.c_str(), fBufferSize, 10);

   int numinp = GetCfgInt(dabc::xmlNumInputs, 2, cmd);

   fDoOutput = GetCfgBool(mbs::xmlNormalOutput, true, cmd);
   fFileOutput = GetCfgBool(mbs::xmlFileOutput, false, cmd);
   fServOutput = GetCfgBool(mbs::xmlServerOutput, false, cmd);

   fBuildCompleteEvents = GetCfgBool(mbs::xmlCombineCompleteOnly, true, cmd);
   fCheckSubIds = GetCfgBool(mbs::xmlCheckSubeventIds, true, cmd);

   fEventIdMask = GetCfgInt(mbs::xmlEvidMask, 0, cmd);
   if (fEventIdMask == 0) fEventIdMask = 0xffffffff;

   fEventIdTolerance = GetCfgInt(mbs::xmlEvidTolerance, 0, cmd);

   std::string ratesprefix = GetCfgStr(mbs::xmlCombinerRatesPrefix, "Mbs", cmd);

   fSpecialTriggerLimit = GetCfgInt(mbs::xmlSpecialTriggerLimit, 12, cmd);

   double flashtmout = GetCfgDouble(dabc::xmlFlashTimeout, 1., cmd);

   for (int n=0;n<numinp;n++) {
      CreateInput(FORMAT((mbs::portInputFmt, n)), fPool, 5);
      fInp.push_back(ReadIterator(0));
      fCfg.push_back(InputCfg());
   }

   if (fDoOutput) CreateOutput(mbs::portOutput, fPool, 5);
   if (fFileOutput) CreateOutput(mbs::portFileOutput, fPool, 5);
   if (fServOutput) CreateOutput(mbs::portServerOutput, fPool, 5);

   if (flashtmout>0.) CreateTimer("Flash", flashtmout, false);

   fDataRate = CreateRateParameter(FORMAT(("%sData", ratesprefix.c_str())), false, 1., "", "", "MB/s", 0., 0.);
   fEvntRate = CreateRateParameter(FORMAT(("%sEvents", ratesprefix.c_str())), false, 1., "", "", "Ev/s", 0., 0.);

   // must be configured in xml file
   //   fDataRate->SetDebugOutput(true);

   dabc::CommandDefinition* def = NewCmdDef(mbs::comStartFile);
   def->AddArgument(mbs::xmlFileName, dabc::argString, true);
   def->AddArgument(mbs::xmlSizeLimit, dabc::argInt, false, "1000");
   def->Register();

   NewCmdDef(mbs::comStopFile)->Register();

   def = NewCmdDef(mbs::comStartServer);
   def->AddArgument(mbs::xmlServerKind, dabc::argString, true, mbs::ServerKindToStr(mbs::StreamServer));
   def->Register();

   NewCmdDef(mbs::comStopServer)->Register();

   CreateParInfo("MbsCombinerInfo", 1, "Green");
   fLastInfoTm = TimeStamp();
   SetInfo(0, dabc::format("MBS combiner module ready. Mode: full events only:%d, subids check:%d" ,fBuildCompleteEvents,fCheckSubIds), true);
}

mbs::CombinerModule::~CombinerModule()
{
   if (fOutBuf!=0) {
      fOut.Close();
      dabc::Buffer::Release(fOutBuf);
      fOutBuf = 0;
   }
}

void mbs::CombinerModule::SetInfo(int lvl, const std::string& info, bool forceinfo)
{
   dabc::Logger::Debug(lvl, __FILE__, __LINE__, __func__, info.c_str());

   dabc::TimeStamp_t tm = TimeStamp();

   if (forceinfo || (dabc::TimeDistance(fLastInfoTm, tm) > 2.0)) {
      SetParStr("MbsCombinerInfo", dabc::format("%s: %s", GetName(), info.c_str()));
      fLastInfoTm = tm;
      if (!forceinfo) DOUT0((info.c_str()));
   }
}


void mbs::CombinerModule::ProcessTimerEvent(dabc::Timer* timer)
{
   if (fTmCnt > 0) fTmCnt--;
   if (fTmCnt == 0) FlushBuffer();
}

void mbs::CombinerModule::ProcessInputEvent(dabc::Port* port)
{
   while (BuildEvent());
}

void mbs::CombinerModule::ProcessOutputEvent(dabc::Port* port)
{
   while (BuildEvent());
}

bool mbs::CombinerModule::FlushBuffer()
{
   if (fOutBuf==0) return false;

   if (fOut.IsEmpty()) return false;

   if (!CanSendToAllOutputs()) return false;

   fOut.Close();

   SendToAllOutputs(fOutBuf);

   fTmCnt = 2; // set 2 means that two timeout events should happen before flush will be triggered

   fOutBuf = 0;

   return true;
}

void mbs::CombinerModule::BeforeModuleStart()
{
   DOUT1(("mbs::CombinerModule::BeforeModuleStart name: %s is calling first build event...", GetName()));

   while (BuildEvent());

   DOUT1(("mbs::CombinerModule::BeforeModuleStart name: %s is finished", GetName()));

}

bool mbs::CombinerModule::ShiftToNextEvent(unsigned ninp)
{
   // always set event number to 0
   fCfg[ninp].curr_evnt_num = 0;
   fCfg[ninp].curr_evnt_special = false;

   bool foundevent(false);

   while (!foundevent) {

      if (!fInp[ninp].IsBuffer()) {
         dabc::Buffer* buf = Input(ninp)->FirstInputBuffer();

         if (buf==0) return false;

         if (!fInp[ninp].Reset(buf)) {

            // skip buffer and try again
            fInp[ninp].Reset(0);
            Input(ninp)->SkipInputBuffers(1);
            continue;
          }
      }

      bool res = fInp[ninp].NextEvent();

      if (!res || (fInp[ninp].evnt()==0)) {
         fInp[ninp].Reset(0);
         Input(ninp)->SkipInputBuffers(1);
         continue;
      }

      if (fCfg[ninp].real_mbs && (fInp[ninp].evnt()->iTrigger>=fSpecialTriggerLimit)) {
         // TODO: Probably, one should combine trigger events from all normal mbs channels.
         foundevent = true;
         fCfg[ninp].curr_evnt_special = true;
         fCfg[ninp].curr_evnt_num = fInp[ninp].evnt()->EventNumber();
         DOUT1(("Found special event with trigger %d on input %u", fInp[ninp].evnt()->iTrigger, ninp));
      } else
      if (fCfg[ninp].real_evnt_num) {
         foundevent = true;
         fCfg[ninp].curr_evnt_num = fInp[ninp].evnt()->EventNumber() & fEventIdMask;
      } else {
         mbs::SubeventHeader* subevnt = fInp[ninp].evnt()->SubEvents();
         fCfg[ninp].curr_evnt_num = 0;

         while (subevnt!=0) {
            // DOUT1(("Saw subevent fullid %u", subevnt->fFullId));
            if (subevnt->fFullId == fCfg[ninp].evntsrc_fullid) break;
            subevnt = fInp[ninp].evnt()->NextSubEvent(subevnt);
         }

         if (subevnt!=0) {
            uint32_t* data = (uint32_t*) (((uint8_t*) subevnt->RawData()) + fCfg[ninp].evntsrc_shift);

            if (fCfg[ninp].evntsrc_shift + sizeof(uint32_t) <= subevnt->RawDataSize()) {
               foundevent = true;
               fCfg[ninp].curr_evnt_num = *data & fEventIdMask; // take only required bits
               //               DOUT1(("Find subevent %u", fCfg[ninp].curr_evnt_num));
            } else {
               EOUT(("Subevent too small %u compare with required shift %u for id location", subevnt->RawDataSize(), fCfg[ninp].evntsrc_shift));
            }
         } else {
            EOUT(("Did not found subevent for id location"));
         }
      }
   }

   // DOUT1(("Inp%u Event%d", ninp, fCfg[ninp].curr_evnt_num));

   return true;
}



bool mbs::CombinerModule::BuildEvent()
{
   mbs::EventNumType mineventid(0), maxeventid(0), triggereventid(0);

   int hasTriggerEvent = -1;

   for (unsigned ninp=0; ninp<NumInputs(); ninp++) {

      fCfg[ninp].selected = false;

      if (fInp[ninp].evnt()==0)
         if (!ShiftToNextEvent(ninp)) return false;

      mbs::EventNumType evid = fCfg[ninp].curr_evnt_num;

      if (ninp==0)  {
         mineventid = evid;
         maxeventid = evid;
      } else {
         if (evid < mineventid) mineventid = evid; else
            if (evid > maxeventid) maxeventid = evid;
      }

      if (fCfg[ninp].curr_evnt_special && (hasTriggerEvent<0)) {
         hasTriggerEvent = ninp;
         triggereventid = evid;
      }

   } // for ninp

   // we always try to build event with minimum id
   mbs::EventNumType buildevid(mineventid);
   mbs::EventNumType diff = maxeventid - mineventid;

   // if any trigger event found, it will be send as is
   if (hasTriggerEvent>=0) {
      buildevid = triggereventid;
      fCfg[hasTriggerEvent].selected = true;
      diff = 0;

   } else {

      // but due to event counter overflow one should build event with maxid
      if (diff > fEventIdMask/2) {
         buildevid = maxeventid;
         diff = fEventIdMask - diff + 1;
      }

      if ((fEventIdTolerance > 0) && (diff > fEventIdTolerance)) {
         SetInfo(-1, dabc::format("Event id difference %u exceeding tolerance window %u, stopping dabc!", diff, fEventIdTolerance), true);
         dabc::mgr()->ChangeState(dabc::Manager::stcmdDoStop);
         return false; // need to return immediately after stop state is set
      }

      // select inputs which will be used for building
      for (unsigned ninp = 0; ninp < NumInputs(); ninp++)
         if (fCfg[ninp].curr_evnt_num == buildevid)
            fCfg[ninp].selected = true;
   }

   // calculated result event size and define if mbs header is available
   // also check here if all subids are unique
   uint32_t subeventssize = 0;
   // define number of input which will be used to copy mbs header
   int copyMbsHdrId = -1;
   std::map<uint32_t, bool> subid_map;
   unsigned numusedinp = 0;

   // check of unique subevent ids:
   bool duplicatefound = false;

   for (unsigned ninp = 0; ninp < NumInputs(); ninp++) {
      if (!fCfg[ninp].selected) continue;

      numusedinp++;

      subeventssize += fInp[ninp].evnt()->SubEventsSize();
      if (fCfg[ninp].real_mbs && (copyMbsHdrId<0)) copyMbsHdrId = ninp;

      if (fCheckSubIds)
         while (fInp[ninp].NextSubEvent()) {
            uint32_t fullid = fInp[ninp].subevnt()->fFullId;
            if (subid_map.find(fullid) != subid_map.end()) {
               EOUT(("Duplicate fullid = 0x%x", fullid));
               duplicatefound = true;
            }
            subid_map[fullid] = true;
         }
   }

   if (fBuildCompleteEvents && (numusedinp < NumInputs() && (hasTriggerEvent<0))) {
      SetInfo(3, dabc::format("Skip incomplete event %u, found inputs %u required %u diff %u", buildevid, numusedinp, NumInputs(), diff));
   } else
   if (duplicatefound && (hasTriggerEvent<0)) {
      SetInfo(3, dabc::format("Skip event %u while duplicates subevents found", buildevid));
   } else {

      if (numusedinp < NumInputs())
         SetInfo(3, dabc::format("Build incomplete event %u, found inputs %u required %u diff %u", buildevid, numusedinp, NumInputs(), diff));
      else
         SetInfo(3, dabc::format("Build event %u with %u inputs", buildevid, numusedinp));

      // if there is no place for the event, flush current buffer
      if ((fOutBuf != 0) && !fOut.IsPlaceForEvent(subeventssize))
         if (!FlushBuffer()) return false;

      if (fOutBuf == 0) {

         fOutBuf = fPool->TakeBufferReq(fBufferSize);
         if (fOutBuf == 0) return false;

         if (!fOut.Reset(fOutBuf)) {
            SetInfo(-1, "Cannot use buffer for output - hard error!!!!", true);

            dabc::Buffer::Release(fOutBuf);
            fOutBuf = 0;
            dabc::mgr()->ChangeState(dabc::Manager::stcmdDoStop);
            return false;
         }
      }

      if (!fOut.IsPlaceForEvent(subeventssize)) {
         EOUT(("Event size %u too big for buffer %u, skip event %u completely", subeventssize+ sizeof(mbs::EventHeader), fBufferSize, buildevid));
      } else {

         if (copyMbsHdrId<0) {
            // SetInfo(-1, "No mbs eventid found in mbs event number mode, stop dabc", true);
            // dabc::Manager* mgr=dabc::Manager::Instance();
            // mgr->ChangeState(dabc::Manager::stcmdDoStop);
         }

         DOUT3(("Building event %u", buildevid));
         fOut.NewEvent(buildevid); // note: this header id may be overwritten due to mode
         dabc::Pointer ptr;
         for (unsigned ninp = 0; ninp < NumInputs(); ninp++) {

            if (fCfg[ninp].selected) {

               // if header id still not defined, used first
               if (copyMbsHdrId<0) copyMbsHdrId = ninp;

               fInp[ninp].AssignEventPointer(ptr);
               ptr.shift(sizeof(mbs::EventHeader));
               fOut.AddSubevent(ptr);
            }
         }

         fOut.evnt()->CopyHeader(fInp[copyMbsHdrId].evnt());

         fOut.FinishEvent();
         fEvntRate->AccountValue(1.);
         fDataRate->AccountValue((subeventssize + sizeof(mbs::EventHeader))/1024./1024.);

         // if output buffer filled already, flush it immediately
         if (!fOut.IsPlaceForEvent(0))
            FlushBuffer();
      }
   } // end of incomplete event

   for (unsigned ninp = 0; ninp < NumInputs(); ninp++)
      if (fCfg[ninp].selected) ShiftToNextEvent(ninp);

   // return true means that method can be called again immediately
   // in all places one requires while loop
   return true;
}


int mbs::CombinerModule::ExecuteCommand(dabc::Command* cmd)
{
   if (cmd->IsName(mbs::comStartFile)) {
      dabc::Port* port = FindPort(mbs::portFileOutput);
      if (port==0) port = CreateOutput(mbs::portFileOutput, fPool, 5);

      std::string filename = port->GetCfgStr(mbs::xmlFileName,"",cmd);
      int sizelimit = port->GetCfgInt(mbs::xmlSizeLimit,1000,cmd);

      SetInfo(1, dabc::format("Start mbs file %s sizelimit %d mb", filename.c_str(), sizelimit), true);

      dabc::Command* dcmd = new dabc::CmdCreateTransport(port->GetFullName(dabc::mgr()).c_str(), mbs::typeLmdOutput, "MbsFileThrd");
      dcmd->SetStr(mbs::xmlFileName, filename);
      dcmd->SetInt(mbs::xmlSizeLimit, sizelimit);
      return dabc::mgr()->Execute(dcmd);
   } else
   if (cmd->IsName(mbs::comStopFile)) {
      dabc::Port* port = FindPort(mbs::portFileOutput);
      if (port!=0) port->Disconnect();

      SetInfo(1, "Stop file", true);

      return dabc::cmd_true;
   } else
   if (cmd->IsName(mbs::comStartServer)) {
      dabc::Port* port = FindPort(mbs::portServerOutput);
      if (port==0) port = CreateOutput(mbs::portServerOutput, fPool, 5);

      std::string serverkind = port->GetCfgStr(mbs::xmlServerKind, "", cmd);
      int kind = StrToServerKind(serverkind.c_str());
      if (kind==mbs::NoServer) kind = mbs::TransportServer;
      serverkind = ServerKindToStr(kind);

      dabc::Command* dcmd = new dabc::CmdCreateTransport(port->GetFullName(dabc::mgr()).c_str(), mbs::typeServerTransport, "MbsServThrd");
      dcmd->SetStr(mbs::xmlServerKind, serverkind);
      dcmd->SetInt(mbs::xmlServerPort, mbs::DefualtServerPort(kind));
      int res = dabc::mgr()->Execute(dcmd);

      SetInfo(1, dabc::format("%s: %s mbs server %s port %d",GetName(),
            ((res==dabc::cmd_true) ? "Start" : " Fail to start"), serverkind.c_str(), DefualtServerPort(kind)), true);

      return res;
   } else
   if (cmd->IsName(mbs::comStopServer)) {
      dabc::Port* port = FindPort(mbs::portServerOutput);
      if (port!=0) port->Disconnect();

      SetInfo(1, "Stop server", true);
      return dabc::cmd_true;
   } else
   if (cmd->IsName("ConfigureInput")) {
      unsigned ninp = cmd->GetUInt("Port", 0);
      if (ninp<fCfg.size()) {
         fCfg[ninp].real_mbs = cmd->GetBool("RealMbs", fCfg[ninp].real_mbs);
         fCfg[ninp].real_evnt_num = cmd->GetBool("RealEvntNum", fCfg[ninp].real_evnt_num);
         fCfg[ninp].evntsrc_fullid = cmd->GetUInt("EvntSrcFullId", fCfg[ninp].evntsrc_fullid);
         fCfg[ninp].evntsrc_shift = cmd->GetUInt("EvntSrcShift", fCfg[ninp].evntsrc_shift);

         std::string ratename = cmd->GetStr("RateName", "");
         if (ratename.length()>0)
            CreateRateParameter(ratename.c_str(), false, 1., Input(ninp)->GetName());

         DOUT1(("Configure input%u of module %s: RealMbs:%s RealEvntNum:%s EvntSrcFullId: 0x%x EvntSrcShift: %u",
               ninp, GetName(),
               DBOOL(fCfg[ninp].real_mbs), DBOOL(fCfg[ninp].real_evnt_num),
               fCfg[ninp].evntsrc_fullid, fCfg[ninp].evntsrc_shift));

      }

      return dabc::cmd_true;
   }



   return dabc::ModuleAsync::ExecuteCommand(cmd);

}

unsigned int mbs::CombinerModule::GetOverflowEventNumber() const
{
   return 0xffffffff;
}

// _________________________________________________________________________


extern "C" void StartMbsCombiner()
{
   if (dabc::mgr()==0) {
      EOUT(("Manager is not created"));
      exit(1);
   }

   DOUT0(("Create MBS combiner module"));

   mbs::CombinerModule* m = new mbs::CombinerModule("Combiner");
   dabc::mgr()->MakeThreadForModule(m);

   //    dabc::mgr()->CreateMemoryPool(dabc::xmlWorkPool,
   //                                  m->GetCfgInt(dabc::xmlBufferSize, 8192),
   //                                  m->GetCfgInt(dabc::xmlNumBuffers, 100));

   for (unsigned n=0;n<m->NumInputs();n++)
      if (!dabc::mgr()->CreateTransport(FORMAT(("Combiner/Input%u", n)), mbs::typeClientTransport, "MbsInpThrd")) {
         EOUT(("Cannot create MBS client transport"));
         exit(131);
      }

   if (m->IsServOutput()) {
      if (!dabc::mgr()->CreateTransport("Combiner/ServerOutput", mbs::typeServerTransport, "MbsServThrd")) {
         EOUT(("Cannot create MBS server"));
         exit(132);
      }
   }

   if (m->IsFileOutput())
      if (!dabc::mgr()->CreateTransport("Combiner/FileOutput", mbs::typeLmdOutput, "MbsFileThrd")) {
         EOUT(("Cannot create MBS file output"));
         exit(133);
      }

   //    m->Start();

   //    DOUT0(("Start MBS combiner module done"));
}



