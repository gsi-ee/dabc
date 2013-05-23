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

#include "dimc/ServiceEntry.h"

#include "dimc/records.h"
#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/XmlEngine.h"

#include <string.h>

dimc::ServiceEntry::ServiceEntry(const std::string& dabcname, const std::string& dimname) :
   ::DimCommandHandler(),
   fDabcName(dabcname),
   fDimName(dimname),
   fService(0),
   fCmd(0),
   fBuffer(0),
   fBufferLen(0),
   fKind(kindNone)
{
   SetQuality(0);
}

dimc::ServiceEntry::~ServiceEntry()
{
   DeleteService();
}

void dimc::ServiceEntry::DeleteService()
{
   delete fCmd; fCmd = 0;

   delete fService; fService = 0;
   free(fBuffer); fBuffer = 0;

   fKind = kindNone;
   fBufferLen = 0;
}

void dimc::ServiceEntry::commandHandler()
{
   if (fCmd==0) {
      EOUT("No command was created !!!");
      return;
   }

   dabc::CommandDefinition cmddef = dabc::mgr.FindPar(fDabcName);
   if (cmddef.null()) {
      EOUT("No command found");
      return;
   }

//   ::DimCommand* com = getCommand();
//   if (com != fCmd) {
//      EOUT("!!!!!!!! Command mismatch !!!!!!");
//   }

//   const char* arg = ;

//   DOUT0("Try to execute command %s full %s arg %s", cmddef.GetName(), fDabcName.c_str(), arg ? arg : "---");

   dabc::WorkerRef wrk = cmddef.GetWorker();

   if (wrk.null()) {
      EOUT("No worker found for command definition %s", fDabcName.c_str());
      return;
   }

   dabc::Command cmd = cmddef.MakeCommand();

   ReadParsFromDimString(cmd, cmddef, fCmd->getString());

   wrk.Submit(cmd);
}

std::string dimc::ServiceEntry::CommandDefAsXml(const dabc::CommandDefinition& src)
{
   std::string res;

   if (src.null()) return res;

   int num = src.NumArgs();

   res = dabc::format("<command name=\"%s\" scope=\"public\">\n", src.GetName());

   for (int n=0;n<num;n++) {
      std::string name, kind, dflt;
      bool required;
      const char* styp = "C";

      if (src.GetArg(n, name, kind, required, dflt)) {
         if (kind == "int") styp = "I"; else
         if (kind == "double") styp = "F";

         res += dabc::format("  <argument name=\"%s\" type=\"%s\" value=\"%s\" required=\"%s\"/>\n",
                            name.c_str(), styp, dflt.c_str(), (required ? dabc::xmlTrueValue : dabc::xmlFalseValue));
      }
   }

   res+="</command>\n";

   return res;
}

bool dimc::ServiceEntry::ReadParsFromDimString(dabc::Command& cmd, const dabc::CommandDefinition& def, const char* pars)
{
   if ((pars==0) || (strlen(pars)<14)) return true;

   dabc::XMLNodePointer_t node = dabc::Xml::ReadSingleNode(pars+14);
   if (node==0) {
      EOUT("Cannot parse DIM command %s", pars);
      return false;
   }

   dabc::XMLNodePointer_t child = dabc::Xml::GetChild(node);

   while (child!=0) {

      if (dabc::Xml::HasAttr(child, "name") && dabc::Xml::HasAttr(child, "value")) {
         const char* parname = dabc::Xml::GetAttr(child, "name");
         const char* parvalue = dabc::Xml::GetAttr(child, "value");

         if ((parname!=0) && def.HasArg(parname))
            cmd.SetStr(parname, parvalue);
      }
      child = dabc::Xml::GetNext(child);
   }

   dabc::Xml::FreeNode(node);

   return true;
}



