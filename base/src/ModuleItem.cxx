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

#include "dabc/ModuleItem.h"

dabc::ModuleItem::ModuleItem(int typ, Reference parent, const char* name) :
   Worker(parent, name, true),
   fModule(0),
   fItemType(typ),
   fItemId(0)
{
   // we should reinitialize variable, while it will be cleared by constructor
   parent = GetParentRef();

   while ((fModule==0) && !parent.null()) {
      fModule = dynamic_cast<Module*> (parent());
      parent = parent()->GetParentRef();
   }

   SetWorkerPriority(1);

   if (fModule)
      fModule->ItemCreated(this);
}

dabc::ModuleItem::~ModuleItem()
{
   DOUT2(("ModuleItem:%p start destructor", this));

   if (fModule)
      fModule->ItemDestroyed(this);

   DOUT2(("ModuleItem:%p did destructor", this));
}

void dabc::ModuleItem::ThisItemCleaned()
{
   if (fModule)
      fModule->ItemCleaned(this);
}


