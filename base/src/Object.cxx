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

#include "dabc/Object.h"

#include <cstdlib>
#include <fnmatch.h>

#include "dabc/Manager.h"
#include "dabc/ReferencesVector.h"

unsigned dabc::Object::gNumInstances = 0;
unsigned dabc::Object::gNumCreated = 0;

namespace dabc {


   const char* xmlDeviceNode        = "Device";
   const char* xmlThreadNode        = "Thread";
   const char* xmlMemoryPoolNode    = "MemoryPool";
   const char* xmlModuleNode        = "Module";
   const char* xmlConnectionNode    = "Connection";

   const char* xmlQueueAttr         = "queue";
   const char* xmlBindAttr          = "bind";
   const char* xmlSignalAttr        = "signal";
   const char* xmlRateAttr          = "rate";
   const char* xmlLoopAttr          = "loop";
   const char* xmlInputQueueSize    = "InputQueueSize";
   const char* xmlOutputQueueSize   = "OutputQueueSize";
   const char* xmlInlineDataSize    = "InlineDataSize";

   const char* xmlPoolName          = "PoolName";
   const char* xmlWorkPool          = "Pool";
   const char* xmlFixedLayout       = "FixedLayout";
   const char* xmlCleanupTimeout    = "CleanupTimeout";
   const char* xmlBufferSize        = "BufferSize";
   const char* xmlNumBuffers        = "NumBuffers";
   const char* xmlAlignment         = "Alignment";
   const char* xmlShowInfo          = "ShowInfo";

   const char* xmlNumInputs         = "NumInputs";
   const char* xmlNumOutputs        = "NumOutputs";
   const char* xmlInputPrefix       = "Input";
   const char* xmlInputMask         = "Input%u";
   const char* xmlOutputPrefix      = "Output";
   const char* xmlOutputMask        = "Output%u";
   const char* xmlUseAcknowledge    = "UseAcknowledge";
   const char* xmlFlushTimeout      = "FlushTimeout";
   const char* xmlConnTimeout       = "ConnTimeout";

   const char* xmlMcastAddr         = "McastAddr";
   const char* xmlMcastPort         = "McastPort";
   const char* xmlMcastRecv         = "McastRecv";

   const char* xmlProtocol          = "Protocol";
   const char* xmlHostName          = "HostName";
   const char* xmlFileName          = "FileName";
   const char* xmlFileNumber        = "FileNumber";
   const char* xmlFileSizeLimit     = "FileSizeLimit";
   const char* xml_maxsize          = "maxsize";
   const char* xml_number           = "number";
   const char* xml_flush            = "flush";

   const char* typeThread           = "dabc::Thread";
   const char* typeDevice           = "dabc::Device";
   const char* typeSocketDevice     = "dabc::SocketDevice";
   const char* typeSocketThread     = "dabc::SocketThread";
   const char* typeApplication      = "dabc::Application";


#ifdef DABC_EXTRA_CHECKS
   PointersVector gObjectGarbageCollector;
   Mutex          gObjectGarbageMutex;
#endif

}

/**  Resolve problem with child-parent mutex locking. Now this completely avoidable.
 *   Parent pointer was and is safe - as long object is exists parent reference is preserved,
 *  only in destructor parent will be released. Child reference can disappear in any time,
 *  therefore it was difficult to get reference on child while only pointer is not enough -
 *  somebody else can remove it very fast. Therefore fObjectBlock counter is introduced.
 *  As long it is non-zero no any removal of childs are allowed. As result, reference on
 *  all childs are preserved. One can release parent lock, acquire reference on child
 *  (which includes child mutex locking) and after that again parent lock and decrement of block counter.
 *  This only limits situation during removal of childs - it all correspondent methods one should respect block counter.
 */


dabc::Object::Object(const std::string &name, unsigned flags) :
   fObjectFlags(flags),
   fObjectParent(),
   fObjectName(name),
   fObjectMutex(nullptr),
   fObjectRefCnt(0),
   fObjectChilds(nullptr),
   fObjectBlock(0)
{
   DOUT5("Created object %s %p", GetName(), this);

   Constructor();
}

dabc::Object::Object(Reference parent, const std::string &name, unsigned flags) :
   fObjectFlags(flags),
   fObjectParent(parent),
   fObjectName(name),
   fObjectMutex(nullptr),
   fObjectRefCnt(0),
   fObjectChilds(nullptr),
   fObjectBlock(0)
{
   DOUT5("Object created %s %p", GetName(), this);

   Constructor();
}


