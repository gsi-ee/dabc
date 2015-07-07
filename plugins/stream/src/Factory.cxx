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

#include "stream/RunModule.h"
#include "stream/TdcCalibrationModule.h"

dabc::FactoryPlugin streamfactory(new stream::Factory("stream"));

dabc::Module* stream::Factory::CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd)
{
   if (classname == "stream::RunModule")
      return new stream::RunModule(modulename, cmd);

   if (classname == "stream::TdcCalibrationModule")
      return new stream::TdcCalibrationModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}
