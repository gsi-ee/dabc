#include "dabc/ConfigBase.h"

#include <fnmatch.h>

#include "dabc/logging.h"

namespace dabc {

   const char* xmlXDAQPartition    = "Partition";
   const char* xmlXDAQContext      = "Context";
   const char* xmlXDAQApplication  = "Application";
   const char* xmlXDAQproperties   = "properties";
   const char* xmlXDAQinstattr     = "instance";
   const char* xmlXDAQurlattr      = "url";
   const char* xmlXDAQModule       = "Module";
   const char* xmlXDAQdebuglevel   = "debugLevel";

}

dabc::XMLNodePointer_t dabc::ConfigBase::XDAQ_FindContext(unsigned instance)
{
   if (!IsXDAQ()) {
      EOUT(("document is not parsed correctly"));
      return 0;
   }

   XMLNodePointer_t rootnode = fXml.DocGetRootElement(fDoc);
   XMLNodePointer_t contextnode = fXml.GetChild(rootnode);

   while (contextnode!=0) {

      if (!IsNodeName(contextnode, xmlXDAQContext)) {
         EOUT(("Other than %s node was found", xmlXDAQContext));
         return 0;
      }

      dabc::XMLNodePointer_t appnode = fXml.GetChild(contextnode);
      if (!IsNodeName(appnode, xmlXDAQApplication)) {
         EOUT(("Other than %s node was found", xmlXDAQApplication));
         return 0;
      }

      const char* value = fXml.GetAttr(appnode, xmlXDAQinstattr);
      if (value==0) {
         EOUT(("No %s attribute in %s node", xmlXDAQinstattr, xmlXDAQApplication));
         return 0;
      }

      if ((unsigned)atoi(value) == instance) return contextnode;

      contextnode = fXml.GetNext(contextnode);
   }

   return 0;
}

unsigned dabc::ConfigBase::XDAQ_NumNodes()
{
   unsigned cnt = 0;

   while (XDAQ_FindContext(cnt) != 0) cnt++;

   return cnt;
}

std::string dabc::ConfigBase::XDAQ_NodeName(unsigned instance)
{
   XMLNodePointer_t contextnode = XDAQ_FindContext(instance);
   if (contextnode==0) return std::string("");
   const char* url = contextnode ? fXml.GetAttr(contextnode, xmlXDAQurlattr) : 0;
   if ((url==0) || (strstr(url,"http://")!=url)) return std::string("");
   url += 7;
   std::string nodename;
   const char* pos = strstr(url, ":");
   if (pos==0) nodename = url;
          else nodename.assign(url, pos-url);

   return nodename;

}


std::string dabc::ConfigBase::XDAQ_SshArgs(unsigned instance, int kind, const char* topcfgfile, const char* topworkdir, const char* connstr)
{
   XMLNodePointer_t contextnode = XDAQ_FindContext(instance);

   std::string nodename = XDAQ_NodeName(instance);

   if ((contextnode==0) || (nodename.length()==0)) return std::string("");

   std::string res = "ssh ";
   res += nodename;

   const char* dabcsys = getenv("DABCSYS");
   if (dabcsys==0) {
      EOUT(("DABCSYS not specified"));
      return std::string("");
   }

   envDABCSYS = dabcsys ? dabcsys : "";
   envDABCWORKDIR = topworkdir ? topworkdir : "";
   envDABCNODEID = FORMAT(("%u", instance));
   envDABCNUMNODES = FORMAT(("%u", NumNodes()));

   if (kind == kindTest) {
      if (topworkdir!=0) {
         std::string dir = ResolveEnv(topworkdir);
         res += FORMAT((" if [ ! -d %s ] ; then echo workdir = %s missed; exit 12; fi; ", dir.c_str(), dir.c_str()));
         res += FORMAT((" cd %s;", dir.c_str()));
      }

      res += FORMAT((" echo test for node %s done", nodename.c_str()));

   } else
   if (kind == kindStart) {
      res += FORMAT((" export DABCSYS=%s; ", dabcsys));
      res += " export LD_LIBRARY_PATH=$DABCSYS/lib:$LD_LIBRARY_PATH;";
      res += FORMAT((" cd %s;", topworkdir));
      res += FORMAT(("$DABCSYS/bin/dabc_run %s -cfgid %u -nodeid %u -numnodes %u", topcfgfile, instance, instance, NumNodes()));

      if (connstr!=0) res += FORMAT((" -conn %s", connstr));
   } else
   if (kind == kindConn) {
      // this is way to get connection string
      if (connstr==0) {
         res += " echo No connection string specified; exit 1";
      } else {
         res += FORMAT((" cd %s;", topworkdir));
         res += FORMAT((" if [ -f %s ] ; then cat %s; rm -f %s; fi", connstr, connstr, connstr));
      }
   } else
   if (kind == kindKill) {
      res += " killall --quiet dabc_run";
   } else
   if (kind == kindStop) {
      res += " pkill -SIGINT dabc_run";
   }

   return res;
}