dabc::Object::Object(const ConstructorPair& pair, unsigned flags) :
   fObjectFlags(flags),
   fObjectParent(pair.parent),
   fObjectName(pair.name),
   fObjectMutex(nullptr),
   fObjectRefCnt(0),
   fObjectChilds(nullptr),
   fObjectBlock(0)
{
   DOUT5("Object created %s %p", GetName(), this);

   Constructor();
}


dabc::Object::~Object()
{
   Destructor();

   DOUT5("Object destroyed %s %p", GetName(), this);
}

void dabc::Object::SetOwner(bool on)
{
   LockGuard lock(fObjectMutex);
   SetFlag(flIsOwner, on);
}

void dabc::Object::SetAutoDestroy(bool on)
{
   LockGuard lock(fObjectMutex);
   SetFlag(flAutoDestroy, on);
}


bool dabc::Object::IsOwner() const
{
   LockGuard lock(fObjectMutex);
   return GetFlag(flIsOwner);
}

bool dabc::Object::IsLogging() const
{
   LockGuard lock(fObjectMutex);
   return GetFlag(flLogging);
}

bool dabc::Object::IsChildsHidden() const
{
   LockGuard lock(fObjectMutex);
   return GetFlag(flChildsHidden);
}

bool dabc::Object::IsTopXmlLevel() const
{
   LockGuard lock(fObjectMutex);
   return GetFlag(flTopXmlLevel);
}

bool dabc::Object::IsHidden() const
{
   LockGuard lock(fObjectMutex);
   return GetFlag(flHidden);
}

void dabc::Object::SetLogging(bool on)
{
   LockGuard lock(fObjectMutex);
   return SetFlag(flLogging, on);
}

bool dabc::Object::IsName(const char* str, int len) const
{
   if ((len == 0) || !str) return false;
   if (len < 0) return fObjectName.compare(str) == 0;
   return ((int) fObjectName.length()==len) && (fObjectName.compare(0, len, str,len) == 0);
}

void dabc::Object::Constructor()
{
   gNumInstances++;

   // use non-recursive mutexes to see faster deadlock problems
   // FIXME: should we use recursive in final version?

   if (!GetFlag(flNoMutex))
      fObjectMutex = new Mutex(false);

   SetState(stNormal);

   fObjectParent.AddChild(this);
}

void dabc::Object::Destructor()
{
   #ifdef DABC_EXTRA_CHECKS
   {
      LockGuard lock(gObjectGarbageMutex);
      gObjectGarbageCollector.remove(this);
   }
   #endif

   Mutex* m = nullptr;
   bool delchilds = false;

   {
      LockGuard lock(fObjectMutex);

      if ((GetState() != stDestructor) && (fObjectRefCnt != 0)) {
         EOUT("Object %p %s deleted not via Destroy method refcounter %u", this, GetName(), fObjectRefCnt);
      }

      SetState(stDestructor);

      if (fObjectRefCnt != 0) {
         EOUT("!!!!!!!!!!!! Destructor called when refcounter %u obj:%s %p", fObjectRefCnt, GetName(), this);
//         Object* obj = (Object*) 29387898;
//         delete obj;
      }

      if (fObjectChilds) {

         if (fObjectChilds->GetSize() > 0)
            EOUT("!!!!!!! CHILDS %u ARE NOT DELETED completely obj:%s %p", fObjectChilds->GetSize(), GetName(), this);

         if (fObjectBlock > 0)
            EOUT("!!!! CHILDS ARE STILL BLOCKED %d!!!!!!!", fObjectBlock);

         delchilds = true;
      }

      m = fObjectMutex;
      fObjectMutex = nullptr;
   }

   if (delchilds) RemoveChilds();

   // release reference on parent, remove from child should be already done
   fObjectParent.Release();

   // destroy mutex
   delete m; m = nullptr;

   gNumInstances--;
}

