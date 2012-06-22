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
#include <iostream>


#include "dabc/Port.h"
#include "dabc/PoolHandle.h"
#include "dabc/Application.h"
#include "dabc/Manager.h"

#include "hadaq/HadaqTypeDefs.h"
#include "mbs/MbsTypeDefs.h"

// this define will just receive all buffers, note their size and discard them, no event building
//#define HADAQ_COMBINER_TESTRECEIVE 1

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
   fTotalRecvBytes=0;
   fTotalRecvEvents=0;
   fTotalDiscEvents=0;
   fTotalTagErrors=0;
   fTotalDataErrors=0;

   fRunNumber=hadaq::Event::CreateRunId(); // runid from configuration time
      // TODO: new runid for any new output file. probably get run number from application later


   fBufferSize = Cfg(dabc::xmlBufferSize,cmd).AsInt(16384);

   //fMasterChannel=0; // TODO: configure this from xml
   fTriggerNrTolerance = 10000;
   std::string poolname = Cfg(dabc::xmlPoolName, cmd).AsStdStr(dabc::xmlWorkPool);

   CreatePoolHandle(poolname.c_str());

   int numinp = Cfg(dabc::xmlNumInputs,cmd).AsInt(1);

//   if(fMasterChannel > numinp-1)
//      {
//         DOUT0(("HadaqCombinerModule: master channel index %d exceeds limit, reduce to %d", fMasterChannel, numinp-1));
//         fMasterChannel=numinp-1;
//     }

//   fDoOutput = Cfg(hadaq::xmlNormalOutput,cmd).AsBool(true);
   fFileOutput = Cfg(hadaq::xmlFileOutput,cmd).AsBool(false);
   fServOutput = Cfg(hadaq::xmlServerOutput,cmd).AsBool(false);

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
//     if (fDoOutput) CreateOutput(hadaq::portOutput, Pool(), 5);
     if (fFileOutput) CreateOutput(hadaq::portFileOutput, Pool(), 5);
     if (fServOutput) CreateOutput(hadaq::portServerOutput, Pool(), 5);

     if (flushtmout>0.) CreateTimer("FlushTimer", flushtmout, false);

     fEventRateName = ratesprefix+"Events";
     fEventDiscardedRateName = ratesprefix+"DiscardedEvents";
     fDataRateName = ratesprefix+"Data";
     fInfoName = ratesprefix+"Info";

     CreatePar(fDataRateName).SetRatemeter(false, 5.).SetUnits("MB");
     CreatePar(fEventRateName).SetRatemeter(false, 5.).SetUnits("Ev");
     CreatePar(fEventDiscardedRateName).SetRatemeter(false, 5.).SetUnits("Ev");
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
     SetInfo(dabc::format("HADAQ combiner module ready. Runid:%d, fileout:%d, servout:%d flush:%3.1f" ,
           fRunNumber, fFileOutput, fServOutput, flushtmout), true);
     DOUT0(("HADAQ combiner module ready. Runid:%d, fileout:%d, servout:%d flush:%3.1f" ,fRunNumber, fFileOutput, fServOutput, flushtmout));
     Par(fDataRateName).SetDebugLevel(1);
     Par(fEventRateName).SetDebugLevel(1);
     Par(fEventDiscardedRateName).SetDebugLevel(1);


}


void hadaq::CombinerModule::SetInfo(const std::string& info, bool forceinfo)
{

   Par(fInfoName).SetStr(info);
   if (forceinfo) Par(fInfoName).FireModified();
}




void hadaq::CombinerModule::ProcessTimerEvent(dabc::Timer* timer)
{
   if (fFlushFlag) FlushOutputBuffer();
   fFlushFlag = true;
}

void hadaq::CombinerModule::ProcessInputEvent(dabc::Port* port)
{

   while (BuildEvent());
}

