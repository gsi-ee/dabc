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

#include "dabc_root/RootSniffer.h"

#include "TH1.h"
#include "TFile.h"
#include "TList.h"
#include "TMemFile.h"
#include "TStreamerInfo.h"
#include "TBufferFile.h"


dabc_root::RootSniffer::RootSniffer(const std::string& name) :
   dabc::Worker(MakePair(name)),
   fEnabled(false),
   fHierarchy(),
   fHierarchyMutex()
{
   fEnabled = Cfg("enabled").AsBool(true);
   if (!fEnabled) return;
}

dabc_root::RootSniffer::~RootSniffer()
{
}


void dabc_root::RootSniffer::OnThreadAssigned()
{
   if (!IsEnabled()) {
      EOUT("sniffer was not enabled - why it is started??");
      return;
   }
   ActivateTimeout(0);
}

double dabc_root::RootSniffer::ProcessTimeout(double last_diff)
{
   dabc::LockGuard lock(fHierarchyMutex);

//   dabc::Hierarchy local = fHierarchy.FindChild("localhost");

//   if (local.null()) {
//      EOUT("Did not find localhost in hierarchy");
//   } else {
//      // TODO: make via XML file like as for remote node!!
//   }

   return 1.;
}
