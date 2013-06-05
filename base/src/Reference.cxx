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
   fObj(0),
   fFlags(flTransient)
{
//   DOUT0("From default constructor");
}

dabc::Reference::Reference(Object* obj, bool owner) throw() :
   fObj(0),
   fFlags(flTransient | (owner ? flOwner : 0))
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
   int res = snprintf(buf, buflen, "%p:%u", fObj, fFlags);

   if ((res<0) || (res==buflen)) {
      EOUT("To small buffer len %d to convert reference!!!", buflen);
      return false;
   }

   fObj = 0;
   fFlags = 0;
   return true;
}


dabc::Reference::Reference(const char* buf, int buflen) throw() :
   fObj(0),
   fFlags(flTransient)
{
   if (buf==0 || (buflen==0)) return;

   if ((strchr(buf,':')==0) || (sscanf(buf,"%p:%u", &fObj, &fFlags)!=2)) {
      fObj = 0;
      fFlags = 0;
      throw dabc::Exception(ex_Object, dabc::format("Cannot reconstruct reference from string %s", buf), "Reference" );
   }
}


void dabc::Reference::SetObject(Object* obj, bool owner, bool withmutex) throw()
{
   Release();

   SetFlag(flOwner, owner);

   fObj = obj;

   // this is empty reference
   if (obj==0) return;

   if (obj->IncReference(withmutex))
      fObj = obj;
   else
      throw dabc::Exception(ex_Object, dabc::format("Cannot assign reference to object %p"), DNAME(obj) );
}

void dabc::Reference::Assign(const Reference& src) throw()
{
   if (src.IsTransient()) {
      fObj = src.fObj;
      SetFlag(flOwner, src.IsOwner());
      (const_cast<Reference*> (&src))->fObj = 0;
   } else
   if (src.fObj->IncReference()) {
      fObj = src.fObj;
      SetFlag(flOwner, false);
   } else
      throw dabc::Exception(ex_Object, dabc::format("Cannot assign reference to object %p", src.fObj), src.GetName() );
}

dabc::Reference::Reference(const Reference& src) throw() :
   fObj(0),
   fFlags(flTransient)
{
   if (!src.null()) Assign(src);
}

dabc::Reference::Reference(const Reference& src, bool transient) throw() :
   fObj(0),
   fFlags(transient ? flTransient : 0)
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

dabc::Reference& dabc::Reference::operator<<(const Reference& src) throw()
{
   Release();

   fObj = src.fObj; (const_cast<Reference*> (&src))->fObj = 0;

   // we move only ownership flag, transient status remains as before
   SetOwner(src.IsOwner());

   // there is no need to change ownership flag, it has no meaning without object pointer
   //src.SetOwner(false);

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


dabc::Reference::Reference(bool withmutex, Object* obj) throw() :
   fObj(0),
   fFlags(flTransient)
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
   if (fObj->DecReference(IsOwner()))
      // special case - object is not referenced and want to be destroyed - let help him
      delete fObj;
   fObj = 0;
   SetFlag(flOwner, false);
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
   SetFlag(flOwner, false);
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

bool dabc::Reference::AddChild(Object* obj, bool setparent)
{
   return GetObject() ? GetObject()->AddChild(obj, true, setparent) : false;
}

dabc::Reference dabc::Reference::PutChild(Object* obj, bool delduplicate)
{
   return GetObject() ? GetObject()->PutChild(obj, delduplicate) : 0;
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

void dabc::Reference::DeleteChilds()
{
   if (GetObject()) GetObject()->DeleteChilds();
}


void dabc::Reference::Print(int lvl, const char* from) const
{
   std::string s;
   dabc::formats(s,"%s REF:%p obj:%p transient:%s owner:%s", (from ? from : ""), this, fObj, DBOOL(IsTransient()), DBOOL(IsOwner()));
   dabc::lgr()->Debug(lvl,"filename",1,"funcname", s.c_str());
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
