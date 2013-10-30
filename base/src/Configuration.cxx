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

#include "dabc/Configuration.h"

#include <unistd.h>
#include <stdlib.h>
#include <fnmatch.h>

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Application.h"
#include "dabc/Iterator.h"
#include "dabc/Factory.h"

std::string dabc::Configuration::fLocalHost = "";

dabc::Configuration::Configuration(const char* fname) :
   ConfigBase(fname),
   fSelected(0),
   fMgrHost(),
   fMgrPort(0),
   fMgrName("Manager"),
   fMgrNodeId(0),
   fMgrNumNodes(0)
{
}

dabc::Configuration::~Configuration()
{
}

bool dabc::Configuration::SelectContext(unsigned nodeid, unsigned numnodes)
{
   fSelected = FindContext(nodeid);

   if (fSelected==0) return false;

   envDABCNODEID = dabc::format("%u", nodeid);
   envDABCNUMNODES = dabc::format("%u", numnodes);

   envDABCSYS = GetEnv(xmlDABCSYS);

   envDABCUSERDIR = Find1(fSelected, "", xmlRunNode, xmlDABCUSERDIR);
   if (envDABCUSERDIR.empty()) envDABCUSERDIR = GetEnv(xmlDABCUSERDIR);

   char sbuf[1000];
   if (getcwd(sbuf, sizeof(sbuf)))
      envDABCWORKDIR = sbuf;
   else
      envDABCWORKDIR = ".";

   fMgrHost     = NodeName(nodeid);
   fMgrPort     = NodePort(nodeid);
   envHost      = fMgrHost;

   fMgrName     = ContextName(nodeid);
   envContext   = fMgrName;

   DOUT2("Select context nodeid %u name %s", nodeid, fMgrName.c_str());

   dabc::SetDebugPrefix(fMgrName.c_str());

   fMgrNodeId   = nodeid;
   fMgrNumNodes = numnodes;

   if (numnodes>1) {
      dabc::SetDebugLevel(0);
      dabc::SetFileLevel(1);
   } else {
      dabc::SetDebugLevel(1);
      dabc::SetFileLevel(1);
   }

   std::string val = Find1(fSelected, "", xmlRunNode, xmlDebuglevel);
   if (!val.empty()) dabc::SetDebugLevel(atoi(val.c_str()));

   val = Find1(fSelected, "", xmlRunNode, xmlNoDebugPrefix);
   if (!val.empty()) {
      dabc::lgr()->SetDebugMask(dabc::Logger::lMessage);
      dabc::lgr()->SetFileMask(dabc::Logger::lMessage);
   }

   val = Find1(fSelected, "", xmlRunNode, xmlLoglevel);
   if (!val.empty()) dabc::SetFileLevel(atoi(val.c_str()));

   std::string log = Find1(fSelected, "", xmlRunNode, xmlLogfile);

   if (log.length()>0)
      dabc::Logger::Instance()->LogFile(log.c_str());

   log = Find1(fSelected, "", xmlRunNode, xmlLoglimit);
   if (log.length()>0)
      dabc::Logger::Instance()->SetLogLimit(atoi(log.c_str()));

   std::string sockethost = Find1(fSelected, "", xmlRunNode, xmlSocketHost);
   if (!sockethost.empty()) fLocalHost = sockethost;

   return true;
}

std::string dabc::Configuration::InitFuncName()
{
   if (fSelected==0) return std::string("");

   return Find1(fSelected, "", xmlRunNode, xmlInitFunc);
}

std::string dabc::Configuration::RunFuncName()
{
   if (fSelected==0) return std::string("");

   return Find1(fSelected, "", xmlRunNode, xmlRunFunc);
}

int dabc::Configuration::ShowCpuInfo()
{
   if (fSelected==0) return -1;
   std::string res = Find1(fSelected, "", xmlRunNode, xmlCpuInfo);
   if (res.empty()) return -1;
   int kind(0);
   return dabc::str_to_int(res.c_str(), &kind) ? kind : -1;
}

