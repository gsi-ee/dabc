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

#include "dabc/ReferencesVector.h"

#include <string.h>

#include "dabc/logging.h"

#include "dabc/Object.h"

dabc::ReferencesVector::ReferencesVector() throw() :
   fVector()
{
}

dabc::ReferencesVector::~ReferencesVector() throw()
{
   Clear();
}

bool dabc::ReferencesVector::Add(Reference ref) throw()
{
   if (ref.GetObject()==0) return false;

   fVector.push_back(ref);

   return true;
}


bool dabc::ReferencesVector::AddAt(Reference ref, unsigned pos) throw()
{
   if (ref.GetObject()==0) return false;

   if (pos >= fVector.size())
      fVector.push_back(ref);
   else
      fVector.insert(fVector.begin() + pos, ref);

   return true;
}


bool dabc::ReferencesVector::Remove(Object* obj) throw()
{
   if ((obj==0) || (GetSize()==0)) return false;

   unsigned n = GetSize();
   while (n-->0) {
      if (fVector[n].GetObject()==obj) {
         fVector[n] = 0;
         fVector.erase(fVector.begin()+n);
      }
   }

   return true;
}

bool dabc::ReferencesVector::DestroyAll()
{
   for (unsigned n=0; n < GetSize(); n++)
      fVector[n].Destroy();

   fVector.clear();

   return true;
}

void dabc::ReferencesVector::RemoveAt(unsigned n) throw()
{
   if (n<GetSize()) fVector.erase(fVector.begin()+n);
}

dabc::Reference dabc::ReferencesVector::TakeRef(unsigned n)
{
   if (n>=GetSize()) return dabc::Reference();

   // by this action reference will be removed from the vector

//   fVector[n].SetTransient(true);
   dabc::Reference ref(fVector[n]);

   fVector.erase(fVector.begin()+n);

   return ref;
}

bool dabc::ReferencesVector::Clear() throw()
{
   for (unsigned n=0;n<GetSize();n++)
      fVector[n].Release();

   fVector.clear();

   return true;
}

dabc::Object* dabc::ReferencesVector::FindObject(const char* name, int len) const
{
   for (unsigned n=0; n<fVector.size(); n++) {
      dabc::Object* obj = GetObject(n);
      if (obj && obj->IsName(name, len)) return obj;
   }
   return 0;
}

bool dabc::ReferencesVector::HasObject(Object* ptr)
{
   if (ptr==0) return false;
   for (unsigned n=0; n<fVector.size(); n++)
      if (GetObject(n) == ptr) return true;

   return false;
}