// ________________________________________________________________________________

namespace dabc {

   const char* xmlRootNode         = "dabc";
   const char* xmlVersionAttr      = "version";
   const char* xmlContNode         = "Context";
   const char* xmlDefualtsNode     = "Defaults";
   const char* xmlNameAttr         = "name";
   const char* xmlClassAttr        = "class";
   const char* xmlValueAttr        = "v";
   const char* xmlRunNode          = "Run";
   const char* xmlSshHost          = "host";
   const char* xmlSshUser          = "user";
   const char* xmlSshPort          = "port";
   const char* xmlSshInit          = "init";
   const char* xmlSshTest          = "test";
   const char* xmlSshTimeout       = "timeout";
   const char* xmlDABCSYS          = "DABCSYS";
   const char* xmlDABCUSERDIR      = "DABCUSERDIR";
   const char* xmlDABCWORKDIR      = "DABCWORKDIR";
   const char* xmlDABCNODEID       = "DABCNODEID";
   const char* xmlDABCNUMNODES     = "DABCNUMNODES";
   const char* xmlWorkDir          = "workdir";
   const char* xmlLogfile          = "logfile";
   const char* xmlLDPATH           = "LD_LIBRARY_PATH";
   const char* xmlConfigFile       = "config";
   const char* xmlConfigFileId     = "configid";
   const char* xmlUserLib          = "Lib";
}

dabc::ConfigBase::ConfigBase(const char* fname) :
   fXml(),
   fDoc(0),
   fVersion(-1),
   fPrnt(0),
   envDABCSYS(),
   envDABCUSERDIR(),
   envDABCNODEID()
{
   if (fname==0) return;

   fDoc = fXml.ParseFile(fname, true);

   XMLNodePointer_t rootnode = fXml.DocGetRootElement(fDoc);

   if (IsNodeName(rootnode, xmlRootNode)) {
      fVersion = GetIntAttr(rootnode, xmlVersionAttr, 1);
   } else
   if (IsNodeName(rootnode, xmlXDAQPartition)) {
      fVersion = 0;
   } else {
      EOUT(("Xml file %s has neither dabc nor xdaq format", fname));
      fXml.FreeDoc(fDoc);
      fDoc = 0;
      fVersion = -1;
   }
}

dabc::ConfigBase::~ConfigBase()
{
   fXml.FreeDoc(fDoc);
}

bool dabc::ConfigBase::IsNodeName(XMLNodePointer_t node, const char* name)
{
   if ((node==0) || (name==0)) return false;

   const char* n = fXml.GetNodeName(node);

   return n==0 ? false : strcmp(n,name) == 0;
}

const char* dabc::ConfigBase::GetAttr(XMLNodePointer_t node, const char* attr, const char* defvalue)
{
   const char* res = fXml.GetAttr(node, attr);
   return res ? res : defvalue;
}

int dabc::ConfigBase::GetIntAttr(XMLNodePointer_t node, const char* attr, int defvalue)
{
   const char* res = GetAttr(node, attr, 0);
   return res ? atoi(res) : defvalue;
}

dabc::XMLNodePointer_t dabc::ConfigBase::FindChild(XMLNodePointer_t node, const char* name)
{
   XMLNodePointer_t child = fXml.GetChild(node);
   while (child!=0) {
      if (IsNodeName(child, name)) return child;
      child = fXml.GetNext(child);
   }
   return 0;
}