bool dabc::Object::IncReference(bool withmutex)
{
   dabc::LockGuard lock(withmutex ? fObjectMutex : nullptr);

   if (GetState() == stDestructor) {
      EOUT("OBJ:%p %s Inc reference during destructor", this, GetName());
      return false;
//         EOUT("Obj:%p %s Class:%s IncReference %u inside destructor :(",
//               this, GetName(), ClassName(), fObjectRefCnt);
   }

   fObjectRefCnt++;

   if (GetFlag(flLogging))
      DOUT0("Obj:%s %p Class:%s IncReference +----- %u thrd:%s", GetName(), this, ClassName(), fObjectRefCnt, dabc::mgr.CurrentThread().GetName());

   return true;
}

void dabc::Object::SetCleanupBit()
{
   dabc::LockGuard lock(fObjectMutex);
   SetFlag(flCleanup, true);
}

bool dabc::Object::_IsNormalState()
{
   return GetState() == stNormal;
}


bool dabc::Object::IsNormalState()
{
   dabc::LockGuard lock(fObjectMutex);

   return _IsNormalState();
}


bool dabc::Object::DecReference(bool ask_to_destroy, bool do_decrement, bool from_thread)
{
   // check destroyment before we lock the mutex
   // if method returns true, object destructor should follow

   bool viathrd = false;

   {
      dabc::LockGuard lock(fObjectMutex);

//      if (GetFlag(flLogging)) {
//           DOUT0("Call DecReference for object %p %s refcnt = %d, ask_to_destroy %s do_decrement %s state %d from_thread %s", this, GetName(), fObjectRefCnt, DBOOL(ask_to_destroy), DBOOL(do_decrement), GetState(), DBOOL(from_thread));
//      }

//      if (GetFlag(flLogging))
//         DOUT0("Obj:%s %p  Class:%s DecReference %u destroy %s dodec %s state %d numchilds %u flags %x",
//               GetName(), this, ClassName(),
//               fObjectRefCnt, DBOOL(ask_to_destroy), DBOOL(do_decrement), GetState(),
//               (fObjectChilds ? fObjectChilds->GetSize() : 0), fObjectFlags);

      if (do_decrement) {

         if (fObjectRefCnt == 0) {
            EOUT("Obj %p name:%s class:%s Reference counter is already 0", this, GetName(), ClassName());
            throw dabc::Exception(ex_Object, "Reference counter is 0 - cannot decrease", GetName());
            return false;
         }

         // if (IsLogging()) DOUT0("Dec object refcounter in 389");
         fObjectRefCnt--;

         if (GetFlag(flLogging))
            DOUT0("Obj:%s %p Class:%s DecReference ----+- %u thrd:%s", GetName(), this, ClassName(), fObjectRefCnt, dabc::mgr.CurrentThread().GetName());
      }

      switch (GetState()) {
         case stConstructor:
            EOUT("Object %s is not yet constructed - most probably, big failure", GetName());
            return false;

         case stNormal:
            // if autodestroy flag specified, object will be destroyed when no more external references are existing

            // if (GetFlag(flAutoDestroy) && (fObjectRefCnt <= (int)(fObjectChilds ? fObjectChilds->GetSize() : 0))) ask_to_destroy = true;
            if (GetFlag(flAutoDestroy) && (fObjectRefCnt == 0)) ask_to_destroy = true;

            if (do_decrement && fObjectChilds && ((unsigned) fObjectRefCnt == fObjectChilds->GetSize()) && GetFlag(flAutoDestroy)) {
               ask_to_destroy = true;
               // DOUT0("One could destroy object %s %p anyhow numrefs %u numchilds %u", GetName(), this, fObjectRefCnt, fObjectChilds->GetSize());
            }

            if (!ask_to_destroy) return false;
            viathrd = GetFlag(flHasThread);
            break;

         // we are waiting for the thread, only thread call can unblock
         case stWaitForThread:
            if (!from_thread) {
               DOUT2("Object %p %s tried to destroy not from thread - IGNORE", this, GetName());
               return false;
            }
            break;

         // recursive call, ignore it
         case stDoingDestroy:
            return false;

         // if object already so far, it can be destroyed when no references remained
         case stWaitForDestructor:
            if (fObjectRefCnt == 0) {
               if (_DoDeleteItself()) return false;

               // once return true, never do it again
               // returning true means that destructor will be immediately call
               SetState(stDestructor);

               return true;
            }

            #ifdef DABC_EXTRA_CHECKS
            {
              LockGuard lock(gObjectGarbageMutex);
              if (!gObjectGarbageCollector.has_ptr(this))
                 gObjectGarbageCollector.emplace_back(this);
            }
            #endif
            return false;

         // in principal error, we are inside destructor
         case stDestructor:
            return false;
      }

      if (viathrd) {
         SetState(stWaitForThread);
         // we delegate reference counter to the thread
         fObjectRefCnt++;
         if (GetFlag(flLogging))
            DOUT0("Obj:%s %p Class:%s IncReference --+--- %u", GetName(), this, ClassName(), fObjectRefCnt);
      } else {
         SetState(stDoingDestroy);
      }
   }

   if (viathrd) {
      if (DestroyByOwnThread()) return false;

      // it means thread do not want to destroy us, let do it normal way
      dabc::LockGuard lock(fObjectMutex);
      SetState(stDoingDestroy);
      // thread reject destroyment, therefore refcounter can be decreased
      // if (IsLogging()) DOUT0("Dec object refcounter in 459");
      fObjectRefCnt--;

      if (GetFlag(flLogging))
         DOUT0("Obj:%s %p Class:%s DecReference -----+ %u", GetName(), this, ClassName(), fObjectRefCnt);
   }


   if (IsLogging()) DOUT0("Calling object %p cleanup from_thrd=%s do_decrement=%s", this, DBOOL(from_thread), DBOOL(do_decrement));

   // Main point of all destroyment - call cleanup in correct place
   ObjectCleanup();

//   if (IsLogging()) DOUT0("Did object cleanup");

   if (IsLogging())
      DOUT0("DecRefe after cleanup %p cleanup %s mgr %p", this, DBOOL(GetFlag(flCleanup)), dabc::mgr());

   {

      LockGuard guard(fObjectMutex);

      if (GetFlag(flCleanup) && dabc::mgr()) {
         // cleanup will be done by manager via reference
         fObjectRefCnt++;
         SetState(stWaitForDestructor);
         if (GetFlag(flLogging))
            DOUT0("Obj:%s %p Class:%s IncReference ---+-- %u", GetName(), this, ClassName(), fObjectRefCnt);
      } else
      if (fObjectRefCnt == 0) {
         // no need to deal with manager - can call destructor immediately
         DOUT3("Obj:%p can be destroyed", this);
         if (_DoDeleteItself()) {
            SetState(stWaitForDestructor);
            return false;
         }
         SetState(stDestructor);
         return true;
      } else {
         /** somebody else has reference, when it release reference object will be automatically destroyed */
         SetState(stWaitForDestructor);
         #ifdef DABC_EXTRA_CHECKS
         {
            LockGuard lock(gObjectGarbageMutex);
            if (!gObjectGarbageCollector.has_ptr(this))
               gObjectGarbageCollector.emplace_back(this);
         }
         #endif
         return false;
      }
   }

   // special case - we already increment reference counter
   Reference ref;
   ref.fObj = this;
   if (dabc::mgr()->DestroyObject(ref)) return false;

   // here reference will be released if manager does not accept destroyment

   LockGuard guard(fObjectMutex);

   if (fObjectRefCnt == 0) {
      // no need to deal with manager - can call destructor immediately
      if (_DoDeleteItself()) {
         SetState(stWaitForDestructor);
         return false;
      }

      DOUT3("Obj:%p can be destroyed", this);
      SetState(stDestructor);
      return true;
   }

   return false;
}

