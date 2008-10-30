#include "dabc/Basic.h"

#include "dabc/Manager.h"

#include "dabc/logging.h"

#include "dabc/threads.h"

long dabc::Basic::gNumInstances = 0;

dabc::Basic::Basic(Basic* parent, const char* name) :
   fManager(0),
   fMutex(0),
   fParent(0),
   fName(name),
   fCleanup(false),
   fAppId(0)
{
   gNumInstances++; 
    
   if (parent!=0) parent->AddChild(this); 
}

dabc::Basic::~Basic()
{
   DOUT5(("dabc::Basic::~Basic() %p %s", this, GetName()));

   if (fCleanup && fManager)
      fManager->ObjectDestroyed(this); 
    
   if (fParent!=0) fParent->RemoveChild(this);
   gNumInstances--;

   DOUT5(("dabc::Basic::~Basic() %p %s done", this, GetName()));
}

void dabc::Basic::AddChild(Basic* obj)
{
   EOUT(("not implemented"));
}

void dabc::Basic::RemoveChild(Basic* obj)
{
   EOUT(("not implemented"));
}

void dabc::Basic::_SetParent(Manager* mgr, Mutex* mtx, Basic* parent)
{
   fManager = mgr;
   fMutex = mtx;
   fParent = parent;
}

void dabc::Basic::MakeFullName(String &fullname, Basic* upto) const
{
   fullname.assign("");

   LockGuard guard(GetMutex()); 
   
   _MakeFullName(fullname, upto);
}

void dabc::Basic::_MakeFullName(String &fullname, Basic* upto) const
{
   if ((fParent!=0) && (fParent != upto)) {
      fParent->_MakeFullName(fullname, upto);
      fullname.append("/");
   }
      
   if (fParent==0) fullname.append("/");
   
   fullname.append(GetName());
}

void dabc::Basic::SetName(const char* name)
{
   LockGuard guard(GetMutex()); 
   
   fName = name;
}


dabc::String dabc::Basic::GetFullName(Basic* upto) const
{
   dabc::String res;
   MakeFullName(res, upto);
   return res; 
}

void dabc::Basic::FillInfo(String& info)
{
   dabc::formats(info, "Object: %s", GetName());
}

void dabc::Basic::DeleteThis()
{
   Manager* mgr = GetManager();
   if (mgr!=0) mgr->DestroyObject(this);
          else delete this; 
}
