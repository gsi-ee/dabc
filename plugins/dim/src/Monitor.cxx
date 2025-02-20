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

#include "dim/Monitor.h"

#include <ctime>

#include "dabc/Publisher.h"
#include "dabc/Url.h"

#include "mbs/SlowControlData.h"


dim::Monitor::Monitor(const std::string &name, dabc::Command cmd) :
   mbs::MonitorSlowControl(name, "Dim", cmd),
   DimInfoHandler(),
   fDimDns(),
   fDimMask(),
   fDimPeriod(1.),
   fDimBr(nullptr),
   fDimInfos(),
   fLastScan(),
   fNeedDnsUpdate(true)
{
   strncpy(fNoLink, "no_link", sizeof(fNoLink));

   fDimDns = Cfg("DimDns", cmd).AsStr();
   fDimMask = Cfg("DimMask", cmd).AsStr("*");
   if (fDimMask.empty()) fDimMask = "*";

   fWorkerHierarchy.Create("DIMC", true);
   // fWorkerHierarchy.EnableHistory(100, true); // TODO: make it configurable

   fDimPeriod = Cfg("DimPeriod", cmd).AsDouble(1);

   CreateTimer("DimTimer", (fDimPeriod>0.01) ? fDimPeriod : 0.01);


   Publish(fWorkerHierarchy, "DIMC");
}

dim::Monitor::~Monitor()
{
   for (auto &entry : fDimInfos) {
      delete entry.second.info;
      entry.second.info = nullptr;
   }
   fDimInfos.clear();

   delete fDimBr;
   fDimBr = nullptr;
}


void dim::Monitor::OnThreadAssigned()
{
   dabc::ModuleAsync::OnThreadAssigned();

   if (!fDimDns.empty()) {
      dabc::Url url(fDimDns);
      if (url.IsValid()) {
        if (url.GetPort()>0)
           ::DimClient::setDnsNode(url.GetHostName().c_str(), url.GetPort());
        else
           ::DimClient::setDnsNode(url.GetHostName().c_str());
      }
   }

   fDimBr = new ::DimBrowser();
}

void dim::Monitor::ScanDimServices()
{
   char *service_name = nullptr, *service_descr = nullptr;
   int type;

// DimClient::addErrorHandler(errHandler);

   {
      dabc::LockGuard lock(fWorkerHierarchy.GetHMutex());

      for (auto &entry : fDimInfos) {
         entry.second.flag = 0;
      }
   }

   int nservices = fDimBr->getServices(fDimMask.c_str());
   DOUT3("found %d DIM services", nservices);

   while((type = fDimBr->getNextService(service_name, service_descr)) != 0)
   {
      nservices--;

      dabc::LockGuard lock(fWorkerHierarchy.GetHMutex());

      DOUT3("DIM type %d name %s descr %s", type, service_name, service_descr);

      if (!service_descr || ((type != 1) && (type != 2))) continue;

      auto iter = fDimInfos.find(service_name);
      if (iter != fDimInfos.end()) {
         iter->second.flag = type;  // mark entry as found
         continue;
      }

      MyEntry entry;
      entry.flag = type;

      // we processing call-back only for normal records
      if (type == 1)
         entry.info = new DimInfo(service_name, (void*) fNoLink, (int) sizeof(fNoLink), (DimInfoHandler*) this);

      fDimInfos[service_name] = entry;

      DOUT3("Create entry %p type %d name %s descr %s", entry.info, type, service_name, service_descr);

      dabc::Hierarchy item = fWorkerHierarchy.CreateHChild(service_name);

      if (type == 1) {
         item.EnableHistory(100);
      } else
      if (type==2) {
         item.SetField(dabc::prop_kind, "DABC.Command");
         item.SetField("dimcmd", service_name);
         item.SetField("numargs", 1);
         item.SetField("arg0", "PAR");
         item.SetField("arg0_dflt", "");
         item.SetField("arg0_kind", "string");
      }
   }

   {
      dabc::LockGuard lock(fWorkerHierarchy.GetHMutex());

      auto iter = fDimInfos.begin();
      while (iter != fDimInfos.end()) {
         if (iter->second.flag != 0) { iter++; continue; }

         delete iter->second.info;
         iter->second.info = nullptr;

         DOUT3("Destroy entry %s", iter->first.c_str());

         fWorkerHierarchy.RemoveHChild(iter->first);

         fDimInfos.erase(iter++);
      }
   }

   fLastScan.GetNow();
   fNeedDnsUpdate = false;
}

void dim::Monitor::ProcessTimerEvent(unsigned timer)
{
   if (!fDimBr) return;

   if (TimerName(timer) == "DimTimer")
      if (fLastScan.Expired(10.) || (fDimInfos.size() == 0) || fNeedDnsUpdate)
         ScanDimServices();

   mbs::MonitorSlowControl::ProcessTimerEvent(timer);
}