bool dabc::Object::DestroyCalledFromOwnThread()
{
   return DecReference(true, true, true);
}

void dabc::Object::DeleteThis()
{
   if (IsLogging())
      DOUT1("OBJ:%p %s DELETETHIS cnt %u", this, GetName(), fObjectRefCnt);

   {
      LockGuard lock(fObjectMutex);

      if (GetState()!=stNormal) {
//         EOUT("Object %p already on the way to destroyment - do not harm it", this);
         return;
      }

      // this flag force object to be cleanup via manager
      SetFlag(flCleanup, true);
   }

   Destroy(this);
}




void dabc::Object::ObjectCleanup()
{
   RemoveChilds();

   if (IsLogging()) {
      DOUT0("Obj:%p %s refcnt %u Before remove from parent %p", this, GetName(), fObjectRefCnt, fObjectParent());
   }

   // Than we remove reference on the object from parent
   if (!fObjectParent.null())
      fObjectParent()->RemoveChild(this, false);

   if (!fObjectParent.null()) {
      EOUT("Parent still not yet cleaned !!!!");
      fObjectParent.Release();
   }

   if (IsLogging()) {
      DOUT0("Obj:%p %s refcnt %u after remove from parent", this, GetName(), fObjectRefCnt);
   }

   DOUT3("Obj:%s Class:%s Finish cleanup numrefs %u", GetName(), ClassName(), NumReferences());

}

