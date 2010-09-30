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
#ifndef DABC_Iterator
#define DABC_Iterator

#ifndef DABC_Folder
#include "dabc/Folder.h"
#endif

#include <vector>

namespace dabc {
    
   class Iterator {
      protected:
         Basic* fTop;  
         Basic* fCurrent;  
         std::vector<unsigned> fIndexes;
         std::vector<dabc::Folder*> fFolders;
         std::string fFullName;
         int fMaxLevel; /** Limit how deep iterator allowed to go inside hierarchy */
      public:
         Iterator(Basic* topfolder, int maxlevel = -1);
         virtual ~Iterator();
         Basic* next(bool goinside = true);
         Basic* current() const { return fCurrent; }
         int level() const { return fCurrent==fTop ? 0 : fIndexes.size(); }
         const char* fullname() const { return fFullName.c_str(); } 
         const char* name() const { return fCurrent ? fCurrent->GetName() : "none"; }
         
         template<class T>
         bool next_cast(T* &ptr, bool goinside = true)
         {
            while (next(goinside)) {
               ptr = dynamic_cast<T*>(current());
               if (ptr!=0) return true;
            }
            return false;
         }

         static void PrintHieararchy(Basic* topfolder);
   };
   
}

#endif
