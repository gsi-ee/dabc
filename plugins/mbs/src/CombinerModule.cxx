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

#include "mbs/CombinerModule.h"

#include <map>

#include "dabc/Manager.h"


mbs::CombinerModule::CombinerModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fInp(),
   fOut(),
   fFlushFlag(false),
   fBuildCompleteEvents(false),
   fCheckSubIds(false)
{
   EnsurePorts(0, 0, dabc::xmlWorkPool);

   fBuildCompleteEvents = Cfg(mbs::xmlCombineCompleteOnly,cmd).AsBool(true);
   fCheckSubIds = Cfg(mbs::xmlCheckSubeventIds,cmd).AsBool(true);

   fEventIdMask = Cfg(mbs::xmlEvidMask,cmd).AsInt(0);
   if (fEventIdMask == 0) fEventIdMask = 0xffffffff;

   fEventIdTolerance = Cfg(mbs::xmlEvidTolerance,cmd).AsInt(0);

   std::string ratesprefix = Cfg(mbs::xmlCombinerRatesPrefix, cmd).AsStr("Mbs");

   fSpecialTriggerLimit = Cfg(mbs::xmlSpecialTriggerLimit,cmd).AsInt(12);

   fExcludeTime = Cfg("ExcludeTime", cmd).AsDouble(5.);

   double flushtmout = Cfg(dabc::xmlFlushTimeout,cmd).AsDouble(1.);

   for (unsigned n=0;n<NumInputs();n++) {
      DOUT0(" MBS COMBINER  Port%u: Capacity %u", n, InputQueueCapacity(n));

      fInp.push_back(ReadIterator());
      fCfg.push_back(InputCfg());
      fInp[n].Close();
      fCfg[n].Reset();
   }

   fNumObligatoryInputs = NumInputs();

   if (flushtmout>0.) CreateTimer("FlushTimer", flushtmout);

   fEventRateName = ratesprefix+"Events";
   fDataRateName = ratesprefix+"Data";
   fInfoName = ratesprefix+"Info";
   fFileStateName= ratesprefix + "FileOn";

   DOUT0("Create rate %s", fDataRateName.c_str());

   CreatePar(fDataRateName).SetRatemeter(false, 3.).SetUnits("MB");
   CreatePar(fEventRateName).SetRatemeter(false, 3.).SetUnits("Ev");

   // must be configured in xml file
   //   fDataRate->SetDebugOutput(true);

   CreateCmdDef(mbs::comStartFile)
      .AddArg(dabc::xmlFileName, "string", true)
      .AddArg(dabc::xmlFileSizeLimit, "int", false, 1000);

   CreateCmdDef(mbs::comStopFile);

   CreateCmdDef(mbs::comStartServer)
      .AddArg(mbs::xmlServerKind, "string", true, mbs::ServerKindToStr(mbs::StreamServer));

   CreateCmdDef(mbs::comStopServer);

   CreatePar(fInfoName, "info").SetSynchron(true, 2., false);
   CreatePar(fFileStateName).Dflt(false);

   PublishPars(dabc::format("$CONTEXT$/%sCombinerModule",ratesprefix.c_str()));

   SetInfo(dabc::format("MBS combiner module ready. Mode: full events only:%d, subids check:%d flush:%3.1f" ,fBuildCompleteEvents,fCheckSubIds,flushtmout), true);
}

mbs::CombinerModule::~CombinerModule()
{
    DOUT0("mbs::CombinerModule::DTOR - does nothing!");
}

void mbs::CombinerModule::ModuleCleanup()
{
   DOUT0("mbs::CombinerModule::ModuleCleanup()");

   fOut.Close().Release();
   for (unsigned n=0;n<fInp.size();n++)
      fInp[n].Reset();
}


void mbs::CombinerModule::SetInfo(const std::string &info, bool forceinfo)
{

   Par(fInfoName).SetValue(info);

   if (forceinfo) Par(fInfoName).FireModified();

/*
   dabc::Logger::Debug(lvl, __FILE__, __LINE__, __func__, info.c_str());

   dabc::TimeStamp now = dabc::Now();

   if (forceinfo || (now > fLastInfoTm + 2.)) {
      Par("MbsCombinerInfo").SetStr(dabc::format("%s: %s", GetName(), info.c_str()));
      fLastInfoTm = now;
      if (!forceinfo) DOUT0("%s: %s", GetName(), info.c_str());
   }
*/
}


