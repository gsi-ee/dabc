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

#include <stdlib.h>
#include <fnmatch.h>

#include "dabc/Manager.h"
#include "dabc/logging.h"
#include "dabc/threads.h"
#include "dabc/defines.h"
#include "dabc/ConfigBase.h"
#include "dabc/Worker.h"
#include "dabc/Thread.h"
#include "dabc/ReferencesVector.h"
#include "dabc/Worker.h"
#include "dabc/Thread.h"

unsigned dabc::Object::gNumInstances = 0;
unsigned dabc::Object::gNumCreated = 0;

namespace dabc {


   const char* clPoolHandle         = "PoolHandle";
   const char* clMemoryPool         = "MemoryPool";

   const char* xmlInputQueueSize    = "InputQueueSize";
   const char* xmlOutputQueueSize   = "OutputQueueSize";
   const char* xmlInlineDataSize    = "InlineDataSize";

   const char* xmlPoolName          = "PoolName";
   const char* xmlWorkPool          = "WorkPool";
   const char* xmlInputPoolName     = "InputPoolName";
   const char* xmlOutputPoolName    = "OutputPoolName";
   const char* xmlFixedLayout       = "FixedLayout";
   const char* xmlSizeLimitMb       = "SizeLimitMb";
   const char* xmlCleanupTimeout    = "CleanupTimeout";
   const char* xmlBufferSize        = "BufferSize";
   const char* xmlNumBuffers        = "NumBuffers";
   const char* xmlRefCoeff          = "RefCoeff";
   const char* xmlNumSegments       = "NumSegments";
   const char* xmlAlignment         = "Alignment";
   const char* xmlShowInfo          = "ShowInfo";

   const char* xmlNumInputs         = "NumInputs";
   const char* xmlNumOutputs        = "NumOutputs";
   const char* xmlUseAcknowledge    = "UseAcknowledge";
   const char* xmlFlushTimeout      = "FlushTimeout";
   const char* xmlTrThread          = "TrThread";
   const char* xmlConnTimeout       = "ConnTimeout";

   const char* xmlTrueValue         = "true";
   const char* xmlFalseValue        = "false";

   const char* xmlMcastAddr         = "McastAddr";
   const char* xmlMcastPort         = "McastPort";
   const char* xmlMcastRecv         = "McastRecv";

   const char* xmlProtocol          = "Protocol";
   const char* xmlHostName          = "HostName";
   const char* xmlFileName          = "FileName";
   const char* xmlUrlName           = "UrlName";
   const char* xmlUrlPort           = "UrlPort";

