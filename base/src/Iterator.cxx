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

#include "dabc/Iterator.h"

#include "dabc/logging.h"

dabc::Iterator::Iterator(Reference topfolder, int maxlevel) :
   fTop(topfolder),
   fCurrent(),
   fIndexes(),
   fFolders(),
   fFullName(),
   fMaxLevel(maxlevel)
{
}

dabc::Iterator::Iterator(Object* topfolder, int maxlevel) :
   fTop(topfolder),
   fCurrent(),
   fIndexes(),
   fFolders(),
   fFullName(),
   fMaxLevel(maxlevel)
{
}

dabc::Iterator::~Iterator()
{
}

dabc::Object* dabc::Iterator::next(bool goinside)
{
   if (fCurrent()==0) {
      fCurrent = fTop;    
      fIndexes.clear();
      fFolders.Clear();
   } else {
   
      int sz = fIndexes.size();
      bool doagain = true;
      
      Reference child;

      if (goinside && (fCurrent()!=0) && ((fMaxLevel<0) || (sz<fMaxLevel)))
         child = fCurrent()->GetChildRef(0);

      if (child()!=0) {
         fIndexes.resize(sz+1);
         fIndexes[sz] = 0;
         fFolders.Add(fCurrent);
         fCurrent = child;
      } else 

      while (doagain) { 
         
         if (sz==0) { fCurrent.Release(); break; }
         
         fIndexes[sz-1]++;

         Reference child = fFolders.GetObject(sz-1)->GetChildRef(fIndexes[sz-1]);

         if (child()!=0) {
            doagain = false;
            fCurrent = child;
         } else {
            sz--;
            fFolders.RemoveAt(sz);
            fIndexes.resize(sz);
         }
      }
   }
   
   if (fCurrent()) fFullName = fCurrent()->ItemName(false);
    
   return fCurrent();
}

void dabc::Iterator::PrintHieararchy(Reference ref)
{
   dabc::Iterator iter(ref);
   while (iter.next()) 
      DOUT0("%*s %s", iter.level()*2, "", iter.name());
}

