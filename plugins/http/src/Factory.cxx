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
   DOUT0("Initialize HTTP server");

   http::Server* serv = new http::Server("/http");
   if (!serv->IsEnabled()) {
      delete serv;
   } else {
      dabc::WorkerRef(serv).MakeThreadForWorker("HttpThread");
   }
}

