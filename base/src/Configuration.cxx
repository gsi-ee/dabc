#include "dabc/Configuration.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Application.h"
#include "dabc/Parameter.h"

dabc::XdaqConfiguration::XdaqConfiguration(const char* fname, bool showerr) :
   fXml(),
   fDoc(0)
{
   fDoc = fXml.ParseFile(fname, showerr);

   XMLNodePointer_t rootnode = fXml.DocGetRootElement(fDoc);

   const char* name = fXml.GetNodeName(rootnode);

   if ((name==0) || (strcmp(name, "Partition")!=0)) {
      if (showerr) EOUT(("Instead of Partition root node %s found", name));
      fXml.FreeDoc(fDoc);
      fDoc = 0;
   }
}

dabc::XdaqConfiguration::~XdaqConfiguration()
{
   fXml.FreeDoc(fDoc);
}

dabc::XMLNodePointer_t dabc::XdaqConfiguration::FindContext(unsigned instance, bool showerr)
{
   if (fDoc==0) {
      if (showerr) EOUT(("document is not parsed correctly"));
      return 0;
   }

   XMLNodePointer_t rootnode = fXml.DocGetRootElement(fDoc);
   XMLNodePointer_t contextnode = fXml.GetChild(rootnode);

   while (contextnode!=0) {

      const char* name = fXml.GetNodeName(contextnode);

      if ((name==0) || (strcmp(name, "Context")!=0)) {
         if (showerr) EOUT(("Instead of Context node %s found", name));
         return 0;
      }

      dabc::XMLNodePointer_t appnode = fXml.GetChild(contextnode);
      name = fXml.GetNodeName(appnode);
      if ((name==0) || (strcmp(name, "Application")!=0)) {
         if (showerr) EOUT(("Instead of Application node %s found", name));
         return 0;
      }

      const char* value = fXml.GetAttr(appnode, "instance");
      if (value==0) {
         if (showerr) EOUT(("No 'instance' attribute in Application node"));
         return 0;
      }

      if ((unsigned)atoi(value) == instance) return contextnode;

      contextnode = fXml.GetNext(contextnode);
   }

   if (showerr) EOUT(("Specified context with instance = %u not found", instance));

   return 0;
}

unsigned dabc::XdaqConfiguration::NumNodes()
{
   unsigned cnt = 0;

   while (FindContext(cnt, false) != 0) cnt++;

   return cnt;
}

dabc::String dabc::XdaqConfiguration::NodeName(unsigned instance)
{
   String res("error");

   XMLNodePointer_t contextnode = FindContext(instance, false);

   const char* url = contextnode ? fXml.GetAttr(contextnode, "url") : 0;

   if (url!=0)
       if (strstr(url,"http://")==url) {
          url += 7;
          const char* pos = strstr(url, ":");
          if (pos==0) res = url;
                 else res.assign(url, pos-url);
       }

   return res;
}

bool dabc::XdaqConfiguration::LoadLibs(unsigned instance)
{
   XMLNodePointer_t contextnode = FindContext(instance);

   if (contextnode==0) return false;

   XMLNodePointer_t modnode = fXml.GetChild(contextnode);

   while (modnode!=0) {
      if (strcmp(fXml.GetNodeName(modnode), "Module")==0) {

         const char* libname = fXml.GetNodeContent(modnode);

         if ((strstr(libname,"libdim")==0) &&
             (strstr(libname,"libDabcBase")==0) &&
             (strstr(libname,"libDabcXDAQControl")==0))
                dabc::Manager::LoadLibrary(libname);
      }

      modnode = fXml.GetNext(modnode);
   }

   return true;
}

bool dabc::XdaqConfiguration::ReadPars(unsigned instance)
{
   XMLNodePointer_t contextnode = FindContext(instance);

   if (contextnode==0) return false;

   XMLNodePointer_t appnode = fXml.GetChild(contextnode);
   if (strcmp(fXml.GetNodeName(appnode), "Application")!=0) {
      EOUT(("Application node in context not found"));
      return false;
   }

   XMLNodePointer_t propnode = fXml.GetChild(appnode);
   if (strcmp(fXml.GetNodeName(propnode), "properties")!=0) {
      EOUT(("properties node in Application not found"));
      return false;
   }

   XMLNodePointer_t parnode = fXml.GetChild(propnode);

   Application* app = mgr()->GetApp();

   while (parnode != 0) {
      const char* parname = fXml.GetNodeName(parnode);
      const char* parvalue = fXml.GetNodeContent(parnode);
//      const char* partyp = xml.GetAttr(parnode, "xsi:type");

      if (strcmp(parname,"debugLevel")==0) {
         dabc::SetDebugLevel(atoi(parvalue));
         DOUT1(("Set debug level = %s", parvalue));
      } else
      if (strcmp(parname,"nodeId")==0) {
         DOUT1(("Ignore set of node id %s", parvalue));
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
