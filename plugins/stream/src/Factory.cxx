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

#include "stream/Factory.h"

#include "dabc/Url.h"
#include "dabc/Port.h"
#include "dabc/Manager.h"

#include "stream/RunModule.h"
#include "stream/TdcCalibrationModule.h"
#include "stream/RecalibrateModule.h"

dabc::FactoryPlugin streamfactory(new stream::Factory("stream"));

dabc::Module* stream::Factory::CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd)
{
   if (classname == "stream::RunModule")
      return new stream::RunModule(modulename, cmd);

   if (classname == "stream::TdcCalibrationModule")
      return new stream::TdcCalibrationModule(modulename, cmd);

   if (classname == "stream::RecalibrateModule")
      return new stream::RecalibrateModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}


dabc::Module* stream::Factory::CreateTransport(const dabc::Reference& port, const std::string& typ, dabc::Command cmd)
{
   dabc::Url url(typ);

   dabc::PortRef portref = port;

   if (!portref.IsOutput() || (url.GetProtocol()!="stream"))
      return dabc::Factory::CreateTransport(port, typ, cmd);

   cmd.SetBool("use_autotdc", true);
   cmd.SetStr("fileurl", typ);

   dabc::mgr.app().AddObject("module", "StreamMonitor");

   return new stream::RunModule("StreamMonitor", cmd);
}