bool dabc::ConfigBase::ProduceClusterFile(const char* fname, int numnodes)
{
   XmlEngine xml;


   XMLNodePointer_t rootnode = xml.NewChild(0, 0, xmlRootNode);
   xml.NewIntAttr(rootnode, xmlVersionAttr, 1);

   for(int n=0;n<numnodes;n++) {
      XMLNodePointer_t contnode = xml.NewChild(rootnode, 0, xmlContNode);
      xml.NewAttr(contnode, 0, xmlNameAttr, FORMAT(("Cont%d", n)));

      XMLNodePointer_t runnode = xml.NewChild(contnode, 0, xmlRunNode);

      xml.NewChild(runnode, 0, xmlSshHost, n==0 ? "lxg0526" : FORMAT(("lxi%03d", n+5)));
//      xml.NewChild(runnode, 0, xmlSshUser, "linev");
//      xml.NewChild(runnode, 0, xmlSshPort, "22");
   }

   XMLNodePointer_t defnode = xml.NewChild(rootnode, 0, xmlDefualtsNode);
   XMLNodePointer_t contnode = xml.NewChild(defnode, 0, xmlContNode);
   xml.NewAttr(contnode, 0, xmlNameAttr, "*");
   XMLNodePointer_t runnode = xml.NewChild(contnode, 0, xmlRunNode);
   xml.NewChild(runnode, 0, xmlSshUser, "linev");
   xml.NewChild(runnode, 0, xmlSshPort, "22");



   XMLDocPointer_t doc = xml.NewDoc();
   xml.DocSetRootElement(doc, rootnode);
   xml.SaveDoc(doc, fname);
   xml.FreeDoc(doc);

   return true;
}

dabc::XMLNodePointer_t dabc::ConfigBase::FindItemMatch(XMLNodePointer_t& lastmatch,
                                                       XMLNodePointer_t node,
                                                       const char* sub1,
                                                       const char* sub2,
                                                       const char* sub3)
{
   if (sub1==0) return 0;

   XMLNodePointer_t subnode = fXml.GetChild(node);
   while (subnode!=0) {
      if (IsNodeName(subnode, sub1)) {
         if (sub2==0) {
            if (lastmatch==0) return subnode;
            if (lastmatch==subnode) lastmatch = 0;
         } else {
            XMLNodePointer_t res = FindItemMatch(lastmatch, subnode, sub2, sub3, 0);
            if (res!=0) return res;
         }
      }

      subnode = fXml.GetNext(subnode);
   }

   return 0;
}

dabc::XMLNodePointer_t dabc::ConfigBase::Dflts()
{
   if (fDoc==0) return 0;

   XMLNodePointer_t rootnode = fXml.DocGetRootElement(fDoc);

   if (rootnode==0) return 0;

   XMLNodePointer_t node = fXml.GetChild(rootnode);

   while (node!=0) {
      if (IsNodeName(node, xmlDefualtsNode)) return node;
      node = fXml.GetNext(node);
   }

   return 0;
}

bool dabc::ConfigBase::NodeMaskMatch(XMLNodePointer_t node, XMLNodePointer_t mask)
{
   if ((node==0) || (mask==0)) return false;

   if (!IsNodeName(node, fXml.GetNodeName(mask))) return false;

   XMLAttrPointer_t xmlattr = fXml.GetFirstAttr(node);

   while (xmlattr!=0) {

      const char* attrname = fXml.GetAttrName(xmlattr);

      const char* value = fXml.GetAttr(node, attrname);
      const char* maskvalue = fXml.GetAttr(mask, attrname);

      if ((value!=0) && (maskvalue!=0))
         if (fnmatch(maskvalue, value, FNM_NOESCAPE)!=0) return false;

      xmlattr = fXml.GetNextAttr(xmlattr);
   }

   return true;
}

dabc::XMLNodePointer_t dabc::ConfigBase::FindNodesMatch(
        XMLNodePointer_t& lastmatch,
        XMLNodePointer_t searchfold,
        XMLNodePointer_t topnode,
        XMLNodePointer_t node,
        const char* sub1,
        const char* sub2,
        const char* sub3)
{
   if (searchfold==0) return 0;

   XMLNodePointer_t subfolder = fXml.GetChild(searchfold);

   XMLNodePointer_t res = 0;

   while (subfolder!=0) {
      if (NodeMaskMatch(topnode, subfolder)) {
         if (topnode==node) {
            res = FindItemMatch(lastmatch, subfolder, sub1, sub2, sub3);
         } else {
            XMLNodePointer_t nexttop = node;

            while (fXml.GetParent(nexttop) != topnode) {
               nexttop = fXml.GetParent(nexttop);
               if (nexttop==0) {
                  EOUT(("Internal error !!!!!"));
                  return 0;
               }
            }

            res = FindNodesMatch(lastmatch, subfolder, nexttop, node, sub1, sub2, sub3);
         }

         if (res!=0) return res;
      }

      subfolder = fXml.GetNext(subfolder);
   }

   return 0;
}


