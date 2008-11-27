#include "dabc/Configuration.h"

#include <unistd.h>
#include <fnmatch.h>


#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Application.h"
#include "dabc/Parameter.h"
#include "dabc/Iterator.h"

bool dabc::Configuration::XDAQ_LoadLibs()
{
   XMLNodePointer_t modnode = fXml.GetChild(fSelected);

   while (modnode!=0) {
      if (strcmp(fXml.GetNodeName(modnode), xmlXDAQModule)==0) {

         const char* libname = fXml.GetNodeContent(modnode);

         DOUT1(("Find lib %s", libname));

         if ((strstr(libname,"libdim")==0) &&
             (strstr(libname,"libDabcBase")==0) &&
             (strstr(libname,"libDabcXDAQControl")==0))
                dabc::Manager::LoadLibrary(ResolveEnv(libname).c_str());
      }

      modnode = fXml.GetNext(modnode);
   }

   return true;
}

bool dabc::Configuration::XDAQ_ReadPars()
{

   XMLNodePointer_t appnode = fXml.GetChild(fSelected);

   if (!IsNodeName(appnode, xmlXDAQApplication)) {
      EOUT(("%s node in context not found", xmlXDAQApplication));
      return false;
   }

   XMLNodePointer_t propnode = fXml.GetChild(appnode);
   if (!IsNodeName(propnode, xmlXDAQproperties)!=0) {
      EOUT(("%s node not found", xmlXDAQproperties));
      return false;
   }

   XMLNodePointer_t parnode = fXml.GetChild(propnode);

   Application* app = mgr()->GetApp();

   while (parnode != 0) {
      const char* parname = fXml.GetNodeName(parnode);
      const char* parvalue = fXml.GetNodeContent(parnode);
//      const char* partyp = xml.GetAttr(parnode, "xsi:type");

      if (strcmp(parname,xmlXDAQdebuglevel)==0) {
         dabc::SetDebugLevel(atoi(parvalue));
         DOUT1(("Set debug level = %s", parvalue));
      } else {
         const char* separ = strchr(parname, '.');
         if ((separ!=0) && (app!=0)) {
            std::string shortname(parname, separ-parname);
            const char* ownername = separ+1;

            if (app->IsName(ownername)) {
               Parameter* par = app->FindPar(shortname.c_str());
               if (par!=0) {
                  par->SetValue(parvalue);
                  DOUT1(("Set parameter %s = %s", parname, parvalue));
               }
               else
                  EOUT(("Not found parameter %s in plugin", shortname.c_str()));
            } else {
               EOUT(("Not find owner %s for parameter %s", ownername, parname));
            }
         }
      }

      parnode = fXml.GetNext(parnode);
   }

   return true;
}

// ___________________________________________________________________________________


dabc::Configuration::Configuration(const char* fname) :
   ConfigBase(fname),
   ConfigIO(),
   fSelected(0),
   fStoreStack(),
   fStoreLastPop(0),
   fCurrItem(0),
   fLastTop(0)
{
}

dabc::Configuration::~Configuration()
{
}

bool dabc::Configuration::SelectContext(unsigned cfgid, unsigned nodeid, unsigned numnodes, const char* logfile)
{
   fSelected = IsXDAQ() ? XDAQ_FindContext(cfgid) : FindContext(cfgid);

   if (fSelected==0) return false;

   const char* val = getenv(xmlDABCSYS);
   if (val!=0) envDABCSYS = val;

   val = 0;
   if (IsNative()) val = Find1(fSelected, 0, xmlRunNode, xmlDABCUSERDIR);
   if (val==0) val = getenv(xmlDABCUSERDIR);
   if (val!=0) envDABCUSERDIR = val;

   char sbuf[1000];
   if (getcwd(sbuf, sizeof(sbuf)))
      envDABCWORKDIR = sbuf;
   else
      envDABCWORKDIR = ".";

   envDABCNODEID = FORMAT(("%u", nodeid));
   envDABCNUMNODES = FORMAT(("%u", numnodes));

   std::string log;

   if (logfile!=0) log = logfile; else
   if (IsNative()) {
      logfile = Find1(fSelected, 0, xmlRunNode, xmlLogfile);
      if (logfile!=0) log = ResolveEnv(logfile);
   }

   if (log.length()>0)
      dabc::Logger::Instance()->LogFile(log.c_str());

   const char* mgrname = 0;

   if (IsXDAQ()) {
      mgrname = fXml.GetAttr(fSelected, xmlXDAQurlattr);
      if (mgrname) mgrname+=7; // remove http:// prefix
   } else {
      mgrname = fXml.GetAttr(fSelected, xmlNameAttr);
   }

   if (mgrname==0) mgrname = "dabc";

   fMgrName     = mgrname;
   fMgrNodeId   = nodeid;
   fMgrNumNodes = numnodes;

   return true;
}

