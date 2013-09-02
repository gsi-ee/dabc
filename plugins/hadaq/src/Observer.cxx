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

hadaq::Observer::Observer(const std::string& name) :
   dabc::Worker(MakePair(name)),
   fEntryMutex()
{
   fEnabled = Cfg(hadaq::xmlObserverEnabled).AsBool(false);
   if(!fEnabled) {
      DOUT0("hadaq shmem observer is disabled.");
      return;
   }

   fNodeId = dabc::mgr.NodeId()+1; // hades eb ids start with 1

   // we use here mask for evtbuild and netmem prefixes only
   std::string maskn = dabc::format("%s_*",hadaq::NetmemPrefix);
   std::string maske = dabc::format("%s_*",hadaq::EvtbuildPrefix);

   RegisterForParameterEvent(maskn, false);
   RegisterForParameterEvent(maske, false);

   std::string netname=dabc::format("daq_netmem%d", fNodeId);
   fNetmemWorker=::Worker_initBegin(netname.c_str(), hadaq::sigHandler, 0, 0);
   if (fNetmemWorker == 0) {
      EOUT("Could not create netmem worker %s !!!!", netname.c_str());
   } else {
      ::Worker_initEnd(fNetmemWorker); // this will just add pid as default entry.
   }
   std::string evtname = dabc::format("daq_evtbuild%d", fNodeId);
   fEvtbuildWorker = ::Worker_initBegin(evtname.c_str(), hadaq::sigHandler, 0, 0);
   if (fEvtbuildWorker == 0) {
      EOUT("Could not create evtbuild worker %s !!!!", evtname.c_str());
   } else {
      ::Worker_initEnd(fEvtbuildWorker); // this will just add pid as default entry.
   }

   fFlushTimeout=1.0;

   DOUT0("############ Creating hadaq observer with shmems %s and %s ##########",netname.c_str(), evtname.c_str());
}

hadaq::Observer::~Observer()
{
   if(!fEnabled) return;
   DOUT0("############# Destroy SHMEM observer #############");
   ::Worker_fini(fEvtbuildWorker);
   ::Worker_fini(fNetmemWorker);
}


double hadaq::Observer::ProcessTimeout(double lastdiff)
{
   DOUT5("###hadaq::Observer::ProcessTimeout");
//   return 1.0;

   // TODO: lock updating the  variables during processparameter event
    //dabc::LockGuard guard(fEntryMutex);
//      ShmEntriesList::iterator iter = fEntries.begin();
//      while (iter!=fEntries.end()) {
//         (*iter)->UpdateParameter();
//      }
// NOTE the above will lead to an overflowing with parameter update events. need to avoid circular signalling here

   // for the moment, we will just update the one value we need for hades eventbuilding:
   std::string netname=dabc::format("daq_evtbuild%d", fNodeId);
      hadaq::ShmEntry* entry = FindEntry("runId",netname);
      if(entry){
         entry->UpdateParameter();
         //std::cout <<"updated runid parameter with "<<entry->GetValue() << std::endl;
      }

return 1.0;
}



bool hadaq::Observer::CreateShmEntry(const std::string& parname)
{
   //dabc::LockGuard guard(fEntryMutex);

   std::string shmemname=ShmName(parname);
   std::string statsname=ReducedName(parname);
   hadaq::ShmEntry* entry = FindEntry(statsname,shmemname);



   dabc::Parameter par = dabc::mgr.FindPar(parname);

// NOTE/TODO: parameters exported from transport workers are not found under their pathname here
// Name is e.g. "//Input3/Netmem_pktsReceived3"
// Names from Combiner module are found "Combiner/Evtbuild_trigNr1"
//   if (par.null()) {
//      EOUT("Warning - Did not find parameter %s !!!!", parname.c_str());
//      //return false;
//   }


   if (entry==0) {
      DOUT3("Create new entry for parameter %s", parname.c_str());
      ::Worker* my=0;
      if(parname.find(hadaq::NetmemPrefix)!= std::string::npos){
         DOUT3("Use netmem:");
         my=fNetmemWorker;

      }
      else if(parname.find(hadaq::EvtbuildPrefix)!= std::string::npos){
         DOUT3("Use evtbuild:");
         my=fEvtbuildWorker;
      }
      if(my==0)
         {
            EOUT("Worker for shmem %s is zero!!!!", shmemname.c_str());
            return false;
         }
      entry = new ShmEntry(statsname, shmemname,my,par);
      fEntries.push_back(entry);
   }

   if(!par.null())
      entry->UpdateValue(par.Value().AsStr());


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
   //dabc::LockGuard guard(fEntryMutex);
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
   //dabc::LockGuard guard(fEntryMutex);
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


//if(parname=="Combiner/Evtbuild_runId")
//   {
//      DOUT0("Get event %d par %s value %s", evnt.EventId(), parname.c_str(), evnt.ParValue().c_str());
//   }

// we can not activate timeout in constructor, need to defer it here. todo for dabc framework?
   static bool firsttime = true;
   if (firsttime && fFlushTimeout > 0.) {
      if (!ActivateTimeout(fFlushTimeout))
         EOUT("%s could not activate timeout of %f s",GetName(),fFlushTimeout);
      firsttime = false;
   }



   switch (evnt.EventId()) {
      case dabc::parCreated: {
         CreateShmEntry(parname);
         break;
      }

      case dabc::parModified: {
           //dabc::LockGuard guard(fEntryMutex);
           hadaq::ShmEntry* entry = FindEntry(parname);
         if (entry==0) {
            DOUT0("NEVER COME HERE: Modified event for non-known parameter %s !!!!", parname.c_str());
            CreateShmEntry(parname);
            return;
         }
         entry->UpdateValue(evnt.ParValue());


         break;
      }

      case dabc::parDestroy: {
         //dabc::LockGuard guard(fEntryMutex);
         hadaq::ShmEntry* entry = FindEntry(parname);
         if (entry!=0) {
            RemoveEntry(entry);
            delete entry;
         }

         break;
      }
   }

//   DOUT0("Did event %d", evnt.EventId());
}

int hadaq::Observer::Args_prefixCode(const char* prefix)
{
   char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
   int code = 0;
   int i, j;

   /* Loop over prefix */
   for (j = 0; j < 2; j++) {

      /* Loop over letters of Alphabet */
      for (i = 0; i < 26; i++) {

         if (alphabet[i] == prefix[j]) {

            /*
             *  Build prefix code:
             *  add 1 to the code to get rid of zero's
             *  in case of 'aa' prefix.
             */
            code = code * 100 + i + 1;
         }
      }
   }

   return code;
}



void  hadaq::sigHandler(int sig)
{
   DOUT0("hadaq Observer caught signal %d", sig);
   // following is copy of dabc_exe dabc_CtrlCHandler
   // probably use this directly?
   static int SigCnt=0;
   SigCnt++;

     if ((SigCnt>2) || (dabc::mgr()==0)) {
        EOUT("hadaq Observer Force application exit");
        dabc::lgr()->CloseFile();
        exit(0);
     }

     dabc::mgr()->ProcessCtrlCSignal();

}