unsigned dabc::Object::NumReferences()
{
   dabc::LockGuard lock(fObjectMutex);

   return fObjectRefCnt;
}


bool dabc::Object::AddChild(Object* child, bool withmutex) throw()
{
   return AddChildAt(child, (unsigned) -1, withmutex);
}

bool dabc::Object::AddChildAt(Object* child, unsigned pos, bool withmutex)
{
   if (!child) return false;

   if (!child->fObjectParent.null() && (child->GetParent() != this)) {
      EOUT("Cannot add child from other parent directly - first remove from old");
      throw dabc::Exception(ex_Object, "Cannot add child from other parent", GetName());
      return false;
   }

   Reference ref(child);

   LockGuard guard(withmutex ? fObjectMutex : nullptr);

   if (child->fObjectParent.null()) {
      child->fObjectParent.fObj = this; // not very nice, but will work
      fObjectRefCnt++;
   }

   // if object is owner of all childs, any child will get autodestroy flag
   if (GetFlag(flIsOwner)) child->SetAutoDestroy(true);

   if (!fObjectChilds) fObjectChilds = new ReferencesVector;

   if (pos == (unsigned) -1)
      fObjectChilds->Add(ref);
   else
      fObjectChilds->AddAt(ref, pos);

   _ChildsChanged();

   return true;
}

bool dabc::Object::RemoveChild(Object* child, bool cleanup) throw()
{
   if (!child) return false;

   if (child->fObjectParent() != this) return false;

   int cnt = 1000000;

   bool isowner = false;

   dabc::Reference childref;

   while (--cnt>0) {
      LockGuard guard(fObjectMutex);

      if (!fObjectChilds) return false;

      if (fObjectBlock > 0) continue;

      isowner = GetFlag(flIsOwner);

      if (fObjectChilds->ExtractRef(child, childref)) {
         // IMPORTANT: parent reference can be released only AFTER childs are decreased,
         // otherwise reference will try to destroy parent
         // IMPORTANT: we are under object mutex and can do anything

         _ChildsChanged();

         if (child->fObjectParent.fObj == this) {
            child->fObjectParent.fObj = nullptr; // not very nice, but will work
            if (fObjectRefCnt>1) {
               fObjectRefCnt--;
            } else {
               fObjectRefCnt = 0;
               if (fObjectChilds->GetSize() > 0)
                  DOUT0("Object %p %s refcnt == 0 when numchild %u", this, GetName(), fObjectChilds->GetSize());
            }
         }
         break;
      }

      EOUT("Not able to extract child reference!!!");
      exit(333);
   }

   if (cleanup && isowner) childref.Destroy();
                      else childref.Release();

   #ifdef DABC_EXTRA_CHECKS
   if (cnt < 999990)
      DOUT0("Object %s Retry %d time before childs were unblocked", GetName(), 1000000-cnt);
   #endif

   if (cnt == 0) {
      EOUT("HARD error!!!! - For a long time fObjectBlock=%d is blocked in object %p %s", fObjectBlock, this, GetName());
      exit(055);
   }

   return true;
}

bool dabc::Object::RemoveChildAt(unsigned n, bool cleanup) throw()
{
   return RemoveChild(GetChild(n), cleanup);
}

unsigned dabc::Object::NumChilds() const
{
   LockGuard guard(fObjectMutex);

   return fObjectChilds ? fObjectChilds->GetSize() : 0;
}

dabc::Object* dabc::Object::GetChild(unsigned n) const
{
   LockGuard guard(fObjectMutex);

   return fObjectChilds ? fObjectChilds->GetObject(n) : nullptr;
}

dabc::Reference dabc::Object::GetChildRef(unsigned n) const
{
   LockGuard guard(fObjectMutex);

   Object *obj = fObjectChilds ? fObjectChilds->GetObject(n) : nullptr;

   if (!obj) return Reference();

   IntGuard block(fObjectBlock);

   UnlockGuard unlock(fObjectMutex);

   return dabc::Reference(obj);

   // here will be mutex locked, fObjectBlock counter decremented and finally mutex unlocked again
}

