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

#include "dabc/Command.h"

#include <stdlib.h>

#include "dabc/Manager.h"
#include "dabc/Worker.h"
#include "dabc/Url.h"
#include "dabc/timing.h"
#include "dabc/logging.h"
#include "dabc/Exception.h"
#include "dabc/threads.h"
#include "dabc/XmlEngine.h"


dabc::CommandContainer::CommandContainer(const char* name) :
   RecordContainer(name),
   fCallers(),
   fTimeout()
{
   // object will be destroy as long no references are existing
   SetFlag(flAutoDestroy, true);

   DOUT3(("CMD:%p name %s created", this, GetName()));
}


dabc::CommandContainer::~CommandContainer()
{

   if (fCallers.size()>0)
      EOUT(("Non empty callers list in cmd %s !!!!!!!!!!!!!!", GetName()));

   fCallers.clear();


   // check if reference is remaining !!!

   std::string field;

   do {
      field = FindField("##REF##*");
      if (!field.empty()) {
         EOUT(("Reference %s not cleared correctly", field.c_str()));

         const char* value = GetField(field);
         if (value!=0) dabc::Command::MakeRef(value).Release();

         // TODO: release of remaining reference
         SetField(field, 0, 0);
      }

   } while (!field.empty());


   DOUT3(("CMD:%p name %s deleted", this, GetName()));
}


const std::string dabc::CommandContainer::DefaultFiledName() const
{
   return dabc::Command::ResultParName();
}



dabc::Command::Command(const std::string& name) throw()
{
   if (!name.empty()) {
      SetObject(new dabc::CommandContainer(name.c_str()));
      SetTransient(false);
   }
}

bool dabc::Command::CreateContainer(const std::string& name)
{
   SetObject(new dabc::CommandContainer(name.c_str()), false);
   SetTransient(false);
   return true;
}


void dabc::Command::AddCaller(Reference worker, bool* exe_ready)
{
   CommandContainer* cont = (CommandContainer*) GetObject();
   if (cont==0) return;

   LockGuard lock(ObjectMutex());

   cont->fCallers.push_back(CommandContainer::CallerRec(worker, exe_ready));
}

void dabc::Command::RemoveCaller(Worker* worker, bool* exe_ready)
{
   CommandContainer* cont = (CommandContainer*) GetObject();
   if (cont==0) return;

   LockGuard lock(ObjectMutex());

   CommandContainer::CallersList::iterator iter = cont->fCallers.begin();

   while (iter != cont->fCallers.end()) {

      if ((iter->worker == worker) && ((exe_ready==0) || (iter->exe_ready==exe_ready)))
         cont->fCallers.erase(iter++);
      else
         iter++;
   }
}

bool dabc::Command::IsLastCallerSync()
{
   CommandContainer* cont = (CommandContainer*) GetObject();
   if (cont==0) return false;

   LockGuard lock(ObjectMutex());

   if ((cont->fCallers.size()==0)) return false;

   return cont->fCallers.back().exe_ready != 0;
}

dabc::Command& dabc::Command::SetTimeout(double tm)
{
   CommandContainer* cont = (CommandContainer*) GetObject();
   if (cont!=0) {
      LockGuard lock(ObjectMutex());
      if (tm<=0.)
         cont->fTimeout.Reset();
      else {
         cont->fTimeout.GetNow();
         cont->fTimeout += tm;
      }
   }

   return *this;
}

bool dabc::Command::IsTimeoutSet() const
{
   CommandContainer* cont = (CommandContainer*) GetObject();
   if (cont==0) return false;
   LockGuard lock(ObjectMutex());
   return !cont->fTimeout.null();
}

double dabc::Command::TimeTillTimeout() const
{
   CommandContainer* cont = (CommandContainer*) GetObject();
   if (cont==0) return -1;

   LockGuard lock(ObjectMutex());
   if (cont->fTimeout.null()) return -1;

   TimeStamp now = TimeStamp::Now();

   return cont->fTimeout < now ? 0. : cont->fTimeout - now;
}