dabc::XMLNodePointer_t dabc::ConfigBase::FindMatch(XMLNodePointer_t lastmatch,
                                                   XMLNodePointer_t topnode,
                                                   XMLNodePointer_t node,
                                                   const char* sub1,
                                                   const char* sub2,
                                                   const char* sub3)
{
   if (node==0) return 0;

   // first of all, check if node has specified subitem

   XMLNodePointer_t nextmatch = FindItemMatch(lastmatch, node, sub1, sub2, sub3);

   if (nextmatch!=0) return nextmatch;

   dabc::ConfigBase* cfg = this;

   do {
      nextmatch = cfg->FindNodesMatch(lastmatch, cfg->Dflts(), topnode, node, sub1, sub2, sub3);
      if (nextmatch!=0) return nextmatch;
      cfg = cfg->fPrnt;
   } while (cfg!=0);

   return 0;
}

/** Search first entry for item sub1/sub2/sub3 in specified node
 *  First tries full path, than found sub1 and path sub2/sub3
 */

const char* dabc::ConfigBase::Find1(XMLNodePointer_t node,
                                    const char* dflt,
                                    const char* sub1,
                                    const char* sub2,
                                    const char* sub3)
{
   XMLNodePointer_t res = FindMatch(0, node, node, sub1, sub2, sub3);
   if (res==0) {
      if (sub2!=0) {
         XMLNodePointer_t child = FindChild(node, sub1);
         if (child) return Find1(child, dflt, sub2, sub3, 0);
      }
      return dflt;
   }
   const char* cont = fXml.GetNodeContent(res);
   if (cont==0) cont = fXml.GetAttr(res, xmlValueAttr);
   return cont ? cont : dflt;
}

const char* dabc::ConfigBase::FindN(XMLNodePointer_t node,
                                    XMLNodePointer_t& prev,
                                    const char* sub1,
                                    const char* sub2,
                                    const char* sub3)
{
   XMLNodePointer_t res = FindMatch(prev, node, node, sub1, sub2, sub3);
   prev = res;
   if (res==0) return 0;
   const char* cont = fXml.GetNodeContent(res);
   if (cont==0) cont = fXml.GetAttr(res, xmlValueAttr);
   return cont ? cont : "";
}

unsigned dabc::ConfigBase::NumNodes()
{
   if (!IsOk()) return 0;
   if (IsXDAQ()) return XDAQ_NumNodes();

   XMLNodePointer_t rootnode = fXml.DocGetRootElement(fDoc);
   if (rootnode==0) return 0;
   XMLNodePointer_t node = fXml.GetChild(rootnode);
   unsigned cnt = 0;
   while (node!=0) {
      if (IsNodeName(node, xmlContNode)) cnt++;
      node = fXml.GetNext(node);
   }
   return cnt;
}

std::string dabc::ConfigBase::NodeName(unsigned id)
{
   if (IsXDAQ()) return XDAQ_NodeName(id);
   XMLNodePointer_t contnode = FindContext(id);
   if (contnode == 0) return std::string("");
   return std::string(Find1(contnode, 0, xmlRunNode, xmlSshHost));
}


dabc::XMLNodePointer_t dabc::ConfigBase::FindContext(unsigned id)
{
   if (fDoc==0) return 0;
   XMLNodePointer_t rootnode = fXml.DocGetRootElement(fDoc);
   if (rootnode==0) return 0;
   XMLNodePointer_t node = fXml.GetChild(rootnode);
   unsigned cnt = 0;

   while (node!=0) {
      if (IsNodeName(node, xmlContNode))
         if (cnt++ == id) return node;
      node = fXml.GetNext(node);
   }

   return 0;
}


