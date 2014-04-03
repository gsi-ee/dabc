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

#include "ezca/Factory.h"

#include <stdlib.h>

#include "dabc/Command.h"
#include "dabc/logging.h"
#include "dabc/Url.h"
#include "dabc/Manager.h"

#include "ezca/Definitions.h"
#include "ezca/EpicsInput.h"
#include "ezca/Monitor.h"

const char* ezca::xmlEpicsName               = "EpicsIdentifier";
const char* ezca::xmlUpdateFlagRecord        = "EpicsFlagRec";
const char* ezca::xmlEventIDRecord           = "EpicsEventIDRec";
const char* ezca::xmlNameLongRecords         = "EpicsLongRecs";
const char* ezca::xmlNameDoubleRecords       = "EpicsDoubleRecs";
const char* ezca::xmlTimeout                 = "EpicsPollingTimeout";
const char* ezca::xmlEpicsSubeventId         = "EpicsSubeventId";
const char* ezca::xmlEzcaTimeout             = "EzcaTimeout";
const char* ezca::xmlEzcaRetryCount          = "EzcaRetryCount";
const char* ezca::xmlEzcaDebug               = "EzcaDebug";
const char* ezca::xmlEzcaAutoError           = "EzcaAutoError";


const char* ezca::nameUpdateCommand          = "ezca::OnUpdate";
const char* ezca::xmlCommandReceiver         = "EpicsDabcCommandReceiver"; // Command receiver on flag change event

dabc::FactoryPlugin epicsfactory(new ezca::Factory("ezca"));


ezca::Factory::Factory(const std::string& name) :
   dabc::Factory(name)
{
}

dabc::DataInput* ezca::Factory::CreateDataInput(const std::string& typ)
{
   dabc::Url url(typ);
   if (url.GetProtocol()=="ezca")
      return new ezca::EpicsInput(url.GetHostName());

   return 0;
}

dabc::Module* ezca::Factory::CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd)
{
   if (classname == "ezca::Monitor")
      return new ezca::Monitor(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}