std::string dabc::Configuration::StartFuncName()
{
   if (IsXDAQ() || (fSelected==0)) return std::string("");

   const char* val = Find1(fSelected, 0, xmlRunNode, xmlUserFunc);

   return std::string(val ? val : "");
}


std::string dabc::Configuration::ControlType()
{
   if (IsXDAQ() || (fSelected==0)) return std::string("");

   const char* val = Find1(fSelected, 0, xmlRunNode, xmlControlType);

   return std::string(val ? val : "");
}

bool dabc::Configuration::LoadLibs(const char* startfunc)
{
    if (fSelected==0) return false;

    if (IsXDAQ()) return XDAQ_LoadLibs();

    const char* libname = 0;
    XMLNodePointer_t last = 0;

    while ((libname = FindN(fSelected, last, xmlRunNode, xmlUserLib))!=0) {
       DOUT0(("Find library %s", libname));
       dabc::Manager::LoadLibrary(ResolveEnv(libname).c_str(), startfunc);
    }

    return true;
}

bool dabc::Configuration::ReadPars()
{
   if (fSelected==0) return false;

   if (IsXDAQ()) return XDAQ_ReadPars();

   /*

   Application* app = mgr()->GetApp();
   if (app==0) return false;

   dabc::Iterator iter(app->GetTopParsFolder());

   while (iter.next()) {
      Parameter* par = dynamic_cast<Parameter*> (iter.current());
      if (par!=0) par->Read(*this);
   }
   */

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
   if (!IsNative() || (fCurrItem==0)) return false;

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

      if (fCurrStrict)
         res = attr == 0 ? false : strcmp(attr, value) == 0;
      else
      if (attr!=0) {
         res = strcmp(attr, value) == 0;
         if (!res) res = fnmatch(attr, value, FNM_NOESCAPE) == 0;
      }
   }

   if (!res) {
      fCurrChld = fCurrItem;
      fCurrItem = fXml.GetParent(fCurrItem);
   }

   return res;
}


const char* dabc::Configuration::GetItemValue()
{
   return fXml.GetNodeContent(fCurrItem);
}

const char* dabc::Configuration::GetAttrValue(const char* name)
{
   return fXml.GetAttr(fCurrItem, name);
}

dabc::Basic* dabc::Configuration::GetObjParent(Basic* obj, int lvl)
{
   while ((obj!=0) && (lvl-->0)) obj = obj->GetParent();
   return obj;
}

const char* dabc::Configuration::Find(Basic* obj, const char* findattr)
{
   if (obj==0) return 0;


   int maxlevel = 0;
   Basic* prnt = 0;
   while ((prnt = GetObjParent(obj, maxlevel)) != 0) {
      if (prnt==dabc::mgr()) break;
      maxlevel++;
   }

//   DOUT0(("Search object %s lvl = %d  attr = %s",
//          obj->GetFullName().c_str(), maxlevel, (findattr ? findattr : "---")));

   if (prnt==0) return 0;

   // first, try strict syntax
   fCurrStrict = true;
   fCurrItem = fSelected;
   fCurrChld = 0;
   int level = maxlevel - 1;

   while (level>=0) {
      prnt = GetObjParent(obj, level);
      if (prnt==0) return 0;

      // search on same level several items - it may match attributes only with second or third
      do {
         if (prnt->Find(*this)) break;
         if (fCurrChld == 0) prnt = 0;
      } while (prnt != 0);
      if (prnt==0) break;

      if (level--==0)
         if ((findattr==0) || fXml.HasAttr(fCurrItem, findattr)) {
            const char* res = findattr ? GetAttrValue(findattr) : GetItemValue();
            if (res==0) res = "";
//            DOUT0(("Exact found %s res = %s", obj->GetFullName().c_str(), res));
            return res;
         }
   }

   fCurrStrict = false;

   do {

      fCurrItem = Dflts();
      fCurrChld = 0;

      level = maxlevel - 1;
      while (level >= 0) {
         prnt = GetObjParent(obj, level);
         if (prnt == 0) return 0;

         if (prnt->Find(*this)) {
            if (level--==0)
               if ((findattr==0) || fXml.HasAttr(fCurrItem, findattr)) {
                  const char* res = findattr ? GetAttrValue(findattr) : GetItemValue();
                  if (res==0) res = "";
//                  DOUT0(("Found object %s res = %s", obj->GetFullName().c_str(), res));
                  return res;
               }
         } else
         if (fCurrChld == 0) {
            level++;
            if (level >= maxlevel) break;
            fCurrChld = fCurrItem;
            fCurrItem = fXml.GetParent(fCurrItem);
         }
      }

      maxlevel--;

      while (maxlevel > 0) {
         prnt = GetObjParent(obj, maxlevel - 1);
         if (prnt->UseMasterClassName()) {
//            DOUT0(("Try with master %s", prnt->GetName()));
            break;
         }
         maxlevel--;
      }

   } while (maxlevel>0);

   return 0;
}
