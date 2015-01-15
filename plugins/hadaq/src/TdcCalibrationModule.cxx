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

#include "hadaq/TdcCalibrationModule.h"

#include <math.h>

#include "dabc/Application.h"
#include "dabc/Manager.h"

#include "hadaq/HadaqTypeDefs.h"
#include "hadaq/Iterator.h"


hadaq::TdcCalibrationModule::TdcCalibrationModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
   // we need one input and one output port
   EnsurePorts(1, 1);
}


bool hadaq::TdcCalibrationModule::retransmit()
{
   // method reads one buffer, calibrate it and send further

   // nothing to do
   if (CanSend() && CanRecv()) {

      dabc::Buffer buf = Recv();

      Send(buf);

      return true;
   }

   return false;
}


int hadaq::TdcCalibrationModule::ExecuteCommand(dabc::Command cmd)
{
   return dabc::ModuleAsync::ExecuteCommand(cmd);
}
