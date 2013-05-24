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

#ifndef DABC_Iterator
#define DABC_Iterator

#ifndef DABC_Object
#include "dabc/Object.h"
#endif

#ifndef DABC_Reference
#include "dabc/Reference.h"
#endif

#ifndef DABC_ReferencesVector
#include "dabc/ReferencesVector.h"
#endif

#include <vector>

namespace dabc {

   /** \brief %Iterator over objects hierarchy
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    */

   class Iterator {
      protected:
         Reference              fTop;
         Reference              fCurrent;
         std::vector<unsigned>  fIndexes;
         ReferencesVector       fFolders;
         std::string            fFullName;
         int fMaxLevel; /** Limit how deep iterator allowed to go inside hierarchy */
      public:
         Iterator(Reference ref, int maxlevel = -1);
         Iterator(Object* topfolder, int maxlevel = -1);
         virtual ~Iterator();
         Object* next(bool goinside = true);
         Object* current() const { return fCurrent(); }
         Reference ref() const { return fCurrent.Ref(); }
         int level() const { return fIndexes.size(); }
         const char* fullname() const { return fFullName.c_str(); } 
         const char* name() const { return fCurrent() ? fCurrent()->GetName() : "none"; }
         
         template<class T>
         bool next_cast(T* &ptr, bool goinside = true)
         {
            while (next(goinside)) {
               ptr = dynamic_cast<T*>(current());
               if (ptr!=0) return true;
            }
            return false;
         }

         static void PrintHieararchy(Reference ref);
   };
   
}

#endif
