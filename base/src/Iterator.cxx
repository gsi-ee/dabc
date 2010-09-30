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
#include "dabc/Iterator.h"

#include "dabc/logging.h"

dabc::Iterator::Iterator(Basic* topfolder, int maxlevel) :
   fTop(topfolder),
   fCurrent(0),
   fIndexes(),
   fFolders(),
   fFullName(),
   fMaxLevel(maxlevel)
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
      
      if (goinside && (fCurrentFolder!=0) && (fCurrentFolder->NumChilds()>0) &&
           ((fMaxLevel<0) || (sz<fMaxLevel))) {
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

