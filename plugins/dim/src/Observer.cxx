#include "dimc/Observer.h"

#include "dimc/ServiceEntry.h"

#include "dabc/Manager.h"
#include "dabc/Configuration.h"
#include "dabc/SocketDevice.h"

dimc::Observer::Observer(const std::string& name) :
   dabc::Worker(MakePair(name)),
   ::DimServer(),
   fEntries()
{
   DimServer::autoStartOff(); // avoid problems with concurrent update/registering before dimserver start

   int nodeid = dabc::mgr()->NodeId();
   std::string nodename = dabc::mgr()->cfg()->NodeName(nodeid);
   if (nodename.empty() || (nodename=="localhost"))
      nodename = dabc::SocketDevice::GetLocalHost(true);
   if (nodename.empty()) nodename = "dabchost";

   fDimPrefix = dabc::format("DABC/%s:%d/%s/",  nodename.c_str(), nodeid, dabc::mgr.GetName());

   fDNS = Cfg("dns").AsStdStr();
   if (fDNS.empty()) fDNS = dabc::mgr()->cfg()->ResolveEnv("${DIM_DNS_NODE}");

   if (fDNS.empty()) {
      EOUT("!!! DIM_DNS_NODE is not specified - try to use localhost");
      fDNS = "localhost";
   }

   fDNSport = Cfg("port").AsUInt(2505);

   // Default, all parameter events are registered
   std::string mask = Cfg("mask").AsStdStr("*");

   // if (mask.empty()) mask = "*";

   fERateName = Cfg("ERate").AsStdStr();
   fDRateName = Cfg("DRate").AsStdStr();
   fInfoName = Cfg("Info").AsStdStr();
   fInfo2Name = Cfg("Info2").AsStdStr();
   fInfo3Name = Cfg("Info3").AsStdStr();

   RegisterForParameterEvent(mask, false);

   ExtendRegistrationFor(mask, dabc::Application::StateParName());
   ExtendRegistrationFor(mask, fERateName);
   ExtendRegistrationFor(mask, fDRateName);
   ExtendRegistrationFor(mask, fInfoName);
   ExtendRegistrationFor(mask, fInfo2Name);
   ExtendRegistrationFor(mask, fInfo3Name);

   DOUT0("############ Creating dim observer dns:%s:%u mask:%s ##########", fDNS.c_str(), fDNSport, mask.c_str());

   StartDimServer(dabc::format("DABC/%s:%d",  nodename.c_str(), nodeid), fDNS, fDNSport);

   // create RunStatus record, which is recognized by gui
//   ServiceEntry* entry = new ServiceEntry(dabc::Application::StateParName(), fDimPrefix + RunStatusName());
//   entry->UpdateService(0, ServiceEntry::kindStatus);
   // entry->UpdateValue("Init");
//   fEntries.push_back(entry);
}

void dimc::Observer::ExtendRegistrationFor(const std::string& mask, const std::string& name)
{
   if (name.empty()) return;

   if (mask.empty() || !dabc::Object::NameMatch(name, mask))
      RegisterForParameterEvent(name, false);
}


void dimc::Observer::StartDimServer(const std::string& name, const std::string& dnsnode, unsigned int dnsport)
{
   ::DimServer::addClientExitHandler(this);
   ::DimServer::addExitHandler(this);
   ::DimServer::addErrorHandler(this);

   if ((dnsport!=0) && !dnsnode.empty()) {
      ::DimServer::setDnsNode(dnsnode.c_str(), dnsport);
      DOUT0("Starting DIM server of name %s for dns %s:%d", name.c_str(), dnsnode.c_str(), dnsport);
   } else {
      DOUT0("Starting DIM server of name %s for DIM_DNS_NODE  %s:%d", name.c_str(), DimServer::getDnsNode(), DimServer::getDnsPort());
   }
   ::DimServer::start(name.c_str());
   ::DimServer::autoStartOn(); // any new service will be started afterwards
}



void dimc::Observer::StopDimServer()
{
   ::DimServer::stop();

}


dimc::Observer::~Observer()
{
   while (fEntries.size()>0) {
      ServiceEntry* entry = fEntries.front();
      fEntries.pop_front();
      delete entry;
   }

   DOUT0("############# Destroy dim observer #############");

   StopDimServer();
}

