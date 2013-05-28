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

#ifndef DABC_Hierarchy
#define DABC_Hierarchy

#ifndef DABC_Record
#include "dabc/Record.h"
#endif

namespace dabc {

   class HierarchyContainer : public RecordContainer {
      protected:

      public:
         HierarchyContainer(const std::string& name);

         HierarchyContainer(Reference parent, const std::string& name);

         virtual const char* ClassName() const { return "Hierarchy"; }
   };

   // ______________________________________________________________

   /** \brief Represents objects hierarchy of remote (or local) DABC process
    *
    * Idea to completely replicate folder structure with many attributes,
    * which are stored into HierarchyContainer class.
    * One should be able to convert such hierarchy to/from xml file.
    * Idea to create such hierarchy is to able provide different clients to monitor and control DABc process.
    */

   class Hierarchy : public Record {

      DABC_REFERENCE(Hierarchy, Record, HierarchyContainer)

      void MakeHierarchy(Reference top);
   };


}

#endif
