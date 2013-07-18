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

#include "fesa/Player.h"

#include "fesa/defines.h"

#include <stdlib.h>

fesa::Player::Player(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{

   fHierarchy.Create("FESA");

   // this is just emulation, later one need list of real variables
   fHierarchy.CreateChild("BeamProfile").Field(dabc::prop_kind).SetStr("FESA.2D");

   CreateTimer("update", 1., false);

   fCounter = 0;
}

fesa::Player::~Player()
{
}

void fesa::Player::ProcessTimerEvent(unsigned timer)
{
//   DOUT0("Process timer");

   fCounter++;

   fesa::BeamProfile* rec = (fesa::BeamProfile*) malloc(sizeof(fesa::BeamProfile));
   rec->fill(fCounter % 7);

   dabc::BinData bindata;

   bindata = new dabc::BinDataContainer(rec, sizeof(fesa::BeamProfile), true, fCounter);

   dabc::Hierarchy item = fHierarchy.FindChild("BeamProfile");

   item()->bindata() = bindata;
   item.Field(dabc::prop_content_hash).SetInt(fCounter);
}



int fesa::Player::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("GetBinary")) {

      DOUT0("Process getbinary");

      std::string itemname = cmd.GetStdStr("Item");

      dabc::Hierarchy item = fHierarchy.FindChild(itemname.c_str());

      if (item.null()) {
         EOUT("No find item %s to get binary", itemname.c_str());
         return dabc::cmd_false;
      }

      dabc::BinData bindata = item()->bindata();

      if (bindata.null()) {
         EOUT("No find binary data for item %s", itemname.c_str());
         return dabc::cmd_false;
      }

      cmd.SetRef("#BinData", bindata);

      return dabc::cmd_true;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}


void fesa::Player::BuildWorkerHierarchy(dabc::HierarchyContainer* cont)
{
   // indicate that we are bin producer of down objects

   // do it here, while all properties of main node are ignored when hierarchy is build
   dabc::Hierarchy(cont).Field(dabc::prop_binary_producer).SetStr(ItemName());

   fHierarchy()->BuildHierarchy(cont);
}

