/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#include "dabc/ModuleItem.h"

dabc::ModuleItem::ModuleItem(int typ, Basic* parent, const char* name) :
   Folder(parent, name),
   WorkingProcessor(this),
   fModule(0),
   fItemType(typ),
   fItemId(0),
   fHalted(false)
{
   SetParDflts(3);

   while ((fModule==0) && (parent!=0)) {
      fModule = dynamic_cast<Module*> (parent);
      parent = parent->GetParent();
   }

   SetProcessorPriority(1);

   if (fModule)
      fModule->ItemCreated(this);
}

dabc::ModuleItem::~ModuleItem()
{
   if (fModule)
      fModule->ItemDestroyed(this);
}