void mbs::CombinerModule::ProcessTimerEvent(unsigned timer)
{
   if (fFlushFlag) {
      unsigned cnt = 0;
      while (IsRunning() && (cnt<100) && BuildEvent()) ++cnt;
      FlushBuffer();
   }
   fFlushFlag = true;
}

bool mbs::CombinerModule::FlushBuffer()
{
   if (fOut.IsEmpty() || !fOut.IsBuffer()) return false;

   if (!CanSendToAllOutputs()) return false;

   dabc::Buffer buf = fOut.Close();

   DOUT3("Send buffer of size = %d", buf.GetTotalSize());

   SendToAllOutputs(buf);

   fFlushFlag = false; // indicate that next flush timeout one not need to send buffer

   return true;
}

void mbs::CombinerModule::BeforeModuleStart()
{
   DOUT0("mbs::CombinerModule::BeforeModuleStart name: %s is calling first build event...", GetName());


   // FIXME: why event processing already done here ???

   unsigned cnt=0;

   while (cnt<100 && BuildEvent()) cnt++;

   DOUT0("mbs::CombinerModule::BeforeModuleStart name: %s is finished %u %u", GetName(), NumInputs(), NumOutputs());
}

void mbs::CombinerModule::AfterModuleStop()
{
   // FIXME: we should process data which is remains in the input queues
   // probably, we could build incomplete events if they are allowed
}

bool mbs::CombinerModule::ShiftToNextBuffer(unsigned ninp)
{
   fCfg[ninp].curr_evnt_num = 0;
   fCfg[ninp].curr_evnt_special = false;
   fCfg[ninp].valid = false;

   fInp[ninp].Close();
   SkipInputBuffers(ninp, 1);

   return true;
}

bool mbs::CombinerModule::ShiftToNextEvent(unsigned ninp)
{
   // always set event number to 0
   fCfg[ninp].curr_evnt_num = 0;
   fCfg[ninp].curr_evnt_special = false;
   fCfg[ninp].valid = false;

   bool foundevent(false);

   while (!foundevent) {

      if (!fInp[ninp].IsData()) {

         if (NumCanRecv(ninp)==0) return false;

         if (!fInp[ninp].Reset(RecvQueueItem(ninp, 0))) {

            // skip buffer and try again
            fInp[ninp].Close();
            SkipInputBuffers(ninp, 1);
            continue;
          }
      }

      bool res = fInp[ninp].NextEvent();

      if (!res || (fInp[ninp].evnt()==0)) {
         fInp[ninp].Close();
         SkipInputBuffers(ninp, 1);
         continue;
      }

      if (fCfg[ninp].real_mbs && (fInp[ninp].evnt()->iTrigger>=fSpecialTriggerLimit)) {
         // TODO: Probably, one should combine trigger events from all normal mbs channels.
         foundevent = true;
         fCfg[ninp].curr_evnt_special = true;
         fCfg[ninp].curr_evnt_num = fInp[ninp].evnt()->EventNumber();
         DOUT1("Found special event with trigger %d on input %u", fInp[ninp].evnt()->iTrigger, ninp);
      } else
      if (fCfg[ninp].real_evnt_num) {
         foundevent = true;
         fCfg[ninp].curr_evnt_num = fInp[ninp].evnt()->EventNumber() & fEventIdMask;

         // DOUT1("Find in input %u event %u", ninp, fCfg[ninp].curr_evnt_num);
      } else
      if (fCfg[ninp].no_evnt_num) {

         // indicate that data in optional input was found, should be append to the next event
         foundevent = true;

      } else {
         mbs::SubeventHeader* subevnt = fInp[ninp].evnt()->SubEvents();
         fCfg[ninp].curr_evnt_num = 0;

         while (subevnt!=0) {
            // DOUT1("Saw subevent fullid %u", subevnt->fFullId);
            if (subevnt->fFullId == fCfg[ninp].evntsrc_fullid) break;
            subevnt = fInp[ninp].evnt()->NextSubEvent(subevnt);
         }

         if (subevnt!=0) {
            uint32_t* data = (uint32_t*) (((uint8_t*) subevnt->RawData()) + fCfg[ninp].evntsrc_shift);

            if (fCfg[ninp].evntsrc_shift + sizeof(uint32_t) <= subevnt->RawDataSize()) {
               foundevent = true;
               fCfg[ninp].curr_evnt_num = *data & fEventIdMask; // take only required bits
               //DOUT1("Find in input %u event %u (in subevent)", ninp, fCfg[ninp].curr_evnt_num);
            } else {
               EOUT("Subevent too small %u compare with required shift %u for id location", subevnt->RawDataSize(), fCfg[ninp].evntsrc_shift);
            }
         } else {
            EOUT("Did not found subevent for id location");
         }
      }
   }

//   if (ninp==2)
//      DOUT1("Inp%u Event%d", ninp, fCfg[ninp].curr_evnt_num);

   fCfg[ninp].valid = true;

   return true;
}