bool dimc::Observer::CreateEntryForParameter(const std::string& parname, const std::string& altername)
{
   dimc::ServiceEntry* entry = FindEntry(parname);

   dabc::Parameter par = dabc::mgr.FindPar(parname);

   if (par.null()) {
      EOUT("Did not find parameter %s !!!!", parname.c_str());
      return false;
   }

   std::string dimname = BuildDIMName(par, parname);
   if (!altername.empty()) dimname = fDimPrefix + altername;

   if (entry==0) {
      DOUT2("Create new entry for parameter %s", parname.c_str());
      entry = new ServiceEntry(parname, dimname);
      fEntries.push_back(entry);
   }

   if (!entry->UpdateService(par)) {
      EOUT("Cannot create service for parameter %s", parname.c_str());
      RemoveEntry(entry);
      delete entry;
      return false;
   }

   return true;
}


void dimc::Observer::ProcessParameterEvent(const dabc::ParameterEvent& evnt)
{
   std::string parname = evnt.ParName();

   std::string dimname;
   if (parname==fDRateName) dimname = "DRate"; else
   if (parname==fERateName) dimname = "ERate"; else
   if (parname==fInfoName) dimname = "Info"; else
   if (parname==fInfo2Name) dimname = "Info2"; else
   if (parname==fInfo3Name) dimname = "Info3";

   dimc::ServiceEntry* entry = FindEntry(parname);

//   DOUT0("Get event %d par %s entry %p value %s", evnt.EventId(), parname.c_str(), entry, evnt.ParValue().c_str());

   switch (evnt.EventId()) {
      case dabc::parCreated: {
         CreateEntryForParameter(parname, dimname);
         break;
      }

      case dabc::parModified: {
         if (entry==0) {
            DOUT2("Modified event for non-known parameter %s !!!!", parname.c_str());
            CreateEntryForParameter(parname, dimname);
            return;
         }

         // this may indicate changes in parameter, one could recreate service for the entry
         if (evnt.AttrModified())
            entry->UpdateService(dabc::mgr.FindPar(parname));
         else
            entry->UpdateValue(evnt.ParValue());

         break;
      }

      case dabc::parDestroy: {
         if (entry!=0) {
            RemoveEntry(entry);
            delete entry;
         }

         break;
      }
   }

//   DOUT0("Did event %d", evnt.EventId());
}

void dimc::Observer::errorHandler(int severity, int code, char *msg)
{
}

void dimc::Observer::clientExitHandler()
{
}

void dimc::Observer::exitHandler( int code )
{
   DOUT0("Process dim exit code %d", code);
   dabc::mgr.StopApplication();
}

dimc::ServiceEntry* dimc::Observer::FindEntry(const std::string& name)
{
   ServiceEntriesList::iterator iter = fEntries.begin();
   while (iter!=fEntries.end()) {
      if ((*iter)->IsDabcName(name)) return *iter;
      iter++;
   }
   return 0;
}

void dimc::Observer::RemoveEntry(ServiceEntry* entry)
{
   if (entry==0) return;
   ServiceEntriesList::iterator iter = fEntries.begin();
   while (iter!=fEntries.end()) {
      if (*iter==entry) {
         fEntries.erase(iter);
         return;
      }
      iter++;
   }
}

std::string dimc::Observer::BuildDIMName(const dabc::Parameter& par, const std::string& fullname)
{
   std::string res;

   if (fullname==dabc::Application::StateParName()) return fDimPrefix + RunStatusName();

   if (par.null()) return res;
   res = par.GetName();

   Object* prnt = par.GetParent();
   while ((prnt!=0) && (prnt!=dabc::mgr()) && (prnt!=dabc::mgr.GetApp()())) {
      res += ".";
      res += prnt->GetName();
      prnt = prnt->GetParent();
   }

   return fDimPrefix + res;
}

dabc::Parameter dimc::Observer::FindControlObject(const char* name)
{
   if ((name==0) || (strlen(name)==0)) return 0;

   Object* prnt = dabc::mgr();

   std::string fullname = name;

   size_t pos = 0;

   while ((pos = fullname.find_last_of('.')) != std::string::npos) {
      const char* prntname = fullname.c_str() + pos + 1;

      prnt = prnt->FindChild(prntname);
      if (prnt==0) return 0;

      fullname.erase(pos);
   }

   return dynamic_cast<dabc::ParameterContainer*> (prnt->FindChild(fullname.c_str()));
}