   const char* typeThread           = "dabc::Thread";
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



dabc::Object::Object(const char* name, bool owner) :
   fObjectFlags(0),
   fObjectParent(),
   fObjectName(name),
   fObjectMutex(0),
   fObjectRefCnt(0),
   fObjectChilds(0),
   fObjectBlock(0)
{
   DOUT3(("Created object %s %p", (name ? name : "---"), this));

   SetFlag(flIsOwner, owner);

   Constructor();
}


dabc::Object::Object(Object* parent, const char* name, bool owner) :
   fObjectFlags(0),
   fObjectParent(parent),
   fObjectName(name),
   fObjectMutex(0),
   fObjectRefCnt(0),
   fObjectChilds(0),
   fObjectBlock(0)
{
   SetFlag(flIsOwner, owner);

   DOUT3(("Object created %s %p", (name ? name : "---"), this));

   Constructor();
}


dabc::Object::Object(Reference parent, const char* name, bool owner) :
   fObjectFlags(0),
   fObjectParent(parent),
   fObjectName(name),
   fObjectMutex(0),
   fObjectRefCnt(0),
   fObjectChilds(0),
   fObjectBlock(0)
{
   SetFlag(flIsOwner, owner);

   DOUT3(("Object created %s %p", (name ? name : "---"), this));

   Constructor();
}


dabc::Object::Object(const ConstructorPair& pair, bool owner) :
   fObjectFlags(0),
   fObjectParent(pair.parent),
   fObjectName(pair.name),
   fObjectMutex(0),
   fObjectRefCnt(0),
   fObjectChilds(0),
   fObjectBlock(0)
{
   SetFlag(flIsOwner, owner);

   DOUT3(("Object created %s %p", GetName(), this));

   Constructor();
}


dabc::Object::~Object()
{
   Destructor();

   DOUT3(("Object destroyed %s %p", GetName(), this));
}

void dabc::Object::SetOwner(bool on)
{
   LockGuard lock(fObjectMutex);
   SetFlag(flIsOwner, on);
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

void dabc::Object::SetLogging(bool on)
{
   LockGuard lock(fObjectMutex);
   return SetFlag(flLogging, on);
}

bool dabc::Object::IsName(const char* str, int len) const
{
   if ((len==0) || (str==0)) return false;
   if (len<0) return fObjectName.compare(str) == 0;
   return ((int) fObjectName.length()==len) && (fObjectName.compare(0, len, str,len) == 0);
}

void dabc::Object::Constructor()
{
   gNumInstances++;

   // use non-recursive mutexes to see faster deadlock problems
   // FIXME: should we use recursive in final version?
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

   Mutex* m = 0;
   ReferencesVector* chlds(0);

   {
      LockGuard lock(fObjectMutex);

      if ((GetState() != stDestructor) && (fObjectRefCnt!=0)) {
         EOUT(("Object %p %s deleted not via Destroy method refcounter %u", this, GetName(), fObjectRefCnt));
      }

      SetState(stDestructor);

      if (fObjectRefCnt!=0) {
         EOUT(("!!!!!!!!!!!! Destructor called when refcounter %u obj:%s %p", fObjectRefCnt, GetName(), this));
//         Object* obj = (Object*) 29387898;
//         delete obj;
      }

      if (fObjectChilds!=0) {
         EOUT(("!!!!!!! CHILDS %u ARE NOT DELETED completely obj:%s %p", fObjectChilds->GetSize(), GetName(), this));

         if (fObjectBlock > 0)
            EOUT(("!!!! CHILDS ARE STILL BLOCKED %d!!!!!!!", fObjectBlock));

         chlds = fObjectChilds;
         fObjectChilds = 0;
      }

      m = fObjectMutex;
      fObjectMutex = 0;
   }


   // from this point on children are not seen to outer world,
   // one can destroy them one after another

   if (chlds!=0) {
      chlds->Clear();
      delete chlds;
   }

   // release reference on parent, remove from child should be already done
   fObjectParent.Release();

   // destroy mutex
   delete m; m =0;

   gNumInstances--;
}

void dabc::Object::_IncObjectRefCnt()
{
   // TODO: make such check optional

   if (fObjectMutex && !fObjectMutex->IsLocked())
      EOUT(("Mutex not locked!!!"));

   fObjectRefCnt++;
}

void dabc::Object::_DecObjectRefCnt()
{
   // TODO: make such check optional
   if (fObjectMutex && !fObjectMutex->IsLocked())
      EOUT(("Mutex not locked!!!"));

   // if (IsLogging()) DOUT0(("Dec object refcounter in 299"));
   fObjectRefCnt--;

   if ((fObjectRefCnt==0) && (GetState()!=stNormal))
      EOUT(("!!! potential problem - change release order in your program, some object may not be cleanup correctly !!!"));
}


bool dabc::Object::IncReference(bool withmutex)
{
   if (withmutex) {
      dabc::LockGuard lock(fObjectMutex);

      if (GetState() == stDestructor) {
         EOUT(("OBJ:%p %s %s Inc reference during destructor", this, GetName(), ClassName()));
         return false;
//         EOUT(("Obj:%p %s Class:%s IncReference %u inside destructor :(",
//               this, GetName(), ClassName(), fObjectRefCnt));
      }

      fObjectRefCnt++;

      if (GetFlag(flLogging))
         DOUT2(("Obj:%s %p Class:%s IncReference %u state %d",
               GetName(), this, ClassName(), fObjectRefCnt, GetState()));

      return true;
   }


   if (fObjectMutex==0) return false;

   if (!fObjectMutex->IsLocked()) {
      EOUT(("Obj:%p %s Class:%s IncReference mutex is not lock but declared so",
            this, GetName(), ClassName()));
      return false;
   }

   DOUT3(("Obj:%p IncReference2 %u", this, fObjectRefCnt));

   fObjectRefCnt++;
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
//           DOUT0(("Call DecReference for object %p %s refcnt = %d, ask_to_destroy %s do_decrement %s state %d from_thread %s", this, GetName(), fObjectRefCnt, DBOOL(ask_to_destroy), DBOOL(do_decrement), GetState(), DBOOL(from_thread)));
//      }

//      if (GetFlag(flLogging))
//         DOUT0(("Obj:%s %p  Class:%s DecReference %u destroy %s dodec %s state %d numchilds %u flags %x",
//               GetName(), this, ClassName(),
//               fObjectRefCnt, DBOOL(ask_to_destroy), DBOOL(do_decrement), GetState(),
//               (fObjectChilds ? fObjectChilds->GetSize() : 0), fObjectFlags));

      if (do_decrement) {

         if (fObjectRefCnt==0) {
            EOUT(("Reference counter is already 0"));
            throw dabc::Exception("Reference counter is 0 - cannot decrease");
            return false;
         }

         // if (IsLogging()) DOUT0(("Dec object refcounter in 389"));
         fObjectRefCnt--;
      }

      switch (GetState()) {
         case stConstructor:
            EOUT(("Object %s is not yet constructed - most probably, big failure", GetName()));
            return false;

         case stNormal:
            // if autodestroy flag specified, object will be destroyed when no more references are existing
            if (GetFlag(flAutoDestroy) && (fObjectRefCnt==0)) ask_to_destroy = true;
            if (!ask_to_destroy) return false;
            viathrd = GetFlag(flHasThread);
            break;

         // we are waiting for the thread, only thread call can unblock
         case stWaitForThread:
            if (!from_thread) {
               DOUT2(("Object %p %s tried to destroy not from thread - IGNORE", this, GetName()));
               return false;
            }
            break;

         // recursive call, ignore it
         case stDoingDestroy:
            return false;

         // if object already so far, it can be destroyed when no references remained
         case stWaitForDestructor:
            if ((fObjectRefCnt == 0) && _NoOtherReferences()) {
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
                 gObjectGarbageCollector.push_back(this);
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
      } else {
         SetState(stDoingDestroy);
      }
   }

   if (viathrd) {
      if (AskToDestroyByThread()) return false;

      // it means thread do not want to destroy us, let do it normal way
      dabc::LockGuard lock(fObjectMutex);
      SetState(stDoingDestroy);
      // thread reject destroyment, therefore refcounter can be decreased
      // if (IsLogging()) DOUT0(("Dec object refcounter in 459"));
      fObjectRefCnt--;
   }


//   if (IsLogging()) DOUT0(("Calling object cleanup"));

   // Main point of all destroyment - call cleanup in correct place
   ObjectCleanup();

//   if (IsLogging()) DOUT0(("Did object cleanup"));


   if (IsLogging())
      DOUT0(("DecRefe after cleanup %p cleanup %s mgr %p", this, DBOOL(GetFlag(flCleanup)), dabc::mgr()));
   {

      LockGuard guard(fObjectMutex);

      if (GetFlag(flCleanup) && dabc::mgr()) {
         // cleanup will be done by manager via reference
         fObjectRefCnt++;
         SetState(stWaitForDestructor);
      } else
      if ((fObjectRefCnt==0) && _NoOtherReferences()) {
         // no need to deal with manager - can call destructor immediately
         DOUT3(("Obj:%p can be destroyed", this));
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
               gObjectGarbageCollector.push_back(this);
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

   if ((fObjectRefCnt==0) && _NoOtherReferences()) {
      // no need to deal with manager - can call destructor immediately
      if (_DoDeleteItself()) {
         SetState(stWaitForDestructor);
         return false;
      }

      DOUT3(("Obj:%p can be destroyed", this));
      SetState(stDestructor);
      return true;
   }

   return false;
}

bool dabc::Object::CallDestroyFromThread()
{
   return DecReference(true, true, true);
}

void dabc::Object::DeleteThis()
{
   if (IsLogging())
      DOUT1(("OBJ:%p %s DELETETHIS cnt %u", this, GetName(), fObjectRefCnt));

   {
      LockGuard lock(fObjectMutex);

      if (GetState()!=stNormal) {
//         EOUT(("Object %p already on the way to destroyment - do not harm it", this));
         return;
      }

      // this flag force object to be cleanup via manager
      SetFlag(flCleanup, true);
   }

   Destroy(this);
}




void dabc::Object::ObjectCleanup()
{
   ReferencesVector* chlds = 0;

   int cnt = 1000000;

   while (--cnt>0) {
      dabc::LockGuard lock(fObjectMutex);
      if (GetState() != stDoingDestroy) {
         EOUT(("obj:%p name:%s class:%s Something completely wrong - cleanup in wrong state %u ", this, GetName(), ClassName(), GetState()));
      }

      if (fObjectChilds == 0) break;

      if (fObjectBlock>0) continue;

      // we deleting childs immediately
      chlds = fObjectChilds; fObjectChilds = 0;
      break;
   }

   #ifdef DABC_EXTRA_CHECKS
   if (cnt < 999990)
      DOUT0(("Object %s Retry %d time before childs were unblocked", 1000000-cnt));
   #endif

   if (cnt==0) {
      EOUT(("HARD error!!!! - For a long time fObjectBlock=%d is blocked in object %p %s", fObjectBlock, this, GetName()));
      exit(055);
   }

   // first we delete all childs !!!!
   if (chlds!=0) {
      if (IsLogging()) DOUT0(("Obj:%p %s Deleting childs %u", this, GetName(), chlds->GetSize()));
      delete chlds;
      if (IsLogging()) DOUT0(("Obj:%p %s Deleting childs done", this, GetName()));
   }

   if (IsLogging()) DOUT0(("Before remove from parent"));

   // Than we remove reference on the object from parent
   if (fObjectParent())
      fObjectParent()->RemoveChild(this);

   if (IsLogging()) DOUT0(("After remove from parent"));


   DOUT3(("Obj:%s Class:%s Finish cleanup numrefs %u", GetName(), ClassName(), NumReferences()));

}

unsigned dabc::Object::NumReferences()
{
   dabc::LockGuard lock(fObjectMutex);

   return fObjectRefCnt;
}

dabc::Reference dabc::Object::_MakeRef()
{
   return dabc::Reference(false, this);
}


bool dabc::Object::AddChild(Object* child, bool withmutex, bool setparent) throw()
{
   if (child==0) return false;

   if (child->GetParent() == 0) {
      if (setparent)
         child->fObjectParent.SetObject(this, false, withmutex);
   } else
   if (child->GetParent() != this) {
      EOUT(("Cannot move child from other parent"));
      throw dabc::Exception("Cannot move child from other parent");
      return false;
   }

   Reference ref(child);

   LockGuard guard(withmutex ? fObjectMutex : 0);

   // one can set owner flag under mutex - reference itself does not uses mutexes
   ref.SetOwner(GetFlag(flIsOwner));

   if (fObjectChilds==0) fObjectChilds = new ReferencesVector;

   fObjectChilds->Add(ref);

   return true;
}

void dabc::Object::RemoveChild(Object* child) throw()
{
   if (child==0) return;

   int cnt = 1000000;

   while (--cnt>0) {
      LockGuard guard(fObjectMutex);

      if (fObjectChilds==0) return;

      if (fObjectBlock>0) continue;

      fObjectChilds->Remove(child);
      break;
   }

   #ifdef DABC_EXTRA_CHECKS
   if (cnt < 999990)
      DOUT0(("Object %s Retry %d time before childs were unblocked", GetName(), 1000000-cnt));
   #endif

   if (cnt==0) {
      EOUT(("HARD error!!!! - For a long time fObjectBlock=%d is blocked in object %p %s", fObjectBlock, this, GetName()));
      exit(055);
   }
}

unsigned dabc::Object::NumChilds() const
{
   LockGuard guard(fObjectMutex);

   return fObjectChilds ? fObjectChilds->GetSize() : 0;
}

dabc::Object* dabc::Object::GetChild(unsigned n) const
{
   LockGuard guard(fObjectMutex);

   return fObjectChilds==0 ? 0 : fObjectChilds->GetObject(n);
}

dabc::Reference dabc::Object::GetChildRef(unsigned n) const
{
   LockGuard guard(fObjectMutex);

   Object* obj = (fObjectChilds==0) ? 0 : fObjectChilds->GetObject(n);

   if (obj==0) return Reference();

   IntGuard block(fObjectBlock);

   UnlockGuard unlock(fObjectMutex);

   return dabc::Reference(obj);

   // here will be mutex locked, fObjectBlock counter decremented and finally mutex unlocked again
}

bool dabc::Object::GetAllChildRef(ReferencesVector* vect) const
{
   if (vect==0) return false;

   LockGuard guard(fObjectMutex);

   if (fObjectChilds==0) return true;

   PointersVector ptrs;

   for (unsigned n=0; n<fObjectChilds->GetSize(); n++)
      ptrs.push_back(fObjectChilds->GetObject(n));

   IntGuard block(fObjectBlock);

   UnlockGuard unlock(fObjectMutex);

   for (unsigned n=0;n<ptrs.size();n++)
     vect->Add(Reference((Object*)ptrs[n]));

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

dabc::Reference dabc::Object::SearchForChild(Reference& ref, const char* name, bool firsttime, bool force, bool isowner) throw()
{
   if (ref()==0) return ref;

   if ((name==0) || (strlen(name)==0)) return ref;

   if (*name=='/') {
      if (firsttime) {
         while (ref()->GetParent()!=0)
            ref = Reference(ref()->GetParent());
         return SearchForChild(ref, name+1, false, force, isowner);
      } else {
         // skip all slashes in the beginning
         while (*name=='/') name++;
         if (*name==0) return ref;
      }
   }

   int len = strlen(name);

   if ((len>=2) && (name[0]=='.') && (name[1] == '.')) {
      if (len==2) return Reference(ref()->GetParent());

      if (name[2]=='/') {
         ref = Reference(ref()->GetParent());
         return SearchForChild(ref, name + 3, false, force, isowner);
      }
   }

   if ((len>=1) && (name[0]=='.')) {
      if (len==1) return ref;
      if (name[1]=='/')
         return SearchForChild(ref, name + 2, false, force, isowner);
   }

   const char* ptok = name;
   while (*ptok!=0) {
      if (*ptok == '/') break;
      ptok++;
   }

   Reference newref;

   {
      LockGuard guard(ref()->fObjectMutex);

      dabc::Object* obj = (ref()->fObjectChilds==0) ? 0 : ref()->fObjectChilds->FindObject(name, ptok-name);

      // create new object under parent mutex - no one can create double entries simultaneously
      if ((obj==0) && force) {
         obj = new Object(0, std::string(name, ptok-name).c_str(), isowner);
         ref()->AddChild(obj, false);
      }

      if (obj!=0) {
         IntGuard block(ref()->fObjectBlock);

         UnlockGuard unlock(ref()->fObjectMutex);

         // make reference outside parent mutex
         newref = dabc::Reference(obj);
      }
   }

   ref = newref;

   if (*ptok==0) return ref;

   return SearchForChild(ref, ptok+1, false, force, isowner);
}


dabc::Reference dabc::Object::GetFolder(const char* name, bool force, bool isowner) throw()
{
   Reference ref(this);

   return SearchForChild(ref, name, true, force, isowner)();
}

dabc::Reference dabc::Object::PutChild(Object* obj, bool delduplicate)
{
   Reference newref;
   if (obj==0) return newref;

   Object* oldobj = 0;

   {
      LockGuard guard(ObjectMutex());

      if (fObjectChilds)
         oldobj = fObjectChilds->FindObject(obj->GetName());

      // if object was not exists, add child create new object under parent mutex - no one can create double entries simultaneously
      if (oldobj==0) { oldobj = obj; AddChild(obj, false); }

      IntGuard block(fObjectBlock);

      UnlockGuard unlock(fObjectMutex);

      // make reference outside parent mutex
      newref << dabc::Reference(oldobj);
   }

   // instance will be automatically deleted when object with givven name already exists
   if ((oldobj!=obj) && delduplicate) dabc::Object::Destroy(obj);

   return newref;
}


void dabc::Object::DeleteChilds(const std::string& exclude_mask)
{
   ReferencesVector del_vect;

   int cnt = 1000000;

   while (--cnt>0) {
      LockGuard guard(fObjectMutex);

      // nothing to do
      if (fObjectChilds==0) return;

      if (fObjectBlock>0) continue;

      unsigned n = fObjectChilds->GetSize();

      while (n-->0) {
         Object* obj = fObjectChilds->GetObject(n);

         if (!exclude_mask.empty() && obj->IsNameMatch(exclude_mask)) continue;

         del_vect.Add(fObjectChilds->TakeRef(n));
      }

      if (fObjectChilds->GetSize()==0) { delete fObjectChilds; fObjectChilds = 0; }

      break;
   }

   #ifdef DABC_EXTRA_CHECKS
   if (cnt < 999990)
      DOUT0(("Object %s Retry %d times before childs were unblocked", 1000000-cnt));
   #endif

   if (cnt==0) {
      EOUT(("HARD error!!!! - For a long time fObjectBlock=%d is blocked in object %p %s", fObjectBlock, this, GetName()));
      exit(055);
   }

   if (IsLogging())
      DOUT1(("Obj:%s Deleting childs:%u", GetName(), del_vect.GetSize()));

   if (IsLogging())
      while (del_vect.GetSize()>0) {
         Reference ref = del_vect.TakeLast();
         DOUT1(("    Del Child %s owner %s", ref.GetName(), DBOOL(ref.IsOwner())));
      }

   del_vect.Clear();

/*
   if (recursive) {

      GetAllChildRef(&del_vect);

      for(unsigned n=0; n<del_vect.GetSize(); n++)
         del_vect.GetObject(n)->DeleteChilds();

      del_vect.Clear();
   }
*/
}

void dabc::Object::SetName(const char* name)
{
   LockGuard guard(fObjectMutex);

   if (fObjectRefCnt>0) {
      EOUT(("Cannot change object name when reference counter %d is not zero!!!", fObjectRefCnt));
      throw dabc::Exception("Cannot change object name when refcounter is not zero");
   }

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
 *  should happen from this thread. In this case AskToDestroyByThread() method will be called.
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
   if (obj==0) return;

   if (obj->DecReference(true, false))
      delete obj;
}

bool dabc::Object::Find(ConfigIO &cfg)
{
   if (GetParent()==0) return false;

   return cfg.FindItem(GetName());
}

dabc::Object::ConstructorPair dabc::Object::MakePair(Reference prnt, const char* fullname, bool withmanager)
{
   if ((fullname==0) && prnt.null())
      return ConstructorPair();

   if (fullname==0) fullname = "---";
   bool isrootfolder = false;
   while (*fullname=='/') {
      isrootfolder = true;
      fullname++;
   }

   if (prnt.null() && withmanager) {
      if (!isrootfolder) prnt = dabc::mgr.GetAppFolder(true);
      if (prnt.null()) prnt = dabc::mgr;
   }

   const char* slash = strrchr(fullname, '/');

   ConstructorPair pair;

   if (slash!=0) {
      pair.name = slash+1;
      std::string path = std::string(fullname, slash - fullname);
      if (!prnt.null())
         prnt = prnt()->FindChildRef(path.c_str(), true);
   } else {
      pair.name = fullname;
   }

   pair.parent = prnt;

   return pair;
}

dabc::Object::ConstructorPair dabc::Object::MakePair(Object* prnt, const char* fullname, bool withmanager)
{
   return dabc::Object::MakePair(Reference(prnt), fullname, withmanager);
}

dabc::Object::ConstructorPair dabc::Object::MakePair(const char* fullname, bool withmanager)
{
   return dabc::Object::MakePair(Reference(), fullname, withmanager);
}


bool dabc::Object::IsParent(Object* obj) const
{
   if (obj==0) return false;

   Object* prnt = GetParent();

   while (prnt!=0) {
      if (prnt==obj) return true;
      prnt = prnt->GetParent();
   }
   return false;
}


bool dabc::Object::IsNameMatch(const std::string& mask) const
{
   return NameMatch(fObjectName, mask);
}

bool dabc::Object::NameMatch(const std::string& name, const std::string& mask)
{
   if ((mask.length()==0) || (name.length()==0))
      return name.length()==mask.length();

   size_t lastsepar(0), separ = mask.find_first_of(';',lastsepar);

   if (separ!=std::string::npos) {
      do {




      } while (lastsepar!=std::string::npos);
   }


   if (mask.find_first_of("*?")==mask.npos) return name == mask;

   if (mask=="*") return true;

   return fnmatch(mask.c_str(), name.c_str(), FNM_NOESCAPE)==0;
}

bool dabc::Object::NameMatchM(const std::string& name, const std::string& mask)
{
   size_t separ = mask.find_first_of(':');

   if (separ==std::string::npos) return NameMatch(name, mask);

   size_t lastsepar = 0;

   do {

      std::string submask;

      if (separ==std::string::npos)
         submask = mask.substr(lastsepar);
      else
      if (separ>lastsepar)
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


void dabc::Object::FillFullName(std::string &fullname, Object* upto) const
{
   if ((GetParent()!=0) && (GetParent() != upto)) {
      GetParent()->FillFullName(fullname, upto);
      fullname.append("/");
   }

   if (GetParent()==0) fullname.append("/");
   fullname.append(GetName());
}


std::string dabc::Object::ItemName(bool compact) const
{
   std::string res;
   if (dabc::mgr.null()) {
      FillFullName(res, 0);
   } else
      dabc::mgr()->FillItemName(this, res, compact);
   return res;
}

// ============================================================================
// FIXME: old code, should be adjusted


void dabc::Object::FillInfo(std::string& info)
{
   dabc::formats(info, "Object: %s", GetName());
}

void dabc::Object::Print(int lvl)
{
   LockGuard guard(ObjectMutex());
   dabc::lgr()->Debug(lvl, "file", 1, "func", dabc::format("Object: %s Class:%s", GetName(), ClassName()).c_str());
}


void dabc::Object::InspectGarbageCollector()
{
#ifdef DABC_EXTRA_CHECKS
   LockGuard lock(gObjectGarbageMutex);

   DOUT0(("GarbageCollector: there are %u objects in collector now", gObjectGarbageCollector.size()));

   for (unsigned n=0;n<gObjectGarbageCollector.size();n++) {
      Object* obj = (Object*) gObjectGarbageCollector.at(n);
      DOUT0(("   obj:%p name:%s class:%s refcnt:%u", obj, obj->GetName(), obj->ClassName(), obj->fObjectRefCnt));
   }

#endif

}