bool dimc::ServiceEntry::UpdateService(const dabc::Parameter& par, EServiceKind kind)
{
   if (par.null() && (kind==kindNone)) return false;

   if (kind==kindNone) {
      std::string skind = par.Kind();

      // DOUT0("SKIND = %s", skind.c_str());

      if (par.IsRatemeter() || par.IsAverage()) kind = kindRate; else
      if (skind == dabc::Record::kind_int()) kind = kindInt; else
      if (skind == dabc::Record::kind_double()) kind = kindDouble; else
      if (skind == dabc::Record::kind_unsigned()) kind = kindInt; else
      if (skind == dabc::Record::kind_str()) kind = kindString; else
      if ((skind == dabc::InfoParameter::infokind()) || par.IsName("Info")) kind = kindInfo; else
      if (skind == dabc::CommandDefinition::cmddefkind()) kind = kindCommand; else
      if (skind == "state") kind = kindStatus;

      if (kind==kindNone) kind = kindString;
   }

   if (kind!=fKind) {

      DeleteService();

      unsigned size(0);

      switch (kind) {
         case kindNone:
            return true;

         case kindInt:
            size = sizeof(int);
            break;
         case kindDouble:
            size = sizeof(double);
            break;
         case kindString:
            if (size<512) size = 512;
            break;
         case kindRate:
            size = sizeof(RateRec);
            break;
         case kindStatus:
            size = sizeof(StatusRec);
            break;
         case kindInfo:
            size = sizeof(InfoRec);
            break;
         case kindCommand:
            size = 8192; // make big buffer for command definition
            break;
         default:
            EOUT("Unprocessed kind %d", kind);
            return false;
      }

      if (size==0) return false;

      fKind = kind;
      fBuffer = malloc(size);
      memset(fBuffer,0,size);
      fBufferLen = size;
   }

   bool need_notify = fService!=0;

   SetRecVisibility(6);

   switch (fKind) {
      case kindInt:
         *((int*) fBuffer) = par.AsInt();

         if (fService==0) {
            fService = new ::DimService(fDimName.c_str(), *((int*)fBuffer));
            SetRecType(dimc::ATOMIC);
         }

         break;

      case kindDouble:
         *((double*) fBuffer) = par.AsDouble();

         if (fService==0) {
            fService = new ::DimService(fDimName.c_str(), *((double*)fBuffer));
            SetRecType(dimc::ATOMIC);
         }
         break;

      case kindString:
         strncpy((char*) fBuffer, par.AsStdStr().c_str(), fBufferLen-1);

         if (fService==0) {
            fService = new ::DimService(fDimName.c_str(), (char*)fBuffer);
            SetRecType(dimc::ATOMIC);
         }
         break;

      case kindRate: {
         RateRec* rec = (RateRec*) fBuffer;
         rec->value = par.AsDouble();
         rec->lower = par.GetLowerLimit();
         rec->upper = par.GetUpperLimit();
         strncpy((char*) rec->units, par.GetActualUnits().c_str(), sizeof(rec->units)-1);
         if (fService==0) {
            fService = new ::DimService(fDimName.c_str(), const_cast<char*>(dimc::desc_RateRec), fBuffer, fBufferLen);
            SetRecType(dimc::RATE);
         }

         DOUT2("Create ratemeter %s", par.GetName());

         break;
      }

      case kindStatus: {
         StatusRec* rec = (StatusRec*) fBuffer;
         rec->severity = 0;
         strcpy(rec->color, dimc::col_Green);
         strcpy(rec->status, "Running");

         if (fService==0) {
            fService = new ::DimService(fDimName.c_str(), const_cast<char*>(dimc::desc_StatusRec), fBuffer, fBufferLen);
            SetRecType(dimc::STATUS);
         }

         break;
      }

      case kindInfo: {
         InfoRec* rec = (InfoRec*) fBuffer;
         rec->verbose = 1;
         strcpy(rec->color, dimc::col_Green);
         strcpy(rec->info, "Initializing...");

         if (fService==0) {
            fService = new ::DimService(fDimName.c_str(), const_cast<char*>(dimc::desc_InfoRec), fBuffer, fBufferLen);
            SetRecType(dimc::INFO);
         }

         break;
      }

      case kindCommand: {
         std::string xml = CommandDefAsXml(par);

         strncpy((char*) fBuffer, xml.c_str(), fBufferLen-1);

         if (fService==0) {
            fService = new ::DimService((fDimName+"_").c_str(), (char*) fBuffer);
            SetRecVisibility(0);
            SetRecType(dimc::COMMANDDESC);
         }

         if (fCmd==0) {
            fCmd = new ::DimCommand(fDimName.c_str(), "C", this);
         }

         break;
      }

      default:
         EOUT("Unprocessed kind %d in update", kind);
         return false;
   }

   if (need_notify && fService) fService->updateService();

   return true;
}

