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
#include "hadaq/TdcProcessor.h"
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
   fProcMgr(0)
{
   EnsurePorts(1, 1);

   fNumSub = Cfg("NumSub",cmd).AsInt(1);

   fWorkerHierarchy.Create("Worker");
   fProcMgr = new DabcProcMgr;
   fProcMgr->SetTop(fWorkerHierarchy, true);

   for (int n=0;n<fNumSub;n++) {
      std::string mname = dabc::format("Sub%d",n);

      dabc::CmdCreateModule cmd("stream::TdcCalibrationModule", mname);
      cmd.SetPtr("ProcMgr", fProcMgr);

      DOUT0("Create module %s", mname.c_str());

      dabc::mgr.Execute(cmd);

      DOUT0("Create module %s done", mname.c_str());
   }

   base::ProcMgr::ClearInstancePointer();
}

stream::RecalibrateModule::~RecalibrateModule()
{
   // do not delete proc manager
}

void stream::RecalibrateModule::OnThreadAssigned()
{
   DOUT0("Start RecalibrateModule in %s", ThreadName().c_str());
}

bool stream::RecalibrateModule::retransmit()
{
   if (CanSend() && CanRecv()) {
      dabc::Buffer buf = Recv();
      Send(buf);
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