std::string dabc::ConfigBase::ResolveEnv(const char* arg)
{
   if ((arg==0) || (strlen(arg)==0)) return std::string();

   std::string name = arg;

   while (name.find("${") != name.npos) {

      size_t pos1 = name.find("${");
      size_t pos2 = name.find("}");

      if ((pos1>pos2) || (pos2==name.npos)) {
         EOUT(("Wrong variable parenthesis %s", arg));
         return std::string(arg);
      }

      std::string var(name, pos1+2, pos2-pos1-2);

      name.erase(pos1, pos2-pos1+1);

      if (var.length()>0) {
         const char* value = 0;

         if ((var==xmlDABCSYS) && (envDABCSYS.length()>0)) value = envDABCSYS.c_str(); else
         if ((var==xmlDABCUSERDIR) && (envDABCUSERDIR.length()>0)) value = envDABCUSERDIR.c_str(); else
         if (var==xmlDABCWORKDIR) value = envDABCWORKDIR.c_str(); else
         if (var==xmlDABCNODEID) value = envDABCNODEID.c_str(); else
         if (var==xmlDABCNUMNODES) value = envDABCNUMNODES.c_str(); else

         if (value==0) value = getenv(var.c_str());

         if (value!=0) name.insert(pos1, value);
      }
   }
   return name;
}

