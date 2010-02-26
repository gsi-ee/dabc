/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#include "dabc/Configuration.h"

#include <unistd.h>
#include <stdlib.h>
#include <fnmatch.h>

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Application.h"
#include "dabc/Parameter.h"
#include "dabc/Iterator.h"
#include "dabc/Factory.h"
#include "dabc/SocketDevice.h"

dabc::Configuration::Configuration(const char* fname) :
   ConfigBase(fname),
   ConfigIO(),
   fSelected(0),
   fStoreStack(),
   fStoreLastPop(0),
   fCurrItem(0),
   fLastTop(0),
   fCurrStrict(true),
   fMgrHost(),
   fMgrName("Manager"),
   fMgrNodeId(0),
   fMgrNumNodes(0),
   fMgrDimNode(),
   fMgrDimPort(0)
{
}

dabc::Configuration::~Configuration()
{
}

bool dabc::Configuration::SelectContext(unsigned cfgid, unsigned nodeid, unsigned numnodes, const char* dimnode)
{
   fSelected = FindContext(cfgid);

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

   fMgrHost     = NodeName(cfgid);
   envHost      = fMgrHost;

   fMgrName     = ContextName(cfgid);
   envContext   = fMgrName;

   DOUT2(("Select context %u nodeid %u name %s", cfgid, nodeid, fMgrName.c_str()));

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
   val = Find1(fSelected, "", xmlRunNode, xmlLoglevel);
   if (!val.empty()) dabc::SetFileLevel(atoi(val.c_str()));
   val = Find1(fSelected, "", xmlRunNode, xmlParslevel);
   if (!val.empty()) dabc::WorkingProcessor::SetGlobalParsVisibility(atoi(val.c_str()));

   std::string log = Find1(fSelected, "", xmlRunNode, xmlLogfile);

   if (log.length()>0)
      dabc::Logger::Instance()->LogFile(log.c_str());

   log = Find1(fSelected, "", xmlRunNode, xmlLoglimit);
   if (log.length()>0)
      dabc::Logger::Instance()->SetLogLimit(atoi(log.c_str()));

   std::string sockethost = Find1(fSelected, "", xmlRunNode, xmlSocketHost);
   if (!sockethost.empty())
      dabc::SocketDevice::SetLocalHost(sockethost);

   if (dimnode==0) dimnode = getenv(xmlDIM_DNS_NODE);

   if (dimnode!=0) {
      const char* separ = strchr(dimnode, ':');
      if (separ==0)
         fMgrDimNode = dimnode;
      else {
         fMgrDimNode.assign(dimnode, separ - dimnode);
         fMgrDimPort = atoi(separ + 1);
      }
      DOUT4(("Mgr dim node %s port %d", fMgrDimNode.c_str(), fMgrDimPort));
   }

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
   if (sscanf(res.c_str(),"%d",&kind)!=1) return -1;
   return kind;
}

int dabc::Configuration::GetRunTime()
{
   if (fSelected==0) return 0;
   std::string res = Find1(fSelected, "", xmlRunNode, xmlRunTime);
   if (res.empty()) return 0;
   int runtime(0);
   if (sscanf(res.c_str(),"%d",&runtime)!=1) return 0;
   return runtime;

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
   return sscanf(sres.c_str(),"%d",&res) == 1 ? res : dflt;
}

const char* dabc::Configuration::ConetextAppClass()
{
   if (fSelected==0) return 0;

   XMLNodePointer_t node = fXml.GetChild(fSelected);

   while (node!=0) {
      if (IsNodeName(node, xmlApplication)) break;
      node = fXml.GetNext(node);
   }

   if (node==0)
      node = FindMatch(0, fSelected, xmlApplication);

   return fXml.GetAttr(node, xmlClassAttr);
}


bool dabc::Configuration::LoadLibs()
{
    if (fSelected==0) return false;

    std::string libname;
    XMLNodePointer_t last = 0;

    do {
       libname = FindN(fSelected, last, xmlRunNode, xmlUserLib);
       if (libname.empty()) break;
       DOUT2(("Find library %s in config", libname.c_str()));
       if (!dabc::Factory::LoadLibrary(ResolveEnv(libname))) return false;
    } while (true);

    return true;
}