bool mbs::CombinerModule::BuildEvent()
{
   mbs::EventNumType mineventid(0), maxeventid(0), triggereventid(0);

   // indicate if some of main (non-optional) input queues are mostly full
   // if such queue will be found, incomplete event may be build when it is allowed

//   DOUT0("BuildEvent method");

   int mostly_full(-1);
   bool required_missing(false);

   for (unsigned ninp=0; ninp<fCfg.size(); ninp++) {
//      DOUT0("  Port%u: pending %u capacity %u", ninp, port->InputPending(), port->InputQueueCapacity());

      if (IsOptionalInput(ninp)) continue;

      if (!IsInputConnected(ninp)) required_missing = true;

      if (InputQueueFull(ninp) && (mostly_full<0))
         mostly_full = (int) ninp;
   }

   if (required_missing && fBuildCompleteEvents) {
      // if some of important input missing than we should clean our queues
      // to let data flowing, no event will be produced to output

      for (unsigned ninp=0; ninp<fCfg.size(); ninp++) {
         if (InputQueueFull(ninp) ||
             (fCfg[ninp].no_evnt_num && (NumCanRecv(ninp) > InputQueueCapacity(ninp) / 2))) {
            ShiftToNextBuffer(ninp);
            SetInfo(dabc::format("Skip buffer on input %u while some other input is disconnected", ninp));
            DOUT0("Skip buffer on input %u",ninp);
         }
      }

      // all queues now have at least one empty entry, one could wait for the next event
      return false;
   }

   int hasTriggerEvent = -1;
   int num_valid = 0;

   double tm_now = dabc::TimeStamp::Now().AsDouble();

   for (unsigned ninp=0; ninp<fCfg.size(); ninp++) {

      fCfg[ninp].selected = false;

      if (fCfg[ninp].last_valid_tm <= 0) fCfg[ninp].last_valid_tm = tm_now;

      if (fInp[ninp].evnt()==0) {
         if (!ShiftToNextEvent(ninp)) {
            // if optional input is absent just continue
            if (fCfg[ninp].no_evnt_num) continue;
            // we can now exclude this input completely while some other is mostly full
            if ((mostly_full>=0) && !fBuildCompleteEvents) continue;

            if (fCfg[ninp].optional_input && (tm_now > (fCfg[ninp].last_valid_tm + fExcludeTime))) continue;

            return false;
         } else {
            fCfg[ninp].last_valid_tm = tm_now;
         }
      }

      if (fCfg[ninp].no_evnt_num) continue;

      mbs::EventNumType evid = fCfg[ninp].curr_evnt_num;

      if (num_valid == 0)  {
         mineventid = evid;
         maxeventid = evid;
      } else {
         if (evid < mineventid) mineventid = evid; else
            if (evid > maxeventid) maxeventid = evid;
      }

      num_valid++;

      if (fCfg[ninp].curr_evnt_special && (hasTriggerEvent<0)) {
         hasTriggerEvent = ninp;
         triggereventid = evid;
      }

   } // for ninp

   if (num_valid==0) return false;

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
         SetInfo(dabc::format("Event id difference %u exceeding tolerance window %u, stopping dabc!", diff, fEventIdTolerance), true);
         dabc::mgr.StopApplication();
         return false; // need to return immediately after stop state is set
      }

      // select inputs which will be used for building
      for (unsigned ninp = 0; ninp < fCfg.size(); ninp++)
         if (fCfg[ninp].valid && ((fCfg[ninp].curr_evnt_num == buildevid) || fCfg[ninp].no_evnt_num))
            fCfg[ninp].selected = true;
   }

   // calculated result event size and define if mbs header is available
   // also check here if all subids are unique
   uint32_t subeventssize = 0;
   // define number of input which will be used to copy mbs header
   int copyMbsHdrId = -1;
   std::map<uint32_t, bool> subid_map;
   unsigned num_selected_important(0), num_selected_all(0);

   // check of unique subevent ids:
   bool duplicatefound = false;

   // indicate if important input skipped - means input which could have important data,
   // used to check if incomplete event can be build when if fBuildCompleteEvents = true
   bool important_input_skipped = false;

   int firstselected = -1;

   std::string sel_str;

   for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
      if (!fCfg[ninp].selected) {
         // input without number can be skipped without any problem
         if (fCfg[ninp].no_evnt_num) continue;

         // if optional input not selected, but has valid data than it is not important for us
         // if no new events on the optional input for a long time, one can skip it as well
         if (fCfg[ninp].optional_input)
            if (fCfg[ninp].valid || (tm_now > (fCfg[ninp].last_valid_tm + fExcludeTime))) continue;

         important_input_skipped = true;

         continue;
      }

      sel_str += dabc::format(" %d", ninp);

      num_selected_all++;

      if (!IsOptionalInput(ninp)) {
         // take into account only events with "normal" event number
         if (firstselected<0) firstselected = ninp;
         num_selected_important++;
         if (fCfg[ninp].real_mbs && (copyMbsHdrId<0)) copyMbsHdrId = ninp;
      }

      subeventssize += fInp[ninp].evnt()->SubEventsSize();

      if (fCheckSubIds)
         while (fInp[ninp].NextSubEvent()) {
            uint32_t fullid = fInp[ninp].subevnt()->fFullId;
            if (subid_map.find(fullid) != subid_map.end()) {
               EOUT("Duplicate fullid = 0x%x", fullid);
               duplicatefound = true;
            }
            subid_map[fullid] = true;
         }
   }

   bool do_skip_data = false;



   if (fBuildCompleteEvents && important_input_skipped && (hasTriggerEvent<0)) {
      SetInfo(dabc::format("Skip incomplete event %u, found inputs %u required %u selected  %s", buildevid, num_selected_important, NumObligatoryInputs(), sel_str.c_str()));
      do_skip_data = true;
//      DOUT0("Skip incomplete event %u, found inputs %u required %u diff %u selected %s", buildevid, num_selected_important, NumObligatoryInputs(), diff, sel_str.c_str());
   } else
   if (duplicatefound && (hasTriggerEvent<0)) {
      SetInfo(dabc::format("Skip event %u while duplicates subevents found", buildevid));
      do_skip_data = true;
//    DOUT0("Skip event %u while duplicates subevents found", buildevid);
   } else {

      if (fBuildCompleteEvents && (num_selected_important < NumObligatoryInputs())) {
         SetInfo( dabc::format("Build incomplete event %u, found inputs %u required %u first %d diff %u mostly_full %d", buildevid, num_selected_important, NumObligatoryInputs(), firstselected, diff, mostly_full) );
//       DOUT0("%s skip optional input and build incomplete event %u, found inputs %u required %u first %d diff %u mostly_full %d", GetName(), buildevid, num_selected_important, NumObligatoryInputs(), firstselected, diff, mostly_full);
      } else
      if (important_input_skipped) {
         SetInfo( dabc::format("Build incomplete event %u, found inputs %u required %u first %d diff %u mostly_full %d", buildevid, num_selected_important, NumObligatoryInputs(), firstselected, diff, mostly_full) );
//       DOUT0("%s Build incomplete event %u, found inputs %u required %u first %d diff %u mostly_full %d", GetName(), buildevid, num_selected_important, NumObligatoryInputs(), firstselected, diff, mostly_full);
      } else {
// JAM2016: better supress this output:
//         SetInfo( dabc::format("Build event %u with %u inputs %s", buildevid, num_selected_all, sel_str.c_str()) );
//         DOUT0("Build event %u with %u inputs selected %s", buildevid, num_selected_all, sel_str.c_str());
      }

      // if there is no place for the event, flush current buffer
      if (fOut.IsBuffer() && !fOut.IsPlaceForEvent(subeventssize))
         if (!FlushBuffer()) return false;

      if (!fOut.IsBuffer()) {

         dabc::Buffer buf = TakeBuffer();
         if (buf.null()) return false;

         if (!fOut.Reset(buf)) {
            SetInfo("Cannot use buffer for output - hard error!!!!", true);

            buf.Release();

            dabc::mgr.StopApplication();
            return false;
         }
      }

      if (!fOut.IsPlaceForEvent(subeventssize)) {
         EOUT("Event size %u too big for buffer, skip event %u completely", subeventssize+ sizeof(mbs::EventHeader), buildevid);
      } else {

         if (copyMbsHdrId<0) {
            // SetInfo("No mbs eventid found in mbs event number mode, stop dabc", true);
            // dabc::mgr.StopApplication();
         }

         DOUT4("Building event %u num_valid %u", buildevid, num_valid);
         fOut.NewEvent(buildevid); // note: this header id may be overwritten due to mode

         for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {

            if (fCfg[ninp].selected) {

               // if header id still not defined, used first
               if (copyMbsHdrId<0) copyMbsHdrId = ninp;

               if (!fInp[ninp].IsData())
                  throw dabc::Exception("Input has no buffer but used for event building");

               dabc::Pointer ptr;
               fInp[ninp].AssignEventPointer(ptr);

               ptr.shift(sizeof(mbs::EventHeader));

               if (ptr.segmid()>100)
                  throw dabc::Exception("Bad segment id");

               fOut.AddSubevent(ptr);
            }
         }

         fOut.evnt()->CopyHeader(fInp[copyMbsHdrId].evnt());

         fOut.FinishEvent();

         DOUT4("Produced event %d subevents %u", buildevid, subeventssize);

         Par(fEventRateName).SetValue(1);
         Par(fDataRateName).SetValue((subeventssize + sizeof(mbs::EventHeader))/1024./1024.);

         // if output buffer filled already, flush it immediately
         if (!fOut.IsPlaceForEvent(0))
            FlushBuffer();
      }
   } // end of incomplete event

   for (unsigned ninp = 0; ninp < fCfg.size(); ninp++)
      if (fCfg[ninp].selected) {
         // if just skipping data, do not remove special (EPICS) inputs
         if (do_skip_data && fCfg[ninp].no_evnt_num) continue;

         if (ShiftToNextEvent(ninp))
            fCfg[ninp].last_valid_tm = tm_now;
      }

   // return true means that method can be called again immediately
   // in all places one requires while loop
   return true;
}


