
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

#include "saftdabc/Factory.h"

#include <stdlib.h>

#include "dabc/Command.h"
#include "dabc/logging.h"
#include "dabc/Url.h"
#include "dabc/Manager.h"

#include "saftdabc/Definitions.h"
#include "saftdabc/Input.h"
//#include "saftdabc/Monitor.h"

/** name of the saft device, e.g. baseboard*/
const char* saftdabc::xmlDeviceName               = "SaftBoard";

/** EXPLODER input name items that should be latched with timestamp*/
const char* saftdabc::xmlInputs                = "SaftHardwareInputNames";

/** WR event masks to snoop*/
const char* saftdabc::xmlMasks           = "SaftSnoopEventMasks";


/** WR event ids to snoop*/
const char* saftdabc::xmlEventIds         = "SaftSnoopEventIds";

const char* saftdabc::xmlOffsets           = "SaftSnoopOffsets";


/** WR event accept flags to snoop*/
const char* saftdabc::xmlAcceptFlags        = "SaftSnoopFlags";



const char* saftdabc::xmlSaftSubeventId         = "SaftMBSSubeventId";


const char* saftdabc::xmlCallbackMode = "SaftTransportCallback";

const char* saftdabc::xmlTimeout = "SaftTimeout";

const char* saftdabc::commandRunMainloop = "RunGlibMainloop";


dabc::FactoryPlugin saftfactory(new saftdabc::Factory("saftdabc"));


saftdabc::Factory::Factory(const std::string& name) :
   dabc::Factory(name)
{
}


dabc::Device* saftdabc::Factory::CreateDevice (const std::string& classname, const std::string& devname,
    dabc::Command cmd)
{
  DOUT0 ("saftdabc::Factory::CreateDevice called for class:%s, device:%s", classname.c_str (), devname.c_str ());

  if (strcmp (classname.c_str (), "saftdabc::Device") != 0)
    return 0;
  dabc::Device* dev = new saftdabc::Device (devname, cmd);
  return dev;
}


