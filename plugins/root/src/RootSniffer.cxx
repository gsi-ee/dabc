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
#include "TROOT.h"


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
   fHierarchy.Create("ROOT");

   ActivateTimeout(0);
   TH1* h1 = new TH1F("histo1","Tilte of histo1", 100, -10., 10.);
   h1->FillRandom("gaus", 10000);
}

double dabc_root::RootSniffer::ProcessTimeout(double last_diff)
{
   dabc::Hierarchy h;
   h.Create("ROOT");
   FillHieararchy(h, gROOT);

   DOUT0("ROOT %p hierarchy = \n%s", gROOT, h.SaveToXml().c_str());

   fHierarchy.Update(h);

//   dabc::LockGuard lock(fHierarchyMutex);

//   dabc::Hierarchy local = fHierarchy.FindChild("localhost");

//   if (local.null()) {
//      EOUT("Did not find localhost in hierarchy");
//   } else {
//      // TODO: make via XML file like as for remote node!!
//   }

   return 10.;
}


void dabc_root::RootSniffer::FillHieararchy(dabc::Hierarchy& h, TDirectory* dir)
{
   if (dir==0) return;

   TIter iter(dir ? dir->GetList() : 0);
   TObject* obj(0);
   while ((obj = iter())!=0) {
      DOUT0("Find ROOT object %s", obj->GetName());
      dabc::Hierarchy chld = h.CreateChild(obj->GetName());
   }
}


void dabc_root::RootSniffer::BuildHierarchy(dabc::HierarchyContainer* cont)
{
   if (!fHierarchy.null()) fHierarchy()->BuildHierarchy(cont);
}

