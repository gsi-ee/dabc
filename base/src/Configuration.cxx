#include "dabc/Configuration.h"

#include <unistd.h>

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Application.h"
#include "dabc/Parameter.h"

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
            dabc::String shortname(parname, separ-parname);
            const char* ownername = separ+1;

            if (app->IsName(ownername)) {
               Parameter* par = app->FindPar(shortname.c_str());
               if (par!=0) {
                  par->SetStr(parvalue);
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
   fStoreLastPop(0)
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

   String log;

   if (logfile!=0) log = logfile; else
   if (IsNative()) {
      logfile = Find1(fSelected, 0, xmlRunNode, xmlLogfile);
      if (logfile!=0) log = ResolveEnv(logfile);
   }

   if (log.length()>0)
      dabc::Logger::Instance()->LogFile(log.c_str());

   return true;
}

bool dabc::Configuration::LoadLibs()
{
    if (fSelected==0) return false;

    if (IsXDAQ()) return XDAQ_LoadLibs();

    const char* libname = 0;
    XMLNodePointer_t last = 0;

    while ((libname = FindN(fSelected, last, xmlRunNode, xmlUserLib))!=0) {
       DOUT0(("Find library %s", libname));
       dabc::Manager::LoadLibrary(ResolveEnv(libname).c_str());
    }

    return true;
}

bool dabc::Configuration::ReadPars()
{
   if (fSelected==0) return false;

   if (IsXDAQ()) return XDAQ_ReadPars();

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
