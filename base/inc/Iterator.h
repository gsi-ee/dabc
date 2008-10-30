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
         String fFullName;
      public:
         Iterator(Basic* topfolder);
         virtual ~Iterator();
         Basic* next(bool goinside = true);
         Basic* current() const { return fCurrent; }
         int level() const { return fCurrent==fTop ? 0 : fIndexes.size(); }
         const char* fullname() const { return fFullName.c_str(); } 
         const char* name() const { return fCurrent ? fCurrent->GetName() : "none"; }
         
         static void PrintHieararchy(Basic* topfolder);
   };
   
}

#endif
