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

dimc::Player::Player(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
   EnsurePorts(0, 0, dabc::xmlWorkPool);

   fWorkerHierarchy.Create("DIMC");

   CreateTimer("update", 1., false);

   Publish(fWorkerHierarchy, "DIMC");
}

dimc::Player::~Player()
{
}


void dimc::Player::OnThreadAssigned()
{
   dabc::ModuleAsync::OnThreadAssigned();
}

void dimc::Player::ProcessTimerEvent(unsigned timer)
{
}

int dimc::Player::ExecuteCommand(dabc::Command cmd)
{
   return dabc::ModuleAsync::ExecuteCommand(cmd);
}