bool dabc::Configuration::CreateItem(const char* name, const char* value)
{
   XMLNodePointer_t parent = 0;
   if (fStoreStack.size() > 0) parent = fStoreStack.back();
   XMLNodePointer_t item = fXml.NewChild(parent, 0, name, value);
   fStoreStack.push_back(item);
   return true;
}

bool dabc::Configuration::CreateAttr(const char* name, const char* value)
{
   if (fStoreStack.size() == 0) return false;
   XMLNodePointer_t parent = fStoreStack.back();
   fXml.NewAttr(parent, 0, name, value);
   return true;
}

bool dabc::Configuration::PopItem()
{
   if (fStoreStack.size() == 0) return false;
   fStoreLastPop = fStoreStack.back();
   fStoreStack.pop_back();
   return true;
}

bool dabc::Configuration::PushLastItem()
{
   if (fStoreLastPop==0) return false;
   fStoreStack.push_back(fStoreLastPop);
   fStoreLastPop = 0;
   return true;
}

bool dabc::Configuration::StoreObject(const char* fname, Basic* obj)
{
   if (obj == 0) return false;

   fStoreStack.clear();
   fStoreLastPop = 0;

   obj->Store(*this);

   if ((fStoreStack.size()!=0) || (fStoreLastPop==0)) {
      EOUT(("Error during store of object %s", obj->GetName()));
      if (fStoreStack.size()==0)
         fXml.FreeNode(fStoreLastPop);
      else
         fXml.FreeNode(fStoreStack.front());
      fStoreStack.clear();
      fStoreLastPop = 0;
      return false;
   }

   XMLDocPointer_t doc = fXml.NewDoc();
   fXml.DocSetRootElement(doc, fStoreLastPop);
   fXml.SaveDoc(doc, fname);
   fXml.FreeDoc(doc);

   fStoreLastPop = 0;

   return true;
}

bool dabc::Configuration::FindItem(const char* name)
{
   if (fCurrItem==0) return false;

   if (fCurrChld==0)
      fCurrChld = fXml.GetChild(fCurrItem);
   else
      fCurrChld = fXml.GetNext(fCurrChld);

   while (fCurrChld!=0) {
      if (IsNodeName(fCurrChld, name)) {
         fCurrItem = fCurrChld;
         fCurrChld = 0;
         return true;
      }
      fCurrChld = fXml.GetNext(fCurrChld);
   }

   return false;
}

bool dabc::Configuration::CheckAttr(const char* name, const char* value)
{
   // make extra check - if fCurrChld!=0 something was wrong already
   if ((fCurrChld!=0) || (fCurrItem==0)) return false;

   bool res = true;

   if (value==0)
      res = fXml.HasAttr(fCurrItem, name);
   else {
      const char* attr = fXml.GetAttr(fCurrItem, name);

      std::string sattr = attr ? ResolveEnv(attr) : std::string("");

      if (fCurrStrict)
         res = sattr.empty() ? false : (sattr == value);
      else
      if (!sattr.empty()) {
         res = (sattr == value);
         if (!res) res = fnmatch(sattr.c_str(), value, FNM_NOESCAPE) == 0;
      }
   }

   if (!res) {
      fCurrChld = fCurrItem;
      fCurrItem = fXml.GetParent(fCurrItem);
   }

   return res;
}

std::string dabc::Configuration::GetAttrValue(const char* name)
{
   const char* res = fXml.GetAttr(fCurrItem, name);
   if (res==0) return std::string();
   return ResolveEnv(res);
}

dabc::Basic* dabc::Configuration::GetObjParent(Basic* obj, int lvl)
{
   while ((obj!=0) && (lvl-->0)) obj = obj->GetParent();
   return obj;
}


bool dabc::Configuration::Find(Basic* obj, std::string& value, const char* findattr)
{
   std::string item;

   if (findattr!=0) {
      const char* pos = strrchr(findattr, '/');

      if (pos!=0) {
         item.assign(findattr, pos - findattr);
         findattr = pos+1;
         if (*findattr==0) findattr = 0;
      }
   }

   return FindItem(obj, value, item.empty() ? 0 : item.c_str(), findattr);
}

dabc::XMLNodePointer_t dabc::Configuration::FindSubItem(XMLNodePointer_t node, const char* name)
{
   if ((node==0) || (name==0) || (strlen(name)==0)) return node;

   const char* pos = strchr(name, '/');
   if (pos==0) return FindChild(node, name);

   std::string subname(name, pos-name);
   return FindSubItem(FindChild(node, subname.c_str()), pos+1);
}

