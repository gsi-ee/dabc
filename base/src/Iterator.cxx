#include "dabc/Iterator.h"

#include "dabc/logging.h"

dabc::Iterator::Iterator(Basic* topfolder) : 
   fTop(topfolder),
   fCurrent(0),
   fIndexes(),
   fFolders(),
   fFullName()
{
}

dabc::Iterator::~Iterator()
{
}

dabc::Basic* dabc::Iterator::next(bool goinside)
{
   if (fCurrent==0) {
      fCurrent = fTop;    
      fIndexes.clear();
      fFolders.clear();
   } else {
   
      int sz = fIndexes.size();
      bool doagain = true;
      
      Folder* fCurrentFolder = dynamic_cast<Folder*> (fCurrent);
      
      if (goinside && (fCurrentFolder!=0) && (fCurrentFolder->NumChilds()>0)) {
         fIndexes.resize(sz+1);
         fFolders.resize(sz+1);
         fIndexes[sz] = 0;
         fFolders[sz] = fCurrentFolder;
         fCurrent = fCurrentFolder->GetChild(0);
      } else 

      while (doagain) { 
         
         if (sz==0) { fCurrent = 0; break; }
         
         dabc::Folder* parent = fFolders[sz-1];
         fIndexes[sz-1]++;

         if (fIndexes[sz-1]<parent->NumChilds()) {
            doagain = false;
            fCurrent = parent->GetChild(fIndexes[sz-1]);
            fIndexes.resize(sz);
            fFolders.resize(sz);
         } else {   
            sz--;
         }
      }
   }
   
   if (fCurrent) fCurrent->MakeFullName(fFullName);
    
   return fCurrent; 
}

void dabc::Iterator::PrintHieararchy(Basic* topfolder)
{
   if (topfolder==0) return;
   dabc::Iterator iter(topfolder);
   while (iter.next()) 
      DOUT0(("%*s %s", iter.level()*2, "", iter.name()));
}

