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

#include <cstring>

#include "dabc/Object.h"

dabc::ReferencesVector::ReferencesVector() throw() :
   fVector(new refs_vector())
{
   if (fVector->capacity() < 8) fVector->reserve(8);
}

dabc::ReferencesVector::~ReferencesVector() throw()
{
   Clear();
   delete fVector;
   fVector = nullptr;
}

void dabc::ReferencesVector::ExpandVector()
{
   if ((fVector->size() == 0) || (fVector->size() < fVector->capacity())) return;

   unsigned new_capacity = fVector->capacity()*2;

   refs_vector* vect = new refs_vector();
   vect->reserve(new_capacity);

   // make shift, which does not require any locking
   for (unsigned n=0;n<fVector->size();n++) {
      vect->push_back(dabc::Reference());
      vect->back() << fVector->at(n);
   }

   fVector->clear();
   delete fVector;
   fVector = vect;
}


bool dabc::ReferencesVector::Add(Reference& ref) throw()
{
   if (!ref.GetObject()) return false;

   ExpandVector();

   fVector->push_back(Reference());

   // by such action no any locking is required -
   fVector->back() << ref;

   return true;
}


bool dabc::ReferencesVector::AddAt(Reference& ref, unsigned pos) throw()
{
   if (!ref.GetObject()) return false;

   ExpandVector();

   fVector->push_back(Reference());

   if (pos >= fVector->size()) {
      fVector->back() << ref;
   } else {
      for (unsigned n=fVector->size()-1; n>pos; n--)
         fVector->at(n) << fVector->at(n-1);
      fVector->at(pos) << ref;
   }

   return true;
}


bool dabc::ReferencesVector::Remove(Object* obj) throw()
{
   if ((obj==0) || (GetSize()==0)) return false;

   unsigned n = GetSize();
   while (n-->0)
      if (fVector->at(n).GetObject()==obj) RemoveAt(n);

   return true;
}


void dabc::ReferencesVector::RemoveAt(unsigned n) throw()
{
   if (n>=fVector->size()) return;

   Reference ref;
   ref << fVector->at(n);
   ref.Release();

   for (unsigned indx = n; indx < fVector->size()-1;indx++)
      fVector->at(indx) << fVector->at(indx+1);

   fVector->pop_back();
}

dabc::Reference dabc::ReferencesVector::TakeRef(unsigned n)
{
   dabc::Reference ref;

   ExtractRef(n, ref);

   return ref;
}


bool dabc::ReferencesVector::ExtractRef(unsigned n, Reference& ref)
{
   if (n>=GetSize()) return false;

   // by this action reference will be removed from the vector
   ref << fVector->at(n);
   RemoveAt(n);
   return true;
}

bool dabc::ReferencesVector::ExtractRef(Object* obj, Reference& ref)
{
   for (unsigned n=0;n<GetSize();n++)
      if (fVector->at(n).GetObject() == obj)
         return ExtractRef(n, ref);
   return false;
}



dabc::Reference dabc::ReferencesVector::TakeLast()
{
   dabc::Reference ref;

   if (GetSize()>0) {
      ref << fVector->back();
      fVector->pop_back();
   }

   return ref;
}


bool dabc::ReferencesVector::Clear(bool isowner) throw()
{
   for (unsigned n=0;n<fVector->size();n++) {
      dabc::Reference ref;
      ref << fVector->at(n);
      if (isowner) ref.Destroy();
              else ref.Release();
   }

   fVector->clear();

   return true;
}

dabc::Object* dabc::ReferencesVector::FindObject(const char* name, int len) const
{
   for (unsigned n = 0; n < fVector->size(); n++) {
      dabc::Object* obj = fVector->at(n).GetObject();
      if (obj && obj->IsName(name, len)) return obj;
   }
   return nullptr;
}

bool dabc::ReferencesVector::HasObject(Object* ptr)
{
   if (!ptr) return false;
   for (unsigned n = 0; n < fVector->size(); n++)
      if (fVector->at(n).GetObject() == ptr) return true;

   return false;
}
