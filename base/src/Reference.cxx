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

#include "dabc/Reference.h"

#include "dabc/Object.h"
#include "dabc/Exception.h"
#include "dabc/string.h"
#include "dabc/logging.h"

#include <stdio.h>
#include <string.h>


dabc::Reference::Reference() :
   fObj(0)
{
}

dabc::Reference::Reference(Object* obj) throw() :
   fObj(0)
{
   // this is empty reference
   if (obj==0) return;

   if (obj->IncReference())
      fObj = obj;
   else
      throw dabc::Exception(ex_Object, dabc::format("Cannot assign reference to object %p", obj), obj->GetName() );
}

bool dabc::Reference::ConvertToString(char* buf, int buflen)
{
   int res = snprintf(buf, buflen, "%p", fObj);

   if ((res<0) || (res==buflen)) {
      EOUT("To small buffer len %d to convert reference!!!", buflen);
      return false;
   }

   fObj = 0;
   return true;
}


dabc::Reference::Reference(const char* buf, int buflen) throw() :
   fObj(0)
{
   if (buf==0 || (buflen==0)) return;

   if (sscanf(buf,"%p", &fObj)!=1) {
      fObj = 0;
      throw dabc::Exception(ex_Object, dabc::format("Cannot reconstruct reference from string %s", buf), "Reference" );
   }
}


void dabc::Reference::SetObject(Object* obj, bool withmutex) throw()
{
   Release();

   // this is empty reference
   if (obj==0) return;

   if (obj->IncReference(withmutex))
      fObj = obj;
   else
      throw dabc::Exception(ex_Object, dabc::format("Cannot assign reference to object %p"), DNAME(obj) );
}

void dabc::Reference::Assign(const Reference& src) throw()
{
   if (src.fObj->IncReference())
      fObj = src.fObj;
   else
      throw dabc::Exception(ex_Object, dabc::format("Cannot assign reference to object %p", src.fObj), src.GetName());
}

dabc::Reference::Reference(const Reference& src) throw() :
   fObj(0)
{
   if (!src.null()) Assign(src);
}

dabc::Reference& dabc::Reference::operator=(const Reference& src) throw()
{
//   Print(0, "From operator=()");

   Release();

   if (!src.null()) Assign(src);

   return *this;
}

dabc::Reference& dabc::Reference::operator=(Object* obj) throw()
{
   Release();

   if (obj!=0) {
      Reference ref(obj);
      *this << ref;
   }

   return *this;
}


dabc::Reference& dabc::Reference::operator<<(Reference& src) throw()
{
   Release();

   // we are trying to avoid situation, that both references have same pointer in a time
   Object* temp = src.fObj;

   src.fObj = 0;

   fObj = temp;

   return *this;
}

dabc::Reference dabc::Reference::Take()
{
   dabc::Reference res;
   res.fObj = fObj;
   fObj = 0;

   return res;
}


unsigned dabc::Reference::NumReferences() const
{
   return GetObject() ? GetObject()->NumReferences() : 0;
}


void dabc::Reference::SetAutoDestroy(bool on)
{
   if (GetObject()) GetObject()->SetAutoDestroy(on);
}


dabc::Reference::Reference(bool withmutex, Object* obj) throw() :
   fObj(0)
{
   if (obj) {
      if (obj->IncReference(withmutex))
         fObj = obj;
      else
         throw dabc::Exception(ex_Object, dabc::format("Cannot assign reference to object %p mutex %s", obj, DBOOL(withmutex)), DNAME(obj));
   }
}


dabc::Reference::~Reference()
{
   Release();
}

void dabc::Reference::Release() throw()
{
   if (fObj==0) return;
   if (fObj->DecReference(false))
      // special case - object is not referenced and want to be destroyed - let help him
      delete fObj;
   fObj = 0;
}


void dabc::Reference::Destroy() throw()
{
   if (fObj==0) return;

   // we ask if object can be destroyed when reference is decreased
   if (fObj->DecReference(true))
      // if yes, state of the object will be blocked - now new references and one can delete object
      // but it does not means that object disappear immediately - it may be cleanup in its own thread
      delete fObj;

   fObj = 0;
}


dabc::Object* dabc::Reference::GetParent() const
{
   return GetObject() ? GetObject()->GetParent() : 0;
}

const char* dabc::Reference::GetName() const
{
   return GetObject() ? GetObject()->GetName() : "---";
}

const char* dabc::Reference::ClassName() const
{
   return GetObject() ? GetObject()->ClassName() : "---";
}

bool dabc::Reference::IsName(const char* name) const
{
   if ((name==0) || (*name==0) || (GetObject()==0)) return false;
   return GetObject()->IsName(name);
}

dabc::Mutex* dabc::Reference::ObjectMutex() const
{
   return GetObject() ? GetObject()->ObjectMutex() : 0;
}

bool dabc::Reference::AddChild(Object* obj)
{
   return GetObject() ? GetObject()->AddChild(obj, true) : false;
}


unsigned dabc::Reference::NumChilds() const
{
   return GetObject() ? GetObject()->NumChilds() : 0;
}

dabc::Reference dabc::Reference::GetChild(unsigned n) const
{
   return GetObject() ? GetObject()->GetChildRef(n) : Reference();
}

bool dabc::Reference::GetAllChildRef(ReferencesVector* vect)
{
   if ((GetObject()==0) || (vect==0)) return false;
   return GetObject()->GetAllChildRef(vect);
}

dabc::Reference dabc::Reference::FindChild(const char* name) const
{
   return GetObject() ? GetObject()->FindChildRef(name) : Reference();
}

bool dabc::Reference::RemoveChild(const char* name, bool cleanup)
{
   if (!GetObject()) return false;

   return GetObject()->RemoveChild(GetObject()->FindChild(name), cleanup);
}


bool dabc::Reference::RemoveChilds(bool cleanup)
{
   return GetObject() ?  GetObject()->RemoveChilds(cleanup) : true;
}


void dabc::Reference::Print(int lvl, const char* from) const
{
   dabc::lgr()->Debug(lvl,"filename",1,"funcname", dabc::format("%s REF:%p obj:%p", (from ? from : ""), this, fObj).c_str());
}

dabc::Reference dabc::Reference::GetFolder(const char* name, bool force) throw()
{
   if (GetObject()==0) return Reference();

   return GetObject()->GetFolder(name, force);
}

std::string dabc::Reference::ItemName(bool compact) const
{
   if (GetObject()==0) return std::string();

   return GetObject()->ItemName(compact);
}

std::string dabc::Reference::RelativeName(const dabc::Reference& topitem)
{
   if (null() || topitem.null()) return "";

   std::string res;

   GetObject()->FillFullName(res, topitem(), true);

   return res;
}
