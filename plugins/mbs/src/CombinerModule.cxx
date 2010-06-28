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

#include <limits.h>




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

   fPool = CreatePoolHandle(dabc::xmlWorkPool, fBufferSize, 10);

   int numinp = GetCfgInt(dabc::xmlNumInputs, 2, cmd);

   fFileOutput = GetCfgBool(mbs::xmlFileOutput, false, cmd);
   fServOutput = GetCfgBool(mbs::xmlServerOutput, false, cmd);

   fBuildCompleteEvents = GetCfgBool(mbs::xmlCombineCompleteOnly, false, cmd);
   fCheckSubIds = GetCfgBool(mbs::xmlCheckSubeventIds, false, cmd);

   fEventIdTolerance = GetCfgInt(mbs::xmlEvidTolerance, 1000, cmd);

   double flashtmout = GetCfgDouble(dabc::xmlFlashTimeout, 1., cmd);

   for (int n=0;n<numinp;n++) {
      CreateInput(FORMAT((mbs::portInputFmt, n)), fPool, 5);
      fInp.push_back(ReadIterator(0));
   }

   CreateOutput(mbs::portOutput, fPool, 5);
   if (fFileOutput) CreateOutput(portFileOutput, fPool, 5);
   if (fServOutput) CreateOutput(mbs::portServerOutput, fPool, 5);

   if (flashtmout>0.) CreateTimer("Flash", flashtmout, false);

   fEvntRate = CreateRateParameter("EventRate", false, 1., "", "", "Ev/s", 0., 20000.);
   fDataRate = CreateRateParameter("DataRate", false, 1., "", "", "MB/s", 0., 10.);

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

   CreateParInfo("CombinerInfo", 1, "Green");
}

