#include "dabc/Basic.h"

#include "dabc/Manager.h"

#include "dabc/logging.h"

#include "dabc/threads.h"

long dabc::Basic::gNumInstances = 0;


namespace dabc {

   const char* xmlInputQueueSize    = "InputQueueSize";
   const char* xmlOutputQueueSize   = "OutputQueueSize";
   const char* xmlUserHeaderSize    = "UserHeaderSize";

   const char* xmlPoolName          = "PoolName";
   const char* xmlWorkPool          = "WorkPool";
   const char* xmlBufferSize        = "BufferSize";
   const char* xmlNumBuffers        = "NumBuffers";
   const char* xmlNumInputs         = "NumInputs";
   const char* xmlNumOutputs        = "NumOutputs";
   const char* xmlUseAcknowledge    = "UseAcknowledge";

   const char* xmlTrueValue         = "true";
   const char* xmlFalseValue        = "false";

   const char* typeSocketDevice     = "dabc::SocketDevice";
}


dabc::Basic::Basic(Basic* parent, const char* name) :
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

   if (fCleanup && dabc::mgr())
      dabc::mgr()->ObjectDestroyed(this);

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

bool dabc::Basic::Find(ConfigIO &cfg)
{
   return false;
}


void dabc::Basic::_SetParent(Mutex* mtx, Basic* parent)
{
   fMutex = mtx;
   fParent = parent;
}

void dabc::Basic::MakeFullName(std::string &fullname, Basic* upto) const
{
   fullname.assign("");

   LockGuard guard(GetMutex());

   _MakeFullName(fullname, upto);
}

void dabc::Basic::_MakeFullName(std::string &fullname, Basic* upto) const
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


std::string dabc::Basic::GetFullName(Basic* upto) const
{
   std::string res;
   MakeFullName(res, upto);
   return res;
}

void dabc::Basic::FillInfo(std::string& info)
{
   dabc::formats(info, "Object: %s", GetName());
}

void dabc::Basic::DeleteThis()
{
   if (dabc::mgr())
      dabc::mgr()->DestroyObject(this);
              else delete this;
}