bool dabc::Object::GetAllChildRef(ReferencesVector* vect) const
{
   if (!vect) return false;

   LockGuard guard(fObjectMutex);

   if (!fObjectChilds) return true;

   PointersVector ptrs;

   for (unsigned n=0; n<fObjectChilds->GetSize(); n++)
      ptrs.emplace_back(fObjectChilds->GetObject(n));

   IntGuard block(fObjectBlock);

   UnlockGuard unlock(fObjectMutex);

   for (unsigned n=0;n<ptrs.size();n++) {
      Reference ref((Object*)ptrs[n]);
      vect->Add(ref);
   }

   return true;

   // here will be mutex locked, fObjectBlock counter decremented and finally mutex unlocked again

}


dabc::Object* dabc::Object::FindChild(const char* name) const
{
   return FindChildRef(name, false)();
}

dabc::Reference dabc::Object::FindChildRef(const char* name, bool force) const throw()
{
   Reference ref(const_cast<Object*> (this));

   return SearchForChild(ref, name, true, force);
}

dabc::Reference dabc::Object::SearchForChild(Reference& ref, const char* name, bool firsttime, bool force) throw()
{
   if (ref.null()) return ref;

   if (!name || (strlen(name) == 0)) return ref;

   if (*name=='/') {
      if (firsttime) {
         while (ref()->GetParent())
            ref = Reference(ref()->GetParent());
         return SearchForChild(ref, name+1, false, force);
      } else {
         // skip all slashes in the beginning
         while (*name == '/') name++;
         if (*name == 0) return ref;
      }
   }

   int len = strlen(name);

   if ((len >= 2) && (name[0] == '.') && (name[1] == '.')) {
      if (len == 2)
         return Reference(ref()->GetParent());

      if (name[2] == '/') {
         ref = Reference(ref()->GetParent());
         return SearchForChild(ref, name + 3, false, force);
      }
   }

   if ((len >= 1) && (name[0] == '.')) {
      if (len == 1)
         return ref;
      if (name[1] == '/')
         return SearchForChild(ref, name + 2, false, force);
   }

   const char* ptok = name;
   while (*ptok != 0) {
      if (*ptok == '/') break;
      ptok++;
   }

   Reference newref;

   {
      LockGuard guard(ref()->fObjectMutex);

      dabc::Object *obj = (ref()->fObjectChilds==nullptr) ? nullptr : ref()->fObjectChilds->FindObject(name, ptok-name);

      // create new object under parent mutex - no one can create double entries simultaneously
      if ((obj==nullptr) && force) {
         obj = ref()->CreateInstance(std::string(name, ptok-name));
         ref()->AddChild(obj, false);
      }

      if (obj) {
         IntGuard block(ref()->fObjectBlock);

         UnlockGuard unlock(ref()->fObjectMutex);

         // make reference outside parent mutex
         newref = dabc::Reference(obj);
      }
   }

   ref << newref;

   if (*ptok == 0) return ref;

   return SearchForChild(ref, ptok+1, false, force);
}


dabc::Reference dabc::Object::GetFolder(const std::string &name, bool force) throw()
{
   Reference ref(this);

   return SearchForChild(ref, name.c_str(), true, force);
}

bool dabc::Object::RemoveChilds(bool cleanup)
{
   ReferencesVector* del_vect = nullptr;

   int cnt = 1000000;

   bool isowner = false;

   while (--cnt > 0) {
      LockGuard guard(fObjectMutex);

      // nothing to do
      if (!fObjectChilds) return true;

      if (fObjectBlock > 0) continue;

      isowner = GetFlag(flIsOwner);
      del_vect = fObjectChilds;
      fObjectChilds = nullptr;
      break;
   }

   #ifdef DABC_EXTRA_CHECKS
   if (cnt < 999990)
      DOUT0("Object %s Retry %d times before childs were unblocked", 1000000-cnt);
   #endif

   if (cnt == 0) {
      EOUT("HARD error!!!! - For a long time fObjectBlock=%d is blocked in object %p %s", fObjectBlock, this, GetName());
      exit(055);
   }

   if (IsLogging())
      DOUT1("Obj:%s Deleting childs:%u", GetName(), del_vect ? del_vect->GetSize() : 0);

   if (del_vect) {

      // ensure that object do not longer reference parent
      for (unsigned n=0;n<del_vect->GetSize();n++) {
         Object* obj = del_vect->GetObject(n);
         if (obj) obj->fObjectParent.Release();
      }

      del_vect->Clear(isowner && cleanup);
      delete del_vect;
   }

   return true;
}

