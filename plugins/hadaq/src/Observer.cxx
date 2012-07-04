#include "hadaq/Observer.h"
#include "hadaq/ShmEntry.h"
#include "hadaq/HadaqTypeDefs.h"

extern "C" {
#include "hadaq/worker.h"
}

#include "dabc/Manager.h"
#include "dabc/Configuration.h"

#include <string>
#include <iostream>
#include <signal.h>
#include <stdlib.h>

hadaq::Observer::Observer(const char* name) :
   dabc::Worker(MakePair(name))
{
   fEnabled = Cfg(hadaq::xmlObserverEnabled).AsBool(false);
   if(!fEnabled)
      {
         DOUT0(("hadaq shmem observer is disabled."));
         return;
      }
   fNodeId = dabc::mgr()->NodeId()+1; // hades eb ids start with 1
   //std::string nodename = dabc::mgr()->cfg()->NodeName(fNodeid);

   // Default, all parameter events are registered
   //std::string mask = Cfg("mask").AsStdStr("*");

   // if (mask.empty()) mask = "*";

   // we use here mask for evtbuild and netmem prefixes only
   std::string maskn=dabc::format("%s_*",hadaq::NetmemPrefix);
   std::string maske=dabc::format("%s_*",hadaq::EvtbuildPrefix);

   RegisterForParameterEvent(maskn, false);
   RegisterForParameterEvent(maske, false);



   // TODO:  initialize different sharedmem for daq_evtbuild and daq_netmem
   // probably we already create the expected worker entries here?

   std::string netname=dabc::format("daq_netmem%d", fNodeId);
   fNetmemWorker=::Worker_initBegin(netname.c_str(), hadaq::sigHandler, 0, 0);
   if (fNetmemWorker == 0) {
      EOUT(("Could not create netmem worker %s !!!!", netname.c_str()));
   } else {
      ::Worker_initEnd(fNetmemWorker); // this will just add pid as default entry.
   }
   std::string evtname = dabc::format("daq_evtbuild%d", fNodeId);
   fEvtbuildWorker = ::Worker_initBegin(evtname.c_str(), hadaq::sigHandler, 0, 0);
   if (fEvtbuildWorker == 0) {
      EOUT(("Could not create evtbuild worker %s !!!!", evtname.c_str()));
   } else {
      ::Worker_initEnd(fEvtbuildWorker); // this will just add pid as default entry.
   }

   DOUT0(("############ Creating hadaq observer with shmems %s and %s ##########",netname.c_str(), evtname.c_str()));

}

//void hadaq::Observer::ExtendRegistrationFor(const std::string& mask, const std::string& name)
//{
//   if (name.empty()) return;
//
//   if (mask.empty() || !dabc::Object::NameMatch(name, mask))
//      RegisterForParameterEvent(name, false);
//}




hadaq::Observer::~Observer()
{
   if(!fEnabled) return;
   DOUT0(("############# Destroy SHMEM observer #############"));
   ::Worker_fini(fEvtbuildWorker);
   ::Worker_fini(fNetmemWorker);

}

bool hadaq::Observer::CreateShmEntry(const std::string& parname)
{
   std::string shmemname=ShmName(parname);
   std::string statsname=ReducedName(parname);
   hadaq::ShmEntry* entry = FindEntry(statsname,shmemname);

   dabc::Parameter par = dabc::mgr.FindPar(parname);

   if (par.null()) {
      EOUT(("Did not find parameter %s !!!!", parname.c_str()));
      return false;
   }


   if (entry==0) {
      DOUT0(("Create new entry for parameter %s", parname.c_str()));
      ::Worker* my=0;
      if(parname.find(hadaq::NetmemPrefix)!= std::string::npos){
         DOUT0(("Use netmem:"));
         my=fNetmemWorker;

      }
      else if(parname.find(hadaq::EvtbuildPrefix)!= std::string::npos){
         DOUT0(("Use evtbuild:"));
         my=fEvtbuildWorker;
      }
      if(my==0)
         {
            EOUT(("Worker for shmem %s is zero!!!!", shmemname.c_str()));
            return false;
         }
      entry = new ShmEntry(statsname, shmemname,my);
      fEntries.push_back(entry);
   }
   entry->UpdateValue(par.AsStdStr());


   return true;
}

std::string hadaq::Observer::ReducedName(const std::string& dabcname)
{
   std::string res="";
   size_t sep;
   sep=dabcname.find("_");
    if (sep!=std::string::npos)
       {
          res=dabcname.substr (sep+1,std::string::npos);
          //std::cout <<"ReducedName:" << res << std::endl;
       }
   return res;
}

std::string hadaq::Observer::ShmName(const std::string& dabcname)
{
   std::string res = "";
   size_t sep;

   sep = dabcname.find("_");
   if (sep != std::string::npos) {
      res = dabcname.substr(0, sep);
      //std::cout << "ShmName:" << res << std::endl;
   }
   // our prefix is behind dabc path, check if it is contained:
   if (res.find(hadaq::EvtbuildPrefix)!= std::string::npos)
      res = "daq_evtbuild";
   else if (res.find(hadaq::NetmemPrefix)!= std::string::npos)
      res = "daq_netmem";
   else
      res = "unknown";

   std::string retval=std::string(dabc::format("%s%d", res.c_str(), fNodeId));
   //std::cout <<"ShmName:" << retval << std::endl;
   return retval;
}


hadaq::ShmEntry* hadaq::Observer::FindEntry(const std::string& statsname, const std::string& shmemname)
{
   ShmEntriesList::iterator iter = fEntries.begin();
   while (iter!=fEntries.end()) {
      if (((*iter)->IsShmemName(shmemname)) && (*iter)->IsStatsName(statsname)) return *iter;
      iter++;
   }
   return 0;
}

void hadaq::Observer::RemoveEntry(ShmEntry* entry)
{
   if (entry==0) return;
   ShmEntriesList::iterator iter = fEntries.begin();
   while (iter!=fEntries.end()) {
      if (*iter==entry) {
         fEntries.erase(iter);
         return;
      }
      iter++;
   }
}




void hadaq::Observer::ProcessParameterEvent(const dabc::ParameterEvent& evnt)
{
   if(!fEnabled) return;

   std::string parname = evnt.ParName();

//   DOUT0(("Get event %d par %s entry %p value %s", evnt.EventId(), parname.c_str(), entry, evnt.ParValue().c_str()));

   switch (evnt.EventId()) {
      case dabc::parCreated: {
         CreateShmEntry(parname);
         break;
      }

      case dabc::parModified: {
           hadaq::ShmEntry* entry = FindEntry(parname);
         if (entry==0) {
            DOUT0(("NEVER COME HERE: Modified event for non-known parameter %s !!!!", parname.c_str()));
            CreateShmEntry(parname);
            return;
         }
         entry->UpdateValue(evnt.ParValue());


         break;
      }

      case dabc::parDestroy: {

         hadaq::ShmEntry* entry = FindEntry(parname);
         if (entry!=0) {
            RemoveEntry(entry);
            delete entry;
         }

         break;
      }
   }

//   DOUT0(("Did event %d", evnt.EventId()));
}


void  hadaq::sigHandler(int sig)
{
   DOUT0(("hadaq Observer caught signal %d", sig));
   // following is copy of dabc_exe dabc_CtrlCHandler
   // probably use this directly?
   static int SigCnt=0;
   SigCnt++;

     if ((SigCnt>2) || (dabc::mgr()==0)) {
        EOUT(("hadaq Observer Force application exit"));
        dabc::lgr()->CloseFile();
        exit(0);
     }

     dabc::mgr()->ProcessCtrlCSignal();

}



