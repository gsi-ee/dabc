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

#include "dimc/Player.h"

#include "dabc/Publisher.h"

#include <dic.hxx>


dimc::Player::Player(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fDimDns(),
   fDimBr(0)
{
   //EnsurePorts(0, 0, dabc::xmlWorkPool);

   fDimDns = Cfg("DimDns", cmd).AsStr();

   fWorkerHierarchy.Create("DIMC");

   CreateTimer("update", 1., false);

   Publish(fWorkerHierarchy, "DIMC");
}

dimc::Player::~Player()
{
   delete fDimBr; fDimBr = 0;
}


void dimc::Player::OnThreadAssigned()
{
   dabc::ModuleAsync::OnThreadAssigned();

   fDimBr = new ::DimBrowser();


}

void dimc::Player::ProcessTimerEvent(unsigned timer)
{
   if (fDimBr==0) return;

   char *ptr, *ptr1;
   int type;

// DimClient::addErrorHandler(errHandler);

   int n = fDimBr->getServices("*");
   DOUT0("found %d DIM services", n);

   while((type = fDimBr->getNextService(ptr, ptr1))!= 0)
   {
      DOUT0("type %d name %s descr %s", type, ptr, ptr1);
   }

}

int dimc::Player::ExecuteCommand(dabc::Command cmd)
{
   return dabc::ModuleAsync::ExecuteCommand(cmd);
}