void dabc::Object::SetName(const char *name)
{
   LockGuard guard(fObjectMutex);

   if (fObjectRefCnt > 0) {
      EOUT("Cannot change object name when reference counter %d is not zero!!!", fObjectRefCnt);
      throw dabc::Exception(ex_Object, "Cannot change object name when refcounter is not zero", GetName());
   }

   fObjectName = name;
}


void dabc::Object::SetNameDirect(const char* name)
{
   LockGuard guard(fObjectMutex);

   fObjectName = name;
}


/** \page object_destroy Destroyment of object
 *  Object live may be complex but way to destroy it much more complex.
 *  Problems are:
 *     - multiple references on object from different threads
 *     - need to cleanup of object from the thread where it is used
 *     - object does not want to be destroyed
 *  All this complexity handled by the dabc::Object::Destroy(obj) call.
 *
 *  First, object is checked that if it is assigned with a thread (flHasThread).
 *  It means that object can be actively used in the thread and any cleanup
 *  should happen from this thread. In this case DestroyByOwnThread() method will be called.
 *  Method can return true only when objects allowed to be destroyed immediately
 *
 *  If object does not have thread, ObjectCleanup() will be called immediately,
 *  otherwise it will be called in thread context.
 *  Main motivation for ObjectCleanup() is to release references on
 *  all other objects which may be used inside. One also can free buffers,
 *  close files and do any other relevant job here.
 *
 *  At the end will be checked existing references on the object.
 *  If any exists, object will be moved to manager cleanup queue.
 *  Manager will try to check if reference can be cleared and finally delete object.
 *
 *  TODO: dabc::Object::Destroy() should not be used very often, rather reference on the object must be used
 *  Therefore one should try to avoid dabc::Object::Destroy() as much as possible
 */



void dabc::Object::Destroy(Object* obj) throw()
{
   if (!obj) return;

   if (obj->DecReference(true, false))
      delete obj;
}

bool dabc::Object::Find(ConfigIO &cfg)
{
   return !GetParent() ? false : cfg.FindItem(GetName());
}

dabc::Object::ConstructorPair dabc::Object::MakePair(Reference prnt, const std::string &fullnamearg, bool withmanager)
{
   if (fullnamearg.empty() && prnt.null())
      return ConstructorPair();

   const char* fullname = fullnamearg.empty() ? "---" : fullnamearg.c_str();
   bool isrootfolder = false, isskipparent = false;
   while (*fullname=='/') {
      isrootfolder = true;
      fullname++;
   }

   if (!isrootfolder && (*fullname=='#')) {
      fullname++;
      isskipparent = true;
   }

   if (prnt.null() && withmanager && !isskipparent) {
      if (prnt.null()) prnt = dabc::mgr;
   }

   const char* slash = strrchr(fullname, '/');

   ConstructorPair pair;

   if (slash) {
      pair.name = slash+1;
      std::string path = std::string(fullname, slash - fullname);
      if (!prnt.null())
         prnt = prnt()->FindChildRef(path.c_str(), true);
   } else {
      pair.name = fullname;
   }

   if (!isskipparent)
      pair.parent = prnt;

   return pair;
}

dabc::Object::ConstructorPair dabc::Object::MakePair(Object* prnt, const std::string &fullname, bool withmanager)
{
   return dabc::Object::MakePair(Reference(prnt), fullname, withmanager);
}

dabc::Object::ConstructorPair dabc::Object::MakePair(const std::string &fullname, bool withmanager)
{
   return dabc::Object::MakePair(Reference(), fullname, withmanager);
}


bool dabc::Object::IsParent(Object* obj) const
{
   if (!obj) return false;

   Object* prnt = GetParent();

   while (prnt) {
      if (prnt==obj) return true;
      prnt = prnt->GetParent();
   }
   return false;
}


bool dabc::Object::IsNameMatch(const std::string &mask) const
{
   return NameMatch(fObjectName, mask);
}