mbs::CombinerModule::~CombinerModule()
{
   if (fOutBuf!=0) {
      fOut.Close();
      dabc::Buffer::Release(fOutBuf);
      fOutBuf = 0;
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

bool mbs::CombinerModule::BuildEvent()
{
   mbs::EventNumType mineventid = 0;
   mbs::EventNumType maxeventid = 0;
   mbs::EventNumType buildevid = 0;

   for (unsigned ninp=0; ninp<NumInputs(); ninp++) {

      if (!fInp[ninp].IsBuffer()) {

         dabc::Port* port = Input(ninp);

         while (port->CanRecv()) {
            if (fInp[ninp].Reset(port->FirstInputBuffer()))
               if (fInp[ninp].NextEvent()) break;

            fInp[ninp].Reset(0);
            port->SkipInputBuffers(1);
         }

         // if no buffer is possible to assign, break
         if (!fInp[ninp].IsBuffer()) return false;
      }

      mbs::EventNumType evid = fInp[ninp].evnt()->EventNumber();

      if (ninp==0) {
         mineventid = evid;
         maxeventid = evid;
      } else {
         if (evid < mineventid) mineventid = evid; else
         if (evid > maxeventid) maxeventid = evid;
      }
   } // for ninp

    bool overflow=false;
	if (IsBuildCompleteEvents())
	{
		// TODO: skip all event inputs with lower ids than maxeventid
		// probably return to complete this in next BuildEvent call
		buildevid = maxeventid;

		// TODO: check evid wraparound here!
		if (maxeventid != mineventid)
		{
			if (maxeventid - mineventid > UINT_MAX - GetEventIdTolerance()) // account tolerance range for evid
				{
					// check for unsigned wraparound
					buildevid = mineventid;
					overflow=true;
					DOUT1(("Build event: detected event id integer wraparound, using buildid  %u", buildevid));
				}
			else if (maxeventid - mineventid > GetEventIdTolerance())
				{
					// we exceed setup tolerance, indicate an error
					EOUT(("Build event: Event id difference %u is exceeding tolerance window %u",maxeventid - mineventid, GetEventIdTolerance() ));
					Stop();
					return false; // need to return immediately after stop state is set
				}

		}
		bool isok[NumInputs()];
		for (unsigned ninp = 0; ninp < NumInputs(); ninp++)
		{
			isok[ninp] = false;
			mbs::EventNumType lasteventid=0;
			bool droppedevents=false;
			while ((!overflow && fInp[ninp].evnt()->EventNumber() < buildevid)
						|| (overflow && fInp[ninp].evnt()->EventNumber() > buildevid))
			{
				droppedevents=true;
				lasteventid=fInp[ninp].evnt()->EventNumber();
				DOUT3(("Combiner Module: Skipping event with id %u on channel %u until reaching buildid=%u", lasteventid, ninp, buildevid));
				if (fInp[ninp].NextEvent())
				{
					// try with next event in buffer
					continue;
				}
				else
				{
					// no more event in current buffer, try next
					dabc::Port* port = Input(ninp);
					port->SkipInputBuffers(1); // discard old buffer
					if (fInp[ninp].Reset(port->FirstInputBuffer()))
					{
						if (fInp[ninp].NextEvent())
						{
							// try first event of new buffer
							continue;
						}
						else
						{
							// no event in new buffer, reset for next call
							fInp[ninp].Reset(0);
							DOUT3(("Build event: no more event in input buffer, resetting input on channel %u, buildid=%u", ninp, buildevid));
							break; // leave inner? loop
						}
					}
					else
					{
						// no more input buffers in port, reset for next call
						fInp[ninp].Reset(0);
						DOUT3(("Build event: no more input buffer, resetting input on channel %u, buildid=%u", ninp, buildevid));
						break; // leave inner loop
					}
				}
			}; // while
			if(droppedevents)
				{
					DOUT1(("Combiner Module: Input %d dropped all events before id %u to reach build event id %u", ninp, lasteventid, buildevid));
				}

			if (fInp[ninp].IsBuffer() && fInp[ninp].evnt()->EventNumber() == buildevid)
			{
				// check if we really match the buildid on this channel:
				DOUT5(("Build event: matching build event id on channel %u, buildid=%u", ninp, buildevid));
				isok[ninp] = true;
			}
			else
			{
				// this may happen if the port queue was empty before buildevid was reached,
				// or if buildevid is missing (skipped) on some input ports
				DOUT3(("Combiner Module: Input %d has not reached build event id %u yet, try next buffer...", ninp, buildevid));
				isok[ninp] = false;
			}

		} // for

		  for (unsigned ninp=0; ninp<NumInputs(); ninp++)
			  {
				  if(!isok[ninp])
					  {
						  return false; // try re-read and evaluate buildid again
					  }
			  }
	}
	else
	{
		// set minimum id found as buildid and only use this events
		buildevid = mineventid;
		// treat correctly situation of event id overflow
//		if (maxeventid != mineventid)
//			if (maxeventid - mineventid > 0x10000000)
//				buildevid = maxeventid;

		if (maxeventid != mineventid)
				{
					if (maxeventid - mineventid > UINT_MAX)
						{
							// check for unsigned wraparound
							buildevid = maxeventid;
							overflow=true;
							DOUT1(("Build event: detected event id integer wraparound, using buildid  %u", buildevid));
						}
				}


	} // if(fBuildCompleteEvents)


   uint32_t subeventssize = 0;

   for (unsigned ninp=0; ninp<NumInputs(); ninp++)
      if (fInp[ninp].evnt()->EventNumber() == buildevid)
         subeventssize += fInp[ninp].evnt()->SubEventsSize();

   if ((fOutBuf!=0) && !fOut.IsPlaceForEvent(subeventssize))
      if (!FlushBuffer()) return false;

   if (fOutBuf==0) {
      fOutBuf = fPool->TakeBufferReq(fBufferSize);
      if (fOutBuf == 0) return false;

      if (!fOut.Reset(fOutBuf)) {
         EOUT(("Cannot use buffer for output - hard error!!!!"));
         dabc::Buffer::Release(fOutBuf);
         return false;
      }
   }



   if (!fOut.IsPlaceForEvent(subeventssize))
      EOUT(("Event size %u too big for buffer %u, skip event %u", subeventssize+ sizeof(mbs::EventHeader), fBufferSize, buildevid));
   else {

      DOUT2(("Build event %u", buildevid));

      fOut.NewEvent(buildevid);
      dabc::Pointer ptr;
      bool isfirst = true;
      for (unsigned ninp=0; ninp<NumInputs(); ninp++)
         if (fInp[ninp].evnt()->EventNumber() == buildevid) {
            if (isfirst) {
               fOut.evnt()->CopyHeader(fInp[ninp].evnt());
               isfirst = false;
            }

            fInp[ninp].AssignEventPointer(ptr);
            ptr.shift(sizeof(mbs::EventHeader));
            fOut.AddSubevent(ptr); // TODO: consistency check of subevent ids -> iterator?
         }
      fOut.FinishEvent();

      fEvntRate->AccountValue(1.);
      fDataRate->AccountValue((subeventssize + sizeof(mbs::EventHeader))/1024./1024.);

      // if output buffer filled already, flush it immediately
      if (!fOut.IsPlaceForEvent(0)) FlushBuffer();
   }

   for (unsigned ninp=0; ninp<NumInputs(); ninp++)
      if (fInp[ninp].evnt()->EventNumber() == buildevid)
         if (!fInp[ninp].NextEvent()) {
            fInp[ninp].Reset(0); // forgot about buffer
            Input(ninp)->SkipInputBuffers(1);
         }

   return true;
}


int mbs::CombinerModule::ExecuteCommand(dabc::Command* cmd)
{
   if (cmd->IsName(mbs::comStartFile)) {
      dabc::Port* port = FindPort(mbs::portFileOutput);
      if (port==0) port = CreateOutput(mbs::portFileOutput, fPool, 5);

      std::string filename = port->GetCfgStr(mbs::xmlFileName,"",cmd);
      int sizelimit = port->GetCfgInt(mbs::xmlSizeLimit,1000,cmd);

      SetParStr("CombinerInfo", dabc::format("Start mbs file %s sizelimit %d mb", filename.c_str(), sizelimit));

      dabc::Command* dcmd = new dabc::CmdCreateTransport(port->GetFullName(dabc::mgr()).c_str(), mbs::typeLmdOutput, "MbsFileThrd");
      dcmd->SetStr(mbs::xmlFileName, filename);
      dcmd->SetInt(mbs::xmlSizeLimit, sizelimit);
      return dabc::mgr()->Execute(dcmd);
   } else
   if (cmd->IsName(mbs::comStopFile)) {
      dabc::Port* port = FindPort(mbs::portFileOutput);
      if (port!=0) port->Disconnect();

      SetParStr("CombinerInfo", "Stop file");

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

      SetParStr("CombinerInfo", dabc::format("%s mbs server %s port %d",
            ((res==dabc::cmd_true) ? "Start" : " Fail to start"), serverkind.c_str(), DefualtServerPort(kind)));

      return res;
   } else
   if (cmd->IsName(mbs::comStopServer)) {
      dabc::Port* port = FindPort(mbs::portServerOutput);
      if (port!=0) port->Disconnect();

      SetParStr("CombinerInfo", "Stop server");
      return dabc::cmd_true;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);

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

