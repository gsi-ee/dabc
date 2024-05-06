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

#include "olmd/Factory.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/SocketThread.h"
#include "dabc/MemoryPool.h"
#include "dabc/Url.h"
#include "dabc/DataTransport.h"

#include "olmd/OlmdInput.h"



const char* olmd::protocolOlmd          = "olmd";

dabc::FactoryPlugin olmdfactory(new olmd::Factory("olmd"));




dabc::DataInput* olmd::Factory::CreateDataInput(const std::string& typ)
{
   DOUT2("Factory::CreateDataInput %s", typ.c_str());

   dabc::Url url(typ);
   if (url.GetProtocol()==olmd::protocolOlmd) {
      DOUT0("original LMD input file name %s", url.GetFullName().c_str());
      return new olmd::OlmdInput(url);
   }

   return 0;
}