bool dabc::Object::NameMatch(const std::string &name, const std::string &mask)
{
   if (mask.empty() || name.empty())
      return name.length()==mask.length();

   size_t lastsepar = 0, separ = mask.find_first_of(';',lastsepar);

   if (separ != std::string::npos) {
      do {
        // FIXME!!!! code missing

         EOUT("Something wrong");
         break;

      } while (lastsepar!=std::string::npos);
   }


   if (mask.find_first_of("*?") == std::string::npos) return name == mask;

   if (mask == "*") return true;

   return fnmatch(mask.c_str(), name.c_str(), FNM_NOESCAPE) == 0;
}

bool dabc::Object::NameMatchM(const std::string &name, const std::string &mask)
{
   size_t separ = mask.find_first_of(':');

   if (separ == std::string::npos) return NameMatch(name, mask);

   size_t lastsepar = 0;

   do {

      std::string submask;

      if (separ == std::string::npos)
         submask = mask.substr(lastsepar);
      else if (separ > lastsepar)
         submask = mask.substr(lastsepar, separ-lastsepar);

      if (!submask.empty())
         if (NameMatch(name, submask)) return true;

      lastsepar = separ;
      if (separ!=std::string::npos) {
         lastsepar++;
         separ = mask.find_first_of(':', lastsepar);
      }

   } while (lastsepar!=std::string::npos);

   return false;
}


void dabc::Object::FillFullName(std::string &fullname, Object* upto, bool exclude_top_parent) const
{
   if (GetParent() && (GetParent() != upto)) {
      GetParent()->FillFullName(fullname, upto, exclude_top_parent);
      fullname.append("/");
   }

   if (!GetParent()) {
      if (exclude_top_parent) return;
      fullname.append("/");
   }
   fullname.append(GetName());
}


std::string dabc::Object::ItemName(bool compact) const
{
   std::string res;
   if (IsParent(dabc::mgr()))
      dabc::mgr()->FillItemName(this, res, compact);
   else
      FillFullName(res, nullptr);

   return res;
}


// ============================================================================
// FIXME: old code, should be adjusted


void dabc::Object::Print(int lvl)
{
   LockGuard guard(ObjectMutex());
   dabc::lgr()->Debug(lvl, "file", 1, "func", dabc::format("Object: %s Class:%s", GetName(), ClassName()).c_str());
}


void dabc::Object::InspectGarbageCollector()
{
#ifdef DABC_EXTRA_CHECKS
   LockGuard lock(gObjectGarbageMutex);

   DOUT0("GarbageCollector: there are %u objects in collector now", gObjectGarbageCollector.size());

   for (unsigned n=0;n<gObjectGarbageCollector.size();n++) {
      Object* obj = (Object*) gObjectGarbageCollector.at(n);
      DOUT0("   obj:%p name:%s class:%s refcnt:%u", obj, obj->GetName(), obj->ClassName(), obj->fObjectRefCnt);
   }

#endif

}



#ifdef DABC_EXTRA_CHECKS

#include <list>
#include <cstdio>
#include <map>

void dabc::Object::DebugObject(const char* classname, Object* instance, int kind)
{

   typedef std::list<dabc::Object*> obj_list;
   typedef std::map<std::string,obj_list> full_map;
   typedef std::map<std::string,int> counts_map;


   static counts_map cnts;
   static full_map objs;
   static dabc::Mutex mutex(true);

   dabc::LockGuard lock(mutex);

   if (classname == 0) {
      printf("NUM ENTRIES = %u\n", (unsigned) cnts.size());
      for (auto iter = cnts.begin(); iter != cnts.end(); iter++) {
         printf("   CLASS = %s  NUM = %d \n", iter->first.c_str(), iter->second);

         auto iter2 = objs.find(iter->first);
         if (iter2 != objs.end())
            for (auto iter3 = iter2->second.begin(); iter3 != iter2->second.end(); iter3++)
               printf("      OBJ:%p NAME = %s\n", *iter3, (*iter3)->GetName());
      }
   } else {
      std::string name(classname);
      if (kind < 0)
         cnts[name]--;
      else
         cnts[name]++;

      if (kind == 10)
         objs[name].emplace_back(instance);
      else if (kind == -10)
         objs[name].remove(instance);
   }
}

#endif