int dabc::Command::GetPriority() const
{
   return GetInt(PriorityParName(), Worker::priorityDefault);
}

void dabc::Command::SetPtr(const std::string& name, void* p)
{
   char buf[100];
   sprintf(buf,"%p",p);
   SetField(name, buf, "pointer");
}

void* dabc::Command::GetPtr(const std::string& name, void* deflt) const
{
   const char* val = GetField(name);
   if (val==0) return deflt;

   void* p = 0;
   int res = sscanf(val,"%p", &p);
   return res>0 ? p : deflt;
}

void dabc::Command::SetRef(const std::string& name, Reference ref)
{
   char buf[100];

   if (ref.ConvertToString(buf,sizeof(buf)))
      SetStr(FORMAT(("##REF##%s", name.c_str())), buf);
}

dabc::Reference dabc::Command::MakeRef(const std::string& buf)
{
   return Reference(buf.c_str(), buf.length());
}


dabc::Reference dabc::Command::GetRef(const std::string& name)
{
   std::string field = dabc::format("##REF##%s", name.c_str());
   std::string value = GetStdStr(field.c_str());
   RemoveField(field);

   return Reference(value.c_str(), value.length());
}


void dabc::Command::AddValuesFrom(const dabc::Command& cmd, bool canoverwrite)
{
   Record::AddFieldsFrom(cmd, canoverwrite);
}

void dabc::Command::SaveToString(std::string& v, bool compact)
{
   v = SaveToXml(compact);
}

bool dabc::Command::ReadFromString(const std::string& v)
{
   return ReadFromXml(v);
}

void dabc::Command::Print(int lvl, const char* from) const
{
   Record::Print(lvl, from);
}


void dabc::Command::Release()
{
   dabc::Record::Release();
}

void dabc::Command::Cancel()
{
   SetInt("_canceled_",1);
   dabc::Record::Release();
}

void dabc::Command::Reply(int res)
{
   if (res>=0) SetResult(res);

   bool process = false;

   do {

      CommandContainer* cont = (CommandContainer*) GetObject();
      if (cont==0) return;

      process = false;
      CommandContainer::CallerRec rec;

      {
         LockGuard lock(ObjectMutex());

         if (cont->fCallers.size()==0) break;

         DOUT5(("Cmd %s Finalize callers %u", cont->GetName(), cont->fCallers.size()));

         rec = cont->fCallers.back();
         cont->fCallers.pop_back();
         process = true;
      }

      Worker* worker = (Worker*) rec.worker();

      if (process && worker) {
         DOUT3(("Call GetCommandReply worker:%p cmd:%p", worker, cont));
         process = worker->GetCommandReply(*this, rec.exe_ready);
         if (!process) { EOUT(("AAAAAAAAAAAAAAAAAAAAAAAA Problem with cmd %s", GetName())); }
      }
   } while (!process);

   DOUT3(("Command %p process %s", GetObject(), DBOOL(process)));

   // in any case release reference at the end
   Release();
}

dabc::Command& dabc::Command::SetReceiver(const std::string& itemname)
{
   // we set receiver parameter to be able identify receiver for the command

   SetStr(ReceiverParName(), itemname);

   return *this;
}

dabc::Command& dabc::Command::SetReceiver(int nodeid, const std::string& itemname)
{
   if (nodeid<0) return SetReceiver(itemname);

   return SetReceiver(Url::ComposeItemName(nodeid, itemname.c_str()));
}

dabc::Command& dabc::Command::SetReceiver(int nodeid, Object* rcv)
{
   if (rcv==0) return SetReceiver(nodeid, "");

   std::string s = rcv->ItemName();

   if (dynamic_cast<Worker*>(rcv)==0)
      EOUT(("Object %s cannot be used to receive commands", s.c_str()));

   return SetReceiver(nodeid, s);
}

dabc::Command& dabc::Command::SetReceiver(Object* rcv)
{
   return SetReceiver(-1, rcv);
}
