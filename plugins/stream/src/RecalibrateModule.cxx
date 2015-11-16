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

#include "stream/RecalibrateModule.h"

#include "hadaq/Iterator.h"
#include "mbs/Iterator.h"
#include "hadaq/HldProcessor.h"
#include "stream/DabcProcMgr.h"

#include "dabc/Manager.h"
#include "dabc/Factory.h"
#include "dabc/Iterator.h"
#include "dabc/Buffer.h"
#include "dabc/Publisher.h"

#include <stdlib.h>

#include "base/Buffer.h"
#include "base/StreamProc.h"

// ==================================================================================

stream::RecalibrateModule::RecalibrateModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fNumSub(0),
   fReplace(false),
   fProcMgr(0),
   fHLD(0)
{
   fNumSub = Cfg("NumSub",cmd).AsInt(1);
   fReplace = Cfg("Replace",cmd).AsBool(true);

   EnsurePorts(1, 1, fReplace ? "" : dabc::xmlWorkPool);

   fWorkerHierarchy.Create("Worker");
   fProcMgr = new DabcProcMgr;
   fProcMgr->SetTop(fWorkerHierarchy, true);

   fHLD = new hadaq::HldProcessor();

   CreatePar("DataRate").SetRatemeter(false, 3.).SetUnits("MB");

   for (int n=0;n<fNumSub;n++) {
      std::string mname = dabc::format("Sub%d",n);

      dabc::CmdCreateModule cmd("stream::TdcCalibrationModule", mname);
      cmd.SetPtr("ProcMgr", fProcMgr);
      cmd.SetPtr("HLDProc", fHLD);
      dabc::mgr.Execute(cmd);
   }

   fProcMgr->UserPreLoop();

   Publish(fWorkerHierarchy, dabc::format("$CONTEXT$/%s", GetName()));

   base::ProcMgr::ClearInstancePointer();
}

stream::RecalibrateModule::~RecalibrateModule()
{
   // do not delete proc manager
}

void stream::RecalibrateModule::OnThreadAssigned()
{
   dabc::ModuleAsync::OnThreadAssigned();
}

bool stream::RecalibrateModule::retransmit()
{
   if (CanSendToAllOutputs() && CanRecv()) {

      if (!fReplace && !CanTakeBuffer()) return false;

      dabc::Buffer buf = Recv();
      Par("DataRate").SetValue(buf.GetTotalSize()/1024./1024.);

      if (buf.GetTypeId() == hadaq::mbt_HadaqEvents) {

         if (fReplace) {
            // this is easier to handle, but hit messages are replaced
            hadaq::ReadIterator iter(buf);
            while (iter.NextEvent())
               fHLD->TransformEvent(iter.evnt(), iter.evntsize());
         } else {

            dabc::Buffer resbuf = TakeBuffer();

            hadaq::ReadIterator iter(buf);
            dabc::Pointer tgt(resbuf);

            //DOUT0("Buffer size %u Original size %u", buf.GetTotalSize(), tgt.distance_to(resbuf));

            while (iter.NextEvent()) {
               unsigned len = fHLD->TransformEvent(iter.evnt(), iter.evntsize(), tgt(), tgt.rawsize());
               if (len==0) { EOUT("Fail to transform HLD event"); break; }
               if (tgt.shift(len)!=len) { EOUT("no enough space to shift to next event"); exit(5); break; }
               //DOUT0("New event size %u diff %d distance %d", len, len - iter.evnt()->GetPaddedSize(), tgt.distance_to(resbuf));
            }

            //DOUT0("Buffer size %u Result size %d", buf.GetTotalSize(), tgt.distance_to_ownbuf());

            resbuf.SetTotalSize(tgt.distance_to_ownbuf());
            resbuf.SetTypeId(hadaq::mbt_HadaqEvents);
            buf = resbuf;
         }

      } else {
         DOUT0("Buffer of unsupported type %d", buf.GetTypeId());
      }

      SendToAllOutputs(buf);

      return true;
   }

   return false;
}

int stream::RecalibrateModule::ExecuteCommand(dabc::Command cmd)
{
   if (fProcMgr && fProcMgr->ExecuteHCommand(cmd)) return dabc::cmd_true;

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

void stream::RecalibrateModule::BeforeModuleStart()
{
}

void stream::RecalibrateModule::AfterModuleStop()
{
}