int mbs::CombinerModule::ExecuteCommand(dabc::Command cmd)
{



///// JAM this is old section, we replace it with new features as implemented in pexorplugin
//   if (cmd.IsName(mbs::comStartFile)) {
//      if (NumOutputs()<2) {
//         EOUT("No ports was created for the file");
//         return dabc::cmd_false;
//      }
//
//      // TODO: check if it works, probably some parameters should be taken from original command
//      bool res = dabc::mgr.CreateTransport(OutputName(1, true));
//      return cmd_bool(res);
//   } else
//   if (cmd.IsName(mbs::comStopFile)) {
//
//      FindPort(OutputName(1)).Disconnect();
//
//      SetInfo("Stop file", true);
//
//      return dabc::cmd_true;
//   } else
//   if (cmd.IsName(mbs::comStartServer)) {
//      if (NumOutputs()<1) {
//         EOUT("No ports was created for the server");
//         return dabc::cmd_false;
//      }
//
//      bool res = dabc::mgr.CreateTransport(OutputName(0, true));
//
//      return cmd_bool(res);
//   } else
//   if (cmd.IsName(mbs::comStopServer)) {
//      FindPort(OutputName()).Disconnect();
//
//      SetInfo("Stop server", true);
//      return dabc::cmd_true;
//   }
///////////////////////////////////////////////////////////////////////////////////7


////// begin new part from pexorplugin:
  if (cmd.IsName(mbs::comStartFile)) {

     std::string fname = cmd.GetStr(dabc::xmlFileName); //"filename")
     int maxsize = cmd.GetInt(dabc::xml_maxsize, 30); // maxsize
     std::string url = dabc::format("%s://%s?%s=%d", mbs::protocolLmd, fname.c_str(), dabc::xml_maxsize, maxsize);
     EnsurePorts(0, 2);
     bool res = dabc::mgr.CreateTransport(OutputName(1, true), url);
     DOUT0("Started file %s res = %d", url.c_str(), res);
     SetInfo(dabc::format("Execute StartFile for %s, result=%d",url.c_str(), res), true);
     ChangeFileState(true);
     return cmd_bool(res);
      } else
      if (cmd.IsName(mbs::comStopFile)) {
         FindPort(OutputName(1)).Disconnect();
         SetInfo("Stopped file", true);
         ChangeFileState(false);
         return dabc::cmd_true;
      } else
      if (cmd.IsName(mbs::comStartServer)) {
         if (NumOutputs()<1) {
            EOUT("No ports was created for the server");
            return dabc::cmd_false;
         }
         std::string skind = cmd.GetStr(mbs::xmlServerKind);

         int port = cmd.GetInt(mbs::xmlServerPort, 6666);
         std::string url = dabc::format("mbs://%s?%s=%d", skind.c_str(), mbs::xmlServerPort,  port);
         EnsurePorts(0, 1);
         bool res = dabc::mgr.CreateTransport(OutputName(0, true));
         DOUT0("Started server %s res = %d", url.c_str(), res);
         SetInfo(dabc::format("Execute StartServer for %s, result=%d",url.c_str(), res), true);
         return cmd_bool(res);
      } else
      if (cmd.IsName(mbs::comStopServer)) {
         FindPort(OutputName(0)).Disconnect();
         SetInfo("Stopped server", true);
         return dabc::cmd_true;
      }
////////////// end new part from pexorplugin



   else
   if (cmd.IsName("ConfigureInput")) {
      unsigned ninp = cmd.GetUInt("Port", 0);
//      DOUT0("Start input configure %u size %u", ninp, fCfg.size());
      if (ninp<fCfg.size()) {

//         DOUT0("Do0 input configure %u size %u", ninp, fCfg.size());

         fCfg[ninp].real_mbs = cmd.GetBool("RealMbs", fCfg[ninp].real_mbs);
         fCfg[ninp].real_evnt_num = cmd.GetBool("RealEvntNum", fCfg[ninp].real_evnt_num);
         fCfg[ninp].no_evnt_num = cmd.GetBool("NoEvntNum", fCfg[ninp].no_evnt_num);
         fCfg[ninp].optional_input = cmd.GetBool("Optional", fCfg[ninp].optional_input);

//         DOUT0("Do1 input configure %u size %u", ninp, fCfg.size());
         fCfg[ninp].evntsrc_fullid = cmd.GetUInt("EvntSrcFullId", fCfg[ninp].evntsrc_fullid);
         fCfg[ninp].evntsrc_shift = cmd.GetUInt("EvntSrcShift", fCfg[ninp].evntsrc_shift);

//         DOUT0("Do2 input configure %u size %u", ninp, fCfg.size());

         std::string ratename = cmd.GetStr("RateName", "");
         if (!ratename.empty())
            SetPortRatemeter(InputName(ninp), CreatePar(ratename).SetRatemeter(false,1.));

//         DOUT0("Do3 input configure %u size %u", ninp, fCfg.size());

         // optional imputs not need to be accounted for obligatory inputs
         if (fCfg[ninp].optional_input || fCfg[ninp].no_evnt_num) {
            if (fNumObligatoryInputs>1) fNumObligatoryInputs--;
         }

         // events without number could not be MBS events
         if (fCfg[ninp].no_evnt_num) {
            fCfg[ninp].real_mbs = false;
         }

         DOUT1("Configure input%u of module %s: RealMbs:%s RealEvntNum:%s EvntSrcFullId: 0x%x EvntSrcShift: %u",
               ninp, GetName(),
               DBOOL(fCfg[ninp].real_mbs), DBOOL(fCfg[ninp].real_evnt_num),
               fCfg[ninp].evntsrc_fullid, fCfg[ninp].evntsrc_shift);

//         DOUT0("Do4 input configure %u size %u", ninp, fCfg.size());

      }

      return dabc::cmd_true;
   }



   return dabc::ModuleAsync::ExecuteCommand(cmd);

}

unsigned int mbs::CombinerModule::GetOverflowEventNumber() const
{
   return 0xffffffff;
}

// JAM2016 - adopted from pexorplugin readout module
void  mbs::CombinerModule::ChangeFileState(bool on)
{
   SetParValue(fFileStateName, on);
}