int dabc::Configuration::UseControl()
{
   if (fSelected==0) return 0;
   std::string res = Find1(fSelected, "", xmlRunNode, xmlUseControl);
   if (res == xmlFalseValue) return -1;
   if (res == xmlTrueValue) return 1;
   return 0;
}

int dabc::Configuration::WithPublisher()
{
   if (fSelected==0) return 0;
   std::string res = Find1(fSelected, "", xmlRunNode, "publisher");
   if (res == xmlFalseValue) return -1;
   if (res == xmlTrueValue) return 1;
   return 0;
}


std::string dabc::Configuration::MasterName()
{
   if (fSelected==0) return std::string("");

   return Find1(fSelected, "", xmlRunNode, xmlMasterProcess);
}

int dabc::Configuration::GetRunTime()
{
   if (fSelected==0) return 0;
   std::string res = Find1(fSelected, "", xmlRunNode, xmlRunTime);
   if (res.empty()) return 0;
   int runtime(0);
   return dabc::str_to_int(res.c_str(), &runtime) ? runtime : 0;
}

bool dabc::Configuration::NormalMainThread()
{
   if (fSelected==0) return true;
   return Find1(fSelected, "", xmlRunNode, xmlNormalMainThrd) == xmlTrueValue;
}

std::string dabc::Configuration::Affinity()
{
   if (fSelected==0) return "";

   return Find1(fSelected, "", xmlRunNode, xmlAffinity);
}

std::string dabc::Configuration::ThreadsLayout()
{
   if (fSelected==0) return "";

   return Find1(fSelected, "", xmlRunNode, xmlThreadsLayout);
}


std::string dabc::Configuration::GetUserPar(const char* name, const char* dflt)
{
   if (fSelected==0) return std::string("");
   return Find1(fSelected, dflt ? dflt : "", xmlUserNode, name);
}

int dabc::Configuration::GetUserParInt(const char* name, int dflt)
{
   std::string sres = GetUserPar(name);
   if (sres.empty()) return dflt;
   int res(dflt);
   return dabc::str_to_int(sres.c_str(), &res) ? res : dflt;
}

std::string dabc::Configuration::ConetextAppClass()
{
   if (fSelected==0) return std::string();

   XMLNodePointer_t node = Xml::GetChild(fSelected);

   while (node!=0) {
      if (IsNodeName(node, xmlApplication)) break;
      node = Xml::GetNext(node);
   }

   if (node==0)
      node = FindMatch(0, fSelected, xmlApplication);

   const char* res = Xml::GetAttr(node, xmlClassAttr);

   return std::string(res ? res : "");
}


bool dabc::Configuration::LoadLibs()
{
    if (fSelected==0) return false;

    std::string libname;
    XMLNodePointer_t last = 0;

    do {
       libname = FindN(fSelected, last, xmlRunNode, xmlUserLib);
       if (libname.empty()) break;
       DOUT2("Find library %s in config", libname.c_str());
       if (!dabc::Factory::LoadLibrary(ResolveEnv(libname))) return false;
    } while (true);

    return true;
}

bool dabc::Configuration::NextCreationNode(XMLNodePointer_t& prev, const char* nodename, bool check_name_for_multicast)
{
   do {

      prev = FindMatch(prev, fSelected, nodename);
      if (prev==0) break;

      // if creation node marked with auto="false" attribute, than creation is disabled
      const char* autoattr = Xml::GetAttr(prev, xmlAutoAttr);
      if ((autoattr!=0) && strcmp(autoattr,xmlFalseValue)==0) continue;

      if (!check_name_for_multicast) break;
      const char* nameattr = Xml::GetAttr(prev, xmlNameAttr);
      if ((nameattr==0) || (strpbrk(nameattr,"*?")==0)) break;

   } while (prev!=0);

   return prev!=0;
}