void dimc::ServiceEntry::UpdateValue(const std::string& value)
{
   if (fBuffer==0) return;

   switch (fKind) {
      case kindInt:
        *((int*) fBuffer) = dabc::RecordValue(value).AsInt();
         break;
      case kindDouble:
         *((double*) fBuffer) = dabc::RecordValue(value).AsDouble();
         break;
      case kindString:
         // we put
         strncpy((char*) fBuffer, value.c_str(), fBufferLen-1);
         break;
      case kindRate: {
         RateRec* rec = (RateRec*) fBuffer;
         rec->value = dabc::RecordValue(value).AsDouble();
         break;
      }
      case kindStatus: {
         StatusRec* rec = (StatusRec*) fBuffer;
         DOUT0("CHANGE STATUS to %s", value.c_str());
         strncpy(rec->status, value.c_str(), sizeof(rec->status)-1);
         if (value == dabc::ApplicationBase::stHalted()) strncpy(rec->color, dimc::col_Red, sizeof(rec->color)-1); else
         if (value == dabc::ApplicationBase::stRunning()) strncpy(rec->color, dimc::col_Green, sizeof(rec->color)-1); else
            strncpy(rec->color, dimc::col_Yellow, sizeof(rec->color)-1);

         break;
      }
      case kindInfo: {
         InfoRec* rec = (InfoRec*) fBuffer;
         strncpy(rec->info, value.c_str(), sizeof(rec->info)-1);
         break;
      }
      case kindCommand: {
         DOUT0("Change of default value of command definition is not interesting");
         break;
      }

      default:
         EOUT("Unsupported kind %u", (unsigned) fKind);
         break;
   }

   if(fService) fService->updateService();
}

void dimc::ServiceEntry::SetQuality(unsigned int qual)
{
   fRecStat       = (dimc::recordstat)  (qual       & 0xff);
   fRecType       = (dimc::recordtype)  ((qual>>8)  & 0xff);
   fRecVisibility = (dimc::visiblemask) ((qual>>16) & 0xff);
   fRecMode       = (dimc::recordmode)  ((qual>>24) & 0xff);
   if(fService)
      fService->setQuality(qual);
}

unsigned dimc::ServiceEntry::GetQuality()
{
   return ((unsigned) fRecStat) | ((unsigned) (fRecType) << 8) | ((unsigned)(fRecVisibility) << 16) | ((unsigned) (fRecMode) << 24);
}

void dimc::ServiceEntry::SetRecType(dimc::recordtype t)
{
   fRecType = t;
   if(fService)
      fService->setQuality(GetQuality());
}

void dimc::ServiceEntry::SetRecVisibility(dimc::visiblemask mask)
{
   fRecVisibility = mask;
   if(fService)
      fService->setQuality(GetQuality());
}

void dimc::ServiceEntry::SetRecStat(dimc::recordstat s)
{
   fRecStat = s;
   if(fService)
      fService->setQuality(GetQuality());
}

void dimc::ServiceEntry::SetRecMode(dimc::recordmode m)
{
  fRecMode = m;
  if(fService)
    fService->setQuality(GetQuality());
}
