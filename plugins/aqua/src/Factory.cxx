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

#include "aqua/Factory.h"

#include "aqua/ClientOutput.h"

#include "dabc/Url.h"

dabc::FactoryPlugin aquafactory(new aqua::Factory("aqua"));


dabc::DataOutput* aqua::Factory::CreateDataOutput(const std::string& typ)
{

   dabc::Url url(typ);
   if (url.GetProtocol()=="aqua") {
      DOUT1("AQUA output name %s", url.GetFullName().c_str());
      return new aqua::ClientOutput(url);
   }

   return 0;
}