std::string dabc::ConfigBase::SshArgs(unsigned id, const char* skind, const char* topcfgfile, const char* topworkdir, const char* connstr)
{
   if (skind==0) skind = "test";

   int kind = -1;
   if (strcmp(skind, "kill")==0) kind = kindKill; else
   if (strcmp(skind, "start")==0) kind = kindStart; else
   if (strcmp(skind, "stop")==0) kind = kindStop; else
   if (strcmp(skind, "test")==0) kind = kindTest; else
   if (strcmp(skind, "run")==0) kind = kindTest; else
   if (strcmp(skind, "conn")==0) kind = kindConn;

   if (kind<0) return std::string("");

   if (IsXDAQ()) return XDAQ_SshArgs(id, kind, topcfgfile, topworkdir, connstr);

   std::string res;
   XMLNodePointer_t contnode = FindContext(id);
   if (contnode == 0) return std::string("");

   const char* hostname = Find1(contnode, 0, xmlRunNode, xmlSshHost);
   if (hostname==0) {
      EOUT(("%s is not found", xmlSshHost));
      return std::string("");
   }

   const char* username = Find1(contnode, 0, xmlRunNode, xmlSshUser);
   const char* portid = Find1(contnode, 0, xmlRunNode, xmlSshPort);
   const char* timeout = Find1(contnode, 0, xmlRunNode, xmlSshTimeout);
   const char* initcmd = Find1(contnode, 0, xmlRunNode, xmlSshInit);
   const char* testcmd = Find1(contnode, 0, xmlRunNode, xmlSshTest);

   const char* dabcsys = Find1(contnode, 0, xmlRunNode, xmlDABCSYS);
   const char* userdir = Find1(contnode, 0, xmlRunNode, xmlDABCUSERDIR);
   const char* workdir = Find1(contnode, 0, xmlRunNode, xmlWorkDir);
   const char* logfile = Find1(contnode, 0, xmlRunNode, xmlLogfile);
   const char* ldpath = Find1(contnode, 0, xmlRunNode, xmlLDPATH);
   const char* cfgfile = Find1(contnode, 0, xmlRunNode, xmlConfigFile);
   const char* cfgid = Find1(contnode, 0, xmlRunNode, xmlConfigFileId);

   const char* envdabcsys = getenv("DABCSYS");

   if ((topcfgfile==0) && (cfgfile==0)) {
      EOUT(("Config file not defined"));
      return std::string("");
   }

//  char currdir[1024];
// if (!getcwd(currdir, sizeof(currdir))) strcpy(currdir,".");

   if (workdir==0) workdir = topworkdir;

   envDABCSYS = dabcsys ? dabcsys : "";
   envDABCUSERDIR = userdir ? userdir : "";
   envDABCWORKDIR = workdir ? workdir : "";
   envDABCNODEID = FORMAT(("%u", id));
   envDABCNUMNODES = FORMAT(("%u", NumNodes()));

   res = "ssh ";

   if (portid!=0) {
      res += "-p ";
      res += portid;
      res += " ";
   }

   if (timeout) {
      res += "-o ConnectTimeout=";
      res += timeout;
      res += " ";
   }

   if (username) {
      res += username;
      res += "@";
   }

   res += hostname;

   if (kind == kindTest) {
      // this is a way to get test arguments

//      res += FORMAT ((" . gsi_environment.sh; echo $HOST - %s; ls /data.local1; exit 243", hostname));

      if (initcmd!=0) res += FORMAT((" %s;", initcmd));

      if (testcmd!=0) res += FORMAT((" %s;", testcmd));

      if (dabcsys!=0)
         res += FORMAT((" if [ ! -d %s ] ; then echo DABCSYS = %s missed; exit 11; fi; ", dabcsys, dabcsys));
      else
      if (envdabcsys!=0)
         res += FORMAT((" if [ z $DABSYS -a ! -d %s ] ; then echo DABCSYS = %s missed; exit 11; fi; ", envdabcsys, envdabcsys));
      else
         res += " if [ z $DABCSYS ] ; then echo DABCSYS not specified; exit 7; fi;";

      if (workdir!=0) {
         std::string dir = ResolveEnv(workdir);
         res += FORMAT((" if [ ! -d %s ] ; then echo workdir = %s missed; exit 12; fi; ", dir.c_str(), dir.c_str()));
         res += FORMAT((" cd %s;", dir.c_str()));
      }

      if (cfgfile!=0) {
         std::string f = ResolveEnv(cfgfile);
         res += FORMAT((" if [ ! -f %s ] ; then echo cfgfile = %s missed; exit 12; fi; ", f.c_str(), f.c_str()));
      } else
         res += FORMAT((" if [ ! -f %s ] ; then echo maincfgfile = %s missed; exit 12; fi; ", topcfgfile, topcfgfile));

      res += FORMAT((" echo test on node %s done;", hostname));

//      res += " 'echo Test node ";
//      res += hostname;
//      res += "'";
   } else

   if (kind == kindStart) {
      // this is main run arguments

      if (initcmd!=0) res += FORMAT((" %s;", initcmd));

      std::string ld;
      if (ldpath) res = ResolveEnv(ldpath);
      if (userdir) { if (res.length()>0) res += ":"; res += ResolveEnv(userdir); }

      if (dabcsys!=0) {
         res += FORMAT((" export DABCSYS=%s;", dabcsys));
         res += " export LD_LIBRARY_PATH=";
         if (ld.length()>0) { res += ld; res+=":"; }
         res += "$DABCSYS/lib:$LD_LIBRARY_PATH;";
      } else
      if (envdabcsys!=0) {
         res += FORMAT(( " if [ z $DABCSYS -a -d %s ] ; then export DABCSYS=%s;", envdabcsys, envdabcsys));
         res += " export LD_LIBRARY_PATH=";
         if (ld.length()>0) { res += ld; res+=":"; }
         res += "$DABCSYS/lib:$LD_LIBRARY_PATH; fi;";
      } else
      if (ld.length()>0) {
         res += " if [ z $DABCSYS ] ; then echo DABCSYS not specified; exit 7; fi;";
         res+=FORMAT((" export LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH;", ld.c_str()));
      }

      if (workdir) res += FORMAT((" cd %s;", ResolveEnv(workdir).c_str()));

      if ((connstr!=0) && (id==0))
         res += FORMAT((" rm -f %s;", connstr));

      res += " $DABCSYS/bin/dabc_run ";

      if (cfgfile) {
         res += ResolveEnv(cfgfile);
         if (cfgid!=0) {
            res += " -cfgid ";
            res += cfgid;
         }
      } else {
         res += topcfgfile;
         res += " -cfgid ";
         res += FORMAT(("%u", id));
      }

      if (logfile) {
         res += " -logfile ";
         res += ResolveEnv(logfile);
      }

      res += FORMAT((" -nodeid %u -numnodes %u", id, NumNodes()));

      if (connstr!=0) res += FORMAT((" -conn %s", connstr));

   } else
   if (kind == kindConn) {
      // this is way to get connection string
      if (connstr==0) {
         res += " echo No connection string specified; exit 1";
      } else {
         if (workdir) res += FORMAT((" cd %s;", ResolveEnv(workdir).c_str()));
         res += FORMAT((" if [ -f %s ] ; then cat %s; rm -f %s; fi", connstr, connstr, connstr));
      }
   } else
   if (kind == kindKill) {
      res += " killall --quiet dabc_run";
   } else
   if (kind == kindStop) {
      res += " pkill -SIGINT dabc_run";
   }

   return res;
}

bool dabc::ConfigBase::HasContext(unsigned id)
{
   return IsXDAQ() ? (XDAQ_FindContext(id) != 0) : (FindContext(id) != 0);
}