int dim::Monitor::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName(dabc::CmdHierarchyExec::CmdName())) {
      std::string path = cmd.GetStr("Item");
      std::string arg = cmd.GetStr("PAR");
      std::string dimcmd;

      {
         dabc::LockGuard lock(fWorkerHierarchy.GetHMutex());
         dabc::Hierarchy item = fWorkerHierarchy.FindChild(path.c_str());
         if (item.null()) {
            EOUT("Did not found command item %s in DIM hierarchy", path.c_str());
            return dabc::cmd_false;
         }
         dimcmd = item.GetField("dimcmd").AsStr();
      }

      if (dimcmd.empty()) return dabc::cmd_false;

      DOUT2("Execute DIM command %s arg %s", dimcmd.c_str(), arg.c_str());

      DimClient::sendCommand(dimcmd.c_str(), arg.c_str());

      return dabc::cmd_true;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

void dim::Monitor::infoHandler()
{
   // DIM method, called when service is updated

   DimInfo *info = getInfo();
   if (!info) return;

   if (info->getData() == fNoLink) {
      DOUT3("Get nolink for %s", info->getName());
      return;
   }

   if (info->getFormat() == 0) {
      EOUT("Get null format for %s", info->getName());
      return;
   }

   dabc::LockGuard lock(fWorkerHierarchy.GetHMutex());
   dabc::Hierarchy item = fWorkerHierarchy.GetHChild(info->getName());

   if (item.null()) {
      EOUT("Fail to find item for %s", info->getName());
      return;
   }

   auto iter = fDimInfos.find(info->getName());
   if ((iter == fDimInfos.end()) || (iter->second.info != info)) {
      EOUT("Did not found service %s in infos map", info->getName());
      return;
   }

   if (strcmp(info->getName(),"DIS_DNS/SERVER_LIST") == 0) {
      DOUT3("Get DIS_DNS/SERVER_LIST");
      fNeedDnsUpdate = true;
   }
   // DOUT0("Get change for info %p name :%s: format %s", info, info->getName(), info->getFormat());

   bool changed = true;

   iter->second.fKind = 0;
   if (strcmp(info->getFormat(),"I") == 0) {
      item.SetField("value", info->getInt());
      item.SetField(dabc::prop_kind,"rate");
      iter->second.fKind = 1;
      iter->second.fLong = info->getInt();
   } else
   if (strcmp(info->getFormat(),"F") == 0) {
      item.SetField("value", info->getFloat());
      item.SetField(dabc::prop_kind,"rate");
      iter->second.fKind = 2;
      iter->second.fDouble = info->getFloat();
   } else
   if (strcmp(info->getFormat(),"D") == 0) {
      item.SetField("value", info->getDouble());
      item.SetField(dabc::prop_kind,"rate");
      iter->second.fKind = 2;
      iter->second.fDouble = info->getDouble();
   } else
   if (strcmp(info->getFormat(),"C") == 0) {
      item.SetField("value", info->getString());
      item.SetField(dabc::prop_kind,"log");
   } else
   if ((strcmp(info->getFormat(),"F:1;I:1;F:1;F:1;F:1;F:1;C:16;C:16;C:16") == 0) && (strncmp(info->getName(),"DABC/",5) == 0)) {
      // old DABC rate record
      item.SetField("value", info->getFloat());
      item.SetField(dabc::prop_kind,"rate");
      iter->second.fKind = 2;
      iter->second.fDouble = info->getFloat();
   } else
   if ((strcmp(info->getFormat(),"I:1;C:16;C:128") == 0) && (strncmp(info->getName(),"DABC/",5) == 0)) {
      // old DABC info record
      item.SetField("value", (const char*) info->getData() + sizeof(int) + 16);
      item.SetField(dabc::prop_kind,"log");
   } else
   if (strlen(info->getFormat()) > 2) {
      const char *fmt = info->getFormat();

      // DOUT0("Process FORMAT %s", fmt);
      int fldcnt = 0;
      char* ptr = (char*) info->getData();
      while (*fmt != 0) {
         char kind = *fmt++;
         if (*fmt++!=':') break;
         int size = 0;
         while ((*fmt>='0') && (*fmt<='9')) {
            size = size*10 + (*fmt - '0');
            fmt++;
         }
         if (*fmt==';') fmt++;

         if (size <= 0) break;

         // DOUT0("Process item %c size %d", kind, size);

         dabc::RecordField fld;
         switch(kind) {
            case 'I' :
               if (size == 1) {
                  fld.SetInt(*((int*)ptr));
               } else {
                 std::vector<int64_t> vect;
                 for (int n=0;n<size;n++) vect.emplace_back(((int*)ptr)[n]);
                 fld.SetVectInt(vect);
               }
               ptr += sizeof(int) * size;
               break;
            case 'F' :
               if (size == 1) {
                  fld.SetDouble(*((float*)ptr));
               } else {
                 std::vector<double> vect;
                 for (int n=0;n<size;n++) vect.emplace_back(((float*)ptr)[n]);
                 fld.SetVectDouble(vect);
               }
               ptr += sizeof(float) * size;
               break;
            case 'D' :
               if (size == 1) {
                  fld.SetDouble(*((double*)ptr));
               } else {
                 std::vector<double> vect;
                 for (int n=0;n<size;n++) vect.emplace_back(((double*)ptr)[n]);
                 fld.SetVectDouble(vect);
               }
               ptr += sizeof(double) * size;
               break;
            case 'C' : {
               int slen = 0;
               while ((slen < size) && (ptr[slen] != 0)) slen++;
               if (slen<size)
                  fld.SetStr(ptr);
               else
                  fld.SetStr(std::string(ptr,size));
               ptr += size;
               break;
            }
            default:
               EOUT("Unknown data format %c", kind);
               break;
         }
         // unrecognized field
         if (fld.null()) break;

         item.SetField(dabc::format("fld%d", fldcnt++), fld);
      }
   } else {
      if (strlen(info->getFormat()) > 0)
         EOUT("Not processed DIM format %s for record %s", info->getFormat(), info->getName());
      changed = false;
   }

   if (changed) item.MarkChangedItems();

}

unsigned dim::Monitor::GetRecRawSize()
{
   fRec.Clear();

   {
      dabc::LockGuard lock(fWorkerHierarchy.GetHMutex());

      for (auto &entry : fDimInfos) {
         switch (entry.second.fKind) {
            case 1: fRec.AddLong(entry.first, entry.second.fLong); break;
            case 2: fRec.AddDouble(entry.first, entry.second.fDouble); break;
            default: break;
         }
      }
   }

   return fRec.GetRawSize();
}