bool dabc::Configuration::FindItem(Basic* obj, std::string &res, const char* finditem, const char* findattr)
{
   res.clear();

   if (obj==0) return false;

   int maxlevel = 0;
   Basic* prnt = 0;
   while ((prnt = GetObjParent(obj, maxlevel)) != 0) {
      if (prnt==dabc::mgr()) break;
      maxlevel++;
   }

   DOUT3(("Configuration::FindItem object %s lvl = %d  attr = %s",
          obj->GetFullName().c_str(), maxlevel, (findattr ? findattr : "---")));

   if (prnt==0) return false;

   // first, try strict syntax
   fCurrStrict = true;
   fCurrItem = fSelected;
   fCurrChld = 0;
   int level = maxlevel - 1;

   while (level>=0) {
      prnt = GetObjParent(obj, level);
      if (prnt==0) return false;

      // search on same level several items - it may match attributes only with second or third
      do {
         if (prnt->Find(*this)) break;
         if (fCurrChld == 0) prnt = 0;
      } while (prnt != 0);
      if (prnt==0) break;

      if (level--==0) {
         if (finditem!=0) fCurrItem = FindSubItem(fCurrItem, finditem);

         if ((fCurrItem!=0) && ((findattr==0) || fXml.HasAttr(fCurrItem, findattr))) {
            res = findattr ? GetAttrValue(findattr) : GetNodeValue(fCurrItem);
            DOUT3(("Exact par found %s res = %s", obj->GetFullName().c_str(), res.c_str()));
            return true;
         }
      }
   }

   fCurrStrict = false;

   bool search_in_normal_path = true; // first try to search in normal position, but with cast syntax

   do {

      DOUT3(("Start search with maxlevel = %d", maxlevel));

      fCurrItem = search_in_normal_path ? fSelected : Dflts();
      fCurrChld = 0;
      level = search_in_normal_path ? maxlevel-1 : maxlevel;

      while (level >= 0) {
         prnt = GetObjParent(obj, level);
         if (prnt == 0) return false;

//         DOUT3(("Search parent %s level %d", prnt->GetName(), level));

         XMLNodePointer_t curr = fCurrItem;

         if (prnt->Find(*this)) {
            if (level--==0) {
               if (finditem!=0) fCurrItem = FindSubItem(fCurrItem, finditem);
               if ((fCurrItem!=0) && ((findattr==0) || fXml.HasAttr(fCurrItem, findattr))) {
                  res = findattr ? GetAttrValue(findattr) : GetNodeValue(fCurrItem);
                  DOUT3(("Found object %s res = %s", obj->GetFullName().c_str(), res.c_str()));
                  return true;
               }
            }
         } else
         if (curr != fCurrItem) {

            EOUT(("FIXME: should not happen"));
            EOUT(("FIXME: problem in hierarchy search for %s lvl %d prnt %s", obj->GetFullName().c_str(), level, prnt->GetName()));
            EOUT(("fCurrChld %p   curr %p  fCurrItem %p", fCurrChld, curr, fCurrItem));

            // it can happen when node name is ok, but attribute did not match
            // normally wrong attribute should rollback hierarchy back

            fCurrChld = fCurrItem;
            fCurrItem = fXml.GetParent(fCurrItem);

            // check again that only one level was moved
            if (curr!=fCurrItem) { EOUT(("FIXME: big step"));  break; }
         } else
         if (fCurrChld == 0) {
            // object was not found, we increment depth level and try to continue our search
            level++;
            if (level > maxlevel) break;
            fCurrChld = fCurrItem;
            fCurrItem = fXml.GetParent(fCurrItem);
            if (fCurrItem==0) {
               EOUT(("FIXME: Wrong hierarchy search - one should repair it"));
               break;
            }
         }
      }

      if (search_in_normal_path)
         search_in_normal_path = false;
      else {
         maxlevel--;
         while (maxlevel > 0) {
            prnt = GetObjParent(obj, maxlevel);
            if (prnt->UseMasterClassName()) {
               DOUT3(("Try with master %s", prnt->GetName()));
               break;
            }
            maxlevel--;
         }
      }

   } while (maxlevel>0);

   return false;
}