void hadaq::CombinerModule::ProcessOutputEvent(dabc::Port* port)
{
    // events are build from queue data until we require something from framework
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



void hadaq::CombinerModule::BeforeModuleStart()
{
   DOUT2(("hadaq::CombinerModule::BeforeModuleStart name: %s is calling first build event...", GetName()));

   // FIXME: why event processing already done here ???

   //while (BuildEvent());

   DOUT0(("hadaq::CombinerModule::BeforeModuleStart name: %s is finished", GetName()));
}

void hadaq::CombinerModule::AfterModuleStop()
{

   std::cout <<"----- Combiner Module Statistics: -----"<<std::endl;
   std::cout << "Complete Events:" << fTotalRecvEvents << ", DiscardedEvents:"
            << fTotalDiscEvents << ", RecvBytes:" << fTotalRecvBytes<< ", data errors:"<< fTotalDataErrors<<", tag errors:"<< fTotalTagErrors<< std::endl;



}

///////////////////////////////////////////////////////////////
//////// OUTPUT BUFFER METHODS:

bool hadaq::CombinerModule::EnsureOutputBuffer(uint32_t payload)
{
   // check if we have enough space in current buffer
   if (fOut.IsBuffer() && !fOut.IsPlaceForEvent(payload)){
               // no, we close current buffer
               if (!FlushOutputBuffer()) return false;
   }
   // after flushing last buffer, take next one:
   if (!fOut.IsBuffer()) {
      dabc::Buffer buf = Pool()->TakeBufferReq(fBufferSize);
      if (buf.null()) return false;

      if (!fOut.Reset(buf)) {
                  SetInfo("Cannot use buffer for output - hard error!!!!", true);
                  buf.Release();
                  dabc::mgr.StopApplication();
                  return false;
               }
   }
   // now check working buffer for space:
   if (!fOut.IsPlaceForEvent(payload)) return false;

   return true;

}

bool hadaq::CombinerModule::FlushOutputBuffer()
{
   if (fOut.IsEmpty() || !fOut.IsBuffer()) return false;

   if (!CanSendToAllOutputs()) return false;

   dabc::Buffer buf = fOut.Close();

   DOUT3(("Send buffer of size = %d", buf.GetTotalSize()));

   SendToAllOutputs(buf.HandOver());

   fFlushFlag = false; // indicate that next flush timeout one not need to send buffer

   return true;
}

///////////////////////////////////////////////////////////////
//////// INPUT BUFFER METHODS:


bool hadaq::CombinerModule::ShiftToNextBuffer(unsigned ninp)
{
   DOUT5(("CombinerModule::ShiftToNextBuffer %d ", ninp));
   fInp[ninp].Close();
   return (Input(ninp)->SkipInputBuffers(1));
}

bool hadaq::CombinerModule::ShiftToNextHadTu(unsigned ninp)
{
   DOUT5(("CombinerModule::ShiftToNextHadTu %d begins", ninp));
   //std::cout <<"ShiftToNextHadTu n="<<ninp << std::endl;
   bool foundhadtu(false);
    while (!foundhadtu) {

        if (!fInp[ninp].IsData()) {
           if (Input(ninp)->InputPending()==0) return false;

           if (!fInp[ninp].Reset(Input(ninp)->FirstInputBuffer())) {
              DOUT5(("CombinerModule::ShiftToNextHadTu %d could not reset FirstInputBuffer", ninp));
              // skip buffer and try again
              ShiftToNextBuffer(ninp);

              continue;
            }
        }

        bool res = fInp[ninp].NextHadTu();
        if (!res || (fInp[ninp].hadtu()==0)) {
           DOUT5(("CombinerModule::ShiftToNextHadTu %d has zero NextHadTu()", ninp));
           ShiftToNextBuffer(ninp);
           continue;
        }
        foundhadtu=true;
     } //  while (!foundhadtu)
return true;

}



bool hadaq::CombinerModule::ShiftToNextSubEvent(unsigned ninp)
{
   DOUT5(("CombinerModule::ShiftToNextSubEvent %d ", ninp));
   fCfg[ninp].Reset();
   bool foundevent(false);
//
   while (!foundevent) {
      bool res = fInp[ninp].NextSubEvent();
      if (!res || (fInp[ninp].subevnt() == 0)) {
         DOUT5(("CombinerModule::ShiftToNextSubEvent %d with zero NextSubEvent()", ninp));
         // retry in next hadtu container
          if (!ShiftToNextHadTu(ninp))
             return false; // no more pending input buffers
         continue;
      }

      foundevent = true;

      fCfg[ninp].fTrigNr = fInp[ninp].subevnt()->GetTrigNr() >> 8;
      fCfg[ninp].fTrigTag = fInp[ninp].subevnt()->GetTrigNr() & 0xFF;
      if (fInp[ninp].subevnt()->GetSize() > sizeof(hadaq::Subevent)) {
         fCfg[ninp].fEmpty = false;
      }
      fCfg[ninp].fDataError = fInp[ninp].subevnt()->GetDataError();

}

   return true;
}


bool hadaq::CombinerModule::DropAllInputBuffers()
{
   DOUT0(("hadaq::CombinerModule::DropAllInputBuffers()..."));
   for (unsigned ninp=0; ninp<fCfg.size(); ninp++) {
      while(ShiftToNextBuffer(ninp)); // until no more buffer in input port
   }
   return true;
}



bool hadaq::CombinerModule::BuildEvent()
{
   // RETURN VALUE: true - event is successfully build, recall immediately
   //               false - leave event loop for framework (other modules input is required!)


   // eventbuilding on hadtu streams here:

   // this is daq_evtbuild logic:
   // first check eventnumber of master channel
   // here loop over all channels: skip subevts with too old eventnumbers
   // if event is not complete, discard this and try next master channel index

#ifdef HADAQ_COMBINER_TESTRECEIVE
   // Testing: just discard all input buffers immediately:
   for (unsigned ninp=0; ninp<fCfg.size(); ninp++) {
      if (!Input(ninp)->CanRecv()) continue;
      dabc::Buffer buf =Input(ninp)->FirstInputBuffer();
      size_t bufferlen=0;
      if(!buf.null()) bufferlen=buf.GetTotalSize();
      fTotalRecvBytes+=bufferlen;
      Par(fDataRateName).SetDouble(bufferlen/1024./1024.);
      ShiftToNextBuffer(ninp);
   }
   return false; // always leave process event function immediately
#endif



   ///////////////////////////////////////////////////////////
   // alternative approach like simplified mbs event building:
   //////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////////////
   // first input loop: find out maximum trignum of all inputs = current event trignumber
   int num_valid = 0;
   unsigned masterchannel=0;
   uint32_t subeventssize = 0;
   hadaq::EventNumType mineventid(0), maxeventid(0);;
   for (unsigned ninp=0; ninp<fCfg.size(); ninp++) {
            if (fInp[ninp].subevnt()==0)
               if (!ShiftToNextSubEvent(ninp)) {
                  return false; // could not get subevent data on any channel. let framework do something before next try
               }


   hadaq::EventNumType evid = fCfg[ninp].fTrigNr;

        if (num_valid == 0) {
         mineventid = evid;
         maxeventid = evid;
         masterchannel = ninp;
      } else {
         if (evid < mineventid) {
            mineventid = evid;
         } else if (evid > maxeventid) {
            maxeventid = evid;
            masterchannel = ninp;
         }
      }


        num_valid++;
   } // for ninp

   if(fCfg[masterchannel].fTrigNr != maxeventid)
   {
      EOUT(("Combiner Module NEVER COME HERE: buildevid %u inconsistent with id %u at master channel %u, "
            , maxeventid, fCfg[masterchannel].fTrigNr, masterchannel));
      dabc::mgr.StopApplication();
      return false; // to leave event processing loops
   }

   //if (num_valid==0) return false;
    // we always build event with maximum trigger id = newest event, discard incomplete older events
    hadaq::EventNumType buildevid= maxeventid;
    hadaq::EventNumType buildtag=fCfg[masterchannel].fTrigTag;
    hadaq::EventNumType diff = maxeventid - mineventid;

    ///////////////////////////////////////////////////////////////////////////////
    // check too large triggertag difference on input channels, flush input buffers
    if ((fTriggerNrTolerance > 0) && (diff > fTriggerNrTolerance)) {
           SetInfo(dabc::format("Event id difference %u exceeding tolerance window %u, flushing buffers!", diff, fTriggerNrTolerance), true);
           DropAllInputBuffers();
           return false; // retry on next set of buffers
        }

    ////////////////////////////////////////////////////////////////////////
    // second input loop: skip all subevents until we reach current trignum
    // select inputs which will be used for building
    //bool eventIsBroken=false;
    bool dataError=false;
    bool tagError=false;
    for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
      bool foundsubevent = false;
      while (!foundsubevent) {

         hadaq::EventNumType trignr = fCfg[ninp].fTrigNr;
         hadaq::EventNumType trigtag = fCfg[ninp].fTrigTag;
         bool isempty = fCfg[ninp].fEmpty;
         bool haserror = fCfg[ninp].fDataError;
         if (trignr == buildevid) {

            if (!isempty) {
               // check also trigtag:
               if (trigtag != buildtag) {
                  tagError = true;
               }
               if (haserror) {
                  dataError = true;
               }
               subeventssize += fInp[ninp].subevnt()->GetPaddedSize();
            }
            foundsubevent = true;
            break;


         } else if (trignr < buildevid) {

            ShiftToNextSubEvent(ninp); // try with next subevt until reaching buildevid
            // TODO: account dropped subevents to statistics
            continue;
         } else {
            // can happen when the subevent of the buildevid is missing on this channel
            // we account broken event and let call BuildEvent again, rescan buildevid without shifting other subevts.
            Par(fEventDiscardedRateName).SetInt(1);
            fTotalDiscEvents++;
            //eventIsBroken = true;
            return false; // we give the framework some time to do other things though
         }

      } // while foundsubevent
   } // for ninpt


      // here all inputs should be aligned to buildevid
    // ensure that we have output buffer that is big enough:
          if (EnsureOutputBuffer(subeventssize))
         {
             // EVENT BUILDING IS HERE
         fOut.NewEvent(fTotalRecvEvents++,fRunNumber); // like in hadaq, event sequence number is independent of trigger.
         // TODO: we may put sync id from subevent payload to event sequence number already here.
         fOut.evnt()->SetDataError((dataError || tagError));
         if(dataError)
            fTotalDataErrors++;
         if(tagError)
            fTotalTagErrors++;

         // third input loop: build output event from all not empty subevents
         for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
            if (fCfg[ninp].fEmpty) continue;
            fOut.AddSubevent(fInp[ninp].subevnt());
         } // for ninp
          fOut.FinishEvent();

          Par(fEventRateName).SetInt(1);
          unsigned currentbytes = subeventssize + sizeof(hadaq::Event);
          fTotalRecvBytes+=currentbytes;
          Par(fDataRateName).SetDouble(currentbytes/1024./1024.);

         }
          else
          {
             EOUT(("Event size %u too big for buffer %u, skip event %u completely", subeventssize+ sizeof(hadaq::Event), fBufferSize, buildevid));
          } // ensure outputbuffer

         // FINAL loop: proceed to next subevents
          for (unsigned ninp = 0; ninp < fCfg.size(); ninp++)
                ShiftToNextSubEvent(ninp);


   // return true means that method can be called again immediately
   // in all places one requires while loop
   return true; // event is build successfully. try next one
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

   //dabc::mgr.CreateMemoryPool(dabc::xmlWorkPool);

   dabc::mgr.CreateMemoryPool("Pool");

   hadaq::CombinerModule* m = new hadaq::CombinerModule("Combiner");
   dabc::mgr()->MakeThreadFor(m,"EvtBuilderThrd");

   for (unsigned n=0;n<m->NumInputs();n++){
      if (!dabc::mgr.CreateTransport(FORMAT(("Combiner/%s%u", hadaq::portInput, n)), hadaq::typeUdpInput, "UdpThrd")) {
         EOUT(("Cannot create Netmem client transport %d",n));
         exit(131);
      }
   }
   if (m->IsServOutput()) {
         DOUT0(("Create Mbs transmitter module for server"));
         dabc::mgr.CreateModule("hadaq::MbsTransmitterModule", "OnlineServer", "OnlineThrd");
         dabc::mgr.Connect(FORMAT(("OnlineServer/%s", hadaq::portInput)), FORMAT(("Combiner/%s", hadaq::portServerOutput)));
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











