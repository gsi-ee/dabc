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

#include "dabc/Hierarchy.h"

#include "dabc/Iterator.h"
#include "dabc/logging.h"

dabc::HierarchyContainer::HierarchyContainer(const std::string& name) :
   dabc::RecordContainer(name)
{
}

dabc::HierarchyContainer::HierarchyContainer(Reference parent, const std::string& name) :
   dabc::RecordContainer(parent, name)
{
}

// __________________________________________________-


void dabc::Hierarchy::MakeHierarchy(Reference top)
{
   Destroy();

   if (top.null()) return;

   Iterator iter(top.Ref());

   while (iter.next()) {
//      DOUT0("lvl:%u %s %s", iter.level(), iter.fullname(), iter.current()->ClassName());

      if (iter.level()==0) {
         // create top-level object
         SetObject(new HierarchyContainer(top.GetName()));
         SetOwner(true);
         SetTransient(false);
//         DOUT0("Creating top-level record %p %s", GetObject(), GetName());
         continue;
      }

      // all hierarchy must be reconstructed correctly
      new HierarchyContainer(*this, iter.fullname());
   }

   Iterator iter2(Ref());

   while (iter2.next()) {
      DOUT0("lvl:%u %s %s", iter2.level(), iter2.fullname(), iter2.current()->ClassName());
   }

}

