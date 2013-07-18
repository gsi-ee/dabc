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

#include "http/Factory.h"

#include "http/Server.h"

dabc::FactoryPlugin httpfactory(new http::Factory("http"));


void http::Factory::Initialize()
{
   http::Server* serv = new http::Server("/http");
   if (!serv->IsEnabled()) {
      dabc::WorkerRef ref = serv;
      ref.SetOwner(true);
   } else {
      DOUT0("Initialize HTTP server");
      dabc::WorkerRef(serv).MakeThreadForWorker("HttpThread");
   }
}

dabc::Reference http::Factory::CreateObject(const std::string& classname, const std::string& objname, dabc::Command cmd)
{
   if (classname == "http::Server")
      return new http::Server(objname, cmd);

   return dabc::Factory::CreateObject(classname, objname, cmd);
}
