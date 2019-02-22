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
#include "user/Factory.h"
#include "user/Input.h"
#include "user/Transport.h"
//#include "user/Device.h"
//#include "user/Player.h"

#include "dabc/Command.h"
#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Url.h"

dabc::FactoryPlugin userfactory (new user::Factory ("user"));

void user::Factory::Initialize()
{


}

dabc::DataInput* user::Factory::CreateDataInput(const std::string& typ)
{
   dabc::Url url(typ);
   DOUT1("user::Factory::CreateDataInput is called with type:%s, url %s\n", typ.c_str(), url.GetFullName().c_str());
   if ((url.GetProtocol()!="user") || url.GetFullName().empty())
          return dabc::Factory::CreateDataInput(typ);
   user::Input* dinput = new user::Input (url);
   return dinput;
}



