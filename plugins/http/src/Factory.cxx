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

#include "dabc/Manager.h"
#include "dabc/Configuration.h"
#include "http/Server.h"
#include "http/Mongoose.h"
#include "http/FastCgi.h"

dabc::FactoryPlugin httpfactory(new http::Factory("http"));


void http::Factory::Initialize()
{
   if (dabc::mgr.null()) return;

   // if dabc started without config file, do not automatically start http server
   if ((dabc::mgr()->cfg()==0) || (dabc::mgr()->cfg()->GetVersion()<=0)) return;

   http::Server* serv = new http::Mongoose("/http");
   if (!serv->IsEnabled()) {
      dabc::WorkerRef ref = serv;
      ref.SetAutoDestroy(true);
   } else {
      DOUT0("Initialize HTTP server");
      dabc::WorkerRef(serv).MakeThreadForWorker("HttpThread");

      dabc::mgr.CreatePublisher();
   }
}

dabc::Reference http::Factory::CreateObject(const std::string& classname, const std::string& objname, dabc::Command cmd)
{
   if (classname == "http::Server")
      return new http::Server(objname, cmd);

   if (classname == "http::Mongoose")
      return new http::Mongoose(objname, cmd);

   if (classname == "http::FastCgi")
      return new http::FastCgi(objname, cmd);

   return dabc::Factory::CreateObject(classname, objname, cmd);
}
