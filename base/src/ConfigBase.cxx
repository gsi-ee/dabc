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
#include "dabc/ConfigBase.h"

#include <fnmatch.h>
#include <stdlib.h>
#include <string.h>

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

std::string dabc::ConfigBase::XDAQ_ContextName(unsigned instance)
{
   XMLNodePointer_t contextnode = XDAQ_FindContext(instance);
   if (contextnode==0) return std::string("");
   const char* url = contextnode ? fXml.GetAttr(contextnode, xmlXDAQurlattr) : 0;
   if ((url==0) || (strstr(url,"http://")!=url)) return std::string("");
   return std::string(url+7);
}

std::string dabc::ConfigBase::XDAQ_NodeName(unsigned instance)
{
   std::string ctxt = XDAQ_ContextName(instance);
   size_t pos = ctxt.find(":");
   if (pos==ctxt.npos) return ctxt;
   return ctxt.substr(0, pos);
}

std::string dabc::ConfigBase::XDAQ_SshArgs(unsigned instance, int ctrlkind, int kind, const char* topcfgfile, const char* topworkdir, const char* connstr)
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
   envContext = XDAQ_ContextName(instance);

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
      res += " . $DABCSYS/script/nodelogin.sh;";
      res += FORMAT((" cd %s;", topworkdir));
      res += FORMAT((" $DABCSYS/bin/dabc_run %s -cfgid %u -nodeid %u -numnodes %u", topcfgfile, instance, instance, NumNodes()));
      if (ctrlkind == kindSctrl)
         res += " -sctrl ";
      else
      if (ctrlkind == kindDim) {
         std::string dimnode = GetEnv(xmlDIM_DNS_NODE);
         if (dimnode.empty()) {
            EOUT(("Cannot identify dim node"));
            return std::string("");
         }
         res += " -dim ";
         res += dimnode;
      }

      if ((connstr!=0) && (ctrlkind != kindDim))
         res += FORMAT((" -conn %s", connstr));
   } else
   if (kind == kindConn) {
      // this is way to get connection string
      if ((ctrlkind != kindDim) && (instance==0)) {
         if (connstr==0) {
            res += " echo No connection string specified; exit 1";
         } else {
            res += FORMAT((" cd %s;", topworkdir));
            res += FORMAT((" if [ -f %s ] ; then cat %s; rm -f %s; fi", connstr, connstr, connstr));
         }
      } else
         res = "";
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
   const char* xmlContext          = "Context";
   const char* xmlApplication      = "Application";
   const char* xmlAppDfltName      = "App";
   const char* xmlDefualtsNode     = "Defaults";
   const char* xmlVariablesNode    = "Variables";
   const char* xmlNameAttr         = "name";
   const char* xmlHostAttr         = "host";
   const char* xmlClassAttr        = "class";
   const char* xmlValueAttr        = "value";
   const char* xmlRunNode          = "Run";
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
   const char* xmlDebugger         = "debugger";
   const char* xmlWorkDir          = "workdir";
   const char* xmlDebuglevel       = "debuglevel";
   const char* xmlLogfile          = "logfile";
   const char* xmlLoglimit         = "loglimit";
   const char* xmlLoglevel         = "loglevel";
   const char* xmlParslevel        = "parslevel";
   const char* xmlLDPATH           = "LD_LIBRARY_PATH";
   const char* xmlConfigFile       = "config";
   const char* xmlConfigFileId     = "configid";
   const char* xmlUserLib          = "lib";
   const char* xmlInitFunc         = "func";
   const char* xmlRunFunc          = "runfunc";
   const char* xmlCpuInfo          = "cpuinfo";
   const char* xmlControlled       = "ctrl";
   const char* xmlDIM_DNS_NODE     = "DIM_DNS_NODE";
   const char* xmlDIM_DNS_PORT     = "DIM_DNS_PORT";

}

dabc::ConfigBase::ConfigBase(const char* fname) :
   fXml(),
   fDoc(0),
   fVersion(-1),
   fPrnt(0),
   envDABCSYS(),
   envDABCUSERDIR(),
   envDABCNODEID(),
   envContext()
{
   if (fname==0) return;

   fDoc = fXml.ParseFile(fname, true);

   XMLNodePointer_t rootnode = fXml.DocGetRootElement(fDoc);

   fDflts = 0;
   fVariables = 0;

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
   fDoc = 0;
}

dabc::XMLNodePointer_t dabc::ConfigBase::Dflts()
{
   if ((fDoc==0) || (fDflts!=0)) return fDflts;

   fDflts = 0;
   XMLNodePointer_t rootnode = fXml.DocGetRootElement(fDoc);
   if (rootnode==0) return 0;

   XMLNodePointer_t node = fXml.GetChild(rootnode);
   while (node!=0) {
      if (IsNodeName(node, xmlDefualtsNode)) break;
      node = fXml.GetNext(node);
   }

   fDflts = node;
   return fDflts;
}

dabc::XMLNodePointer_t dabc::ConfigBase::Variables()
{
   if ((fDoc==0) || (fVariables!=0)) return fVariables;

   fVariables = 0;
   XMLNodePointer_t rootnode = fXml.DocGetRootElement(fDoc);
   if (rootnode==0) return 0;

   XMLNodePointer_t node = fXml.GetChild(rootnode);
   while (node!=0) {
      if (IsNodeName(node, xmlVariablesNode)) break;
      node = fXml.GetNext(node);
   }

   fVariables = node;
   return fVariables;
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
   if (node==0) return 0;
   XMLNodePointer_t child = fXml.GetChild(node);
   while (child!=0) {
      if (IsNodeName(child, name)) return child;
      child = fXml.GetNext(child);
   }
   return 0;
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


dabc::XMLNodePointer_t dabc::ConfigBase::FindMatch(XMLNodePointer_t lastmatch,
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

      XMLNodePointer_t subfolder = cfg->fXml.GetChild(cfg->Dflts());

      while (subfolder!=0) {
         if (cfg->NodeMaskMatch(node, subfolder)) {

            nextmatch = cfg->FindItemMatch(lastmatch, subfolder, sub1, sub2, sub3);

            if (nextmatch!=0) return nextmatch;
         }

         subfolder = cfg->fXml.GetNext(subfolder);
      }

      cfg = cfg->fPrnt;
   } while (cfg!=0);

   return 0;
}

std::string dabc::ConfigBase::GetNodeValue(XMLNodePointer_t node)
{
   if (node==0) return std::string();
   const char* value = fXml.GetAttr(node, xmlValueAttr);
   if (value==0) value = fXml.GetNodeContent(node);
   return ResolveEnv(value ? value : "");
}

/** Search first entry for item sub1/sub2/sub3 in specified node
 *  First tries full path, than found sub1 and path sub2/sub3
 */

std::string dabc::ConfigBase::Find1(XMLNodePointer_t node,
                                    const std::string& dflt,
                                    const char* sub1,
                                    const char* sub2,
                                    const char* sub3)
{
   XMLNodePointer_t res = FindMatch(0, node, sub1, sub2, sub3);
   if (res==0) {
      if (sub2!=0) {
         XMLNodePointer_t child = FindChild(node, sub1);
         if (child) return Find1(child, dflt, sub2, sub3);
      }
      return dflt;
   }
   std::string cont = GetNodeValue(res);
   return cont.empty() ? dflt : cont;
}

std::string dabc::ConfigBase::FindN(XMLNodePointer_t node,
                                    XMLNodePointer_t& prev,
                                    const char* sub1,
                                    const char* sub2,
                                    const char* sub3)
{
   XMLNodePointer_t res = FindMatch(prev, node, sub1, sub2, sub3);
   prev = res;
   if (res==0) return std::string();
   return GetNodeValue(res);
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
      if (IsNodeName(node, xmlContext)) cnt++;
      node = fXml.GetNext(node);
   }
   return cnt;
}

unsigned dabc::ConfigBase::NumControlNodes()
{
   if (!IsOk()) return 0;
   if (IsXDAQ()) return XDAQ_NumNodes();

   XMLNodePointer_t rootnode = fXml.DocGetRootElement(fDoc);
   if (rootnode==0) return 0;
   XMLNodePointer_t node = fXml.GetChild(rootnode);
   unsigned cnt = 0;
   while (node!=0) {
      if (IsNodeName(node, xmlContext))
         if (Find1(node, "true", xmlRunNode, xmlControlled) == "true") cnt++;
      node = fXml.GetNext(node);
   }
   return cnt;
}

unsigned dabc::ConfigBase::ControlSequenceId(unsigned id)
{
   if (!IsOk()) return 0;
   if (IsXDAQ()) return id;

   XMLNodePointer_t rootnode = fXml.DocGetRootElement(fDoc);
   if (rootnode==0) return id;
   XMLNodePointer_t node = fXml.GetChild(rootnode);
   unsigned cnt = 0;
   unsigned ctrlcnt = 0;
   while (node!=0) {
      if (IsNodeName(node, xmlContext)) {
         bool isctrl = (Find1(node, "true", xmlRunNode, xmlControlled) == "true");

         if (isctrl) ctrlcnt++;

         if (cnt++==id) return isctrl ? ctrlcnt : 0;
      }
      node = fXml.GetNext(node);
   }
   return id;
}

std::string dabc::ConfigBase::NodeName(unsigned id)
{
   if (IsXDAQ()) return XDAQ_NodeName(id);
   XMLNodePointer_t contnode = FindContext(id);
   if (contnode == 0) return std::string("");
   const char* host = fXml.GetAttr(contnode, xmlHostAttr);
   if (host == 0) return std::string("");
   return ResolveEnv(host);
}

std::string dabc::ConfigBase::ContextName(unsigned id)
{
   if (IsXDAQ()) return XDAQ_ContextName(id);
   XMLNodePointer_t contnode = FindContext(id);
   std::string oldhost = envHost;
   envHost = NodeName(id);
   const char* name = fXml.GetAttr(contnode, xmlNameAttr);
   std::string res = name ? ResolveEnv(name) : "";
   if (res.empty()) res = envHost;
   envHost = oldhost;
   return res;
}

dabc::XMLNodePointer_t dabc::ConfigBase::FindContext(unsigned id)
{
   if (fDoc==0) return 0;
   XMLNodePointer_t rootnode = fXml.DocGetRootElement(fDoc);
   if (rootnode==0) return 0;
   XMLNodePointer_t node = fXml.GetChild(rootnode);
   unsigned cnt = 0;

   while (node!=0) {
      if (IsNodeName(node, xmlContext))
         if (cnt++ == id) return node;
      node = fXml.GetNext(node);
   }

   return 0;
}


std::string dabc::ConfigBase::ResolveEnv(const std::string& arg)
{
   if (arg.empty()) return arg;

   std::string name = arg;

   XMLNodePointer_t vars = Variables();

   while (name.find("${") != name.npos) {

      size_t pos1 = name.find("${");
      size_t pos2 = name.find("}");

      if ((pos1>pos2) || (pos2==name.npos)) {
         EOUT(("Wrong variable parenthesis %s", arg.c_str()));
         return std::string(arg);
      }

      std::string var(name, pos1+2, pos2-pos1-2);

      name.erase(pos1, pos2-pos1+1);

      if (var.length()>0) {
         std::string value;

         XMLNodePointer_t node = FindChild(vars, var.c_str());
         if (node!=0) value = GetNodeValue(node);

         if (value.empty()){
            if (var==xmlDABCSYS) value = envDABCSYS; else
            if (var==xmlDABCUSERDIR) value = envDABCUSERDIR; else
            if (var==xmlDABCWORKDIR) value = envDABCWORKDIR; else
            if (var==xmlDABCNODEID) value = envDABCNODEID; else
            if (var==xmlDABCNUMNODES) value = envDABCNUMNODES; else
            if (var==xmlHostAttr) value = envHost; else
            if (var==xmlContext) value = envContext;
         }
         if (value.empty()) value = GetEnv(var.c_str());

         if (!value.empty()) name.insert(pos1, value);
      }
   }
   return name;
}

std::string dabc::ConfigBase::GetEnv(const char* name)
{
   const char* env = getenv(name);
   return std::string(env ? env : "");
}

std::string dabc::ConfigBase::SshArgs(unsigned id, int ctrlkind, const char* skind, const char* topcfgfile, const char* topworkdir, const char* connstr)
{
   if (skind==0) skind = "test";

   int kind = -1;
   if (strcmp(skind, "kill")==0) kind = kindKill; else
   if (strcmp(skind, "start")==0) kind = kindStart; else
   if (strcmp(skind, "stop")==0) kind = kindStop; else
   if (strcmp(skind, "test")==0) kind = kindTest; else
   if (strcmp(skind, "run")==0) kind = kindRun; else
   if (strcmp(skind, "conn")==0) kind = kindConn;

   if (kind<0) return std::string("");

   if (IsXDAQ()) return XDAQ_SshArgs(id, ctrlkind, kind, topcfgfile, topworkdir, connstr);

   std::string res;
   XMLNodePointer_t contnode = FindContext(id);
   if (contnode == 0) return std::string("");

   envDABCNODEID = dabc::format("%u", id);
   envDABCNUMNODES = dabc::format("%u", NumNodes());

   envHost = NodeName(id);
   envContext = ContextName(id);

   envDABCSYS = Find1(contnode, "", xmlRunNode, xmlDABCSYS);
   envDABCUSERDIR = Find1(contnode, "", xmlRunNode, xmlDABCUSERDIR);
   envDABCWORKDIR = Find1(contnode, "", xmlRunNode, xmlWorkDir);

   std::string hostname = envHost;
   if (hostname.empty()) hostname = "localhost";

   std::string username = Find1(contnode, "", xmlRunNode, xmlSshUser);
   std::string portid = Find1(contnode, "", xmlRunNode, xmlSshPort);
   std::string timeout = Find1(contnode, "", xmlRunNode, xmlSshTimeout);
   std::string initcmd = Find1(contnode, "", xmlRunNode, xmlSshInit);
   std::string testcmd = Find1(contnode, "", xmlRunNode, xmlSshTest);

   std::string debugger = Find1(contnode, "", xmlRunNode, xmlDebugger);
   std::string ldpath = Find1(contnode, "", xmlRunNode, xmlLDPATH);
   std::string cfgfile = Find1(contnode, "", xmlRunNode, xmlConfigFile);
   std::string cfgid = Find1(contnode, "", xmlRunNode, xmlConfigFileId);

   std::string workdir = envDABCWORKDIR;
   std::string dabcsys = envDABCSYS;
   std::string userdir = envDABCUSERDIR;

   std::string envdabcsys = GetEnv("DABCSYS");

   if ((topcfgfile==0) && cfgfile.empty()) {
      EOUT(("Config file not defined"));
      return std::string("");
   }

//  char currdir[1024];
// if (!getcwd(currdir, sizeof(currdir))) strcpy(currdir,".");

   if (workdir.empty()) workdir = topworkdir;

   res = "ssh -x ";

   if (!portid.empty()) {
      res += "-p ";
      res += portid;
      res += " ";
   }

   if (!timeout.empty()) {
      res += "-o ConnectTimeout=";
      res += timeout;
      res += " ";
   }

   if (!username.empty()) {
      res += username;
      res += "@";
   }

   res += hostname;

   if (kind == kindTest) {
      // this is a way to get test arguments

//      res += FORMAT ((" . gsi_environment.sh; echo $HOST - %s; ls /data.local1; exit 243", hostname));

      if (!initcmd.empty()) res += dabc::format(" %s;", initcmd.c_str());

      if (!testcmd.empty()) res += dabc::format(" %s;", testcmd.c_str());

      if (!dabcsys.empty())
         res += dabc::format(" if [ ! -d %s ] ; then echo DABCSYS = %s missed; exit 11; fi; ", dabcsys.c_str(), dabcsys.c_str());
      else
      if (!envdabcsys.empty())
         res += dabc::format(" if [ z $DABSYS -a ! -d %s ] ; then echo DABCSYS = %s missed; exit 11; fi; ", envdabcsys.c_str(), envdabcsys.c_str());
      else
         res += " if [ z $DABCSYS ] ; then echo DABCSYS not specified; exit 7; fi;";

      if (!workdir.empty()) {
         res += dabc::format(" if [ ! -d %s ] ; then echo workdir = %s missed; exit 12; fi; ", workdir.c_str(), workdir.c_str());
         res += dabc::format(" cd %s;", workdir.c_str());
      }

      if (!cfgfile.empty()) {
         res += dabc::format(" if [ ! -f %s ] ; then echo cfgfile = %s missed; exit 12; fi; ", cfgfile.c_str(), cfgfile.c_str());
      } else
         res += dabc::format(" if [ ! -f %s ] ; then echo maincfgfile = %s missed; exit 12; fi; ", topcfgfile, topcfgfile);

      res += dabc::format(" echo Test on node %s done;", hostname.c_str());

   } else

   if ((kind == kindStart) || (kind == kindRun)) {
      // this is main run arguments

      if (!initcmd.empty()) res += dabc::format(" %s;", initcmd.c_str());

      std::string ld;
      if (!ldpath.empty()) ld += ldpath;
      if (!userdir.empty()) { if (!ld.empty()) ld += ":"; ld += userdir; }

      if (!dabcsys.empty()) {
         res += dabc::format(" export DABCSYS=%s;", dabcsys.c_str());
         res += " export LD_LIBRARY_PATH=";
         if (!ld.empty()) { res += ld; res += ":"; }
         res += "$DABCSYS/lib:$LD_LIBRARY_PATH;";
      } else
      if (!envdabcsys.empty()) {
         res += dabc::format( " if [ z $DABCSYS -a -d %s ] ; then export DABCSYS=%s;", envdabcsys.c_str(), envdabcsys.c_str());
         res += " export LD_LIBRARY_PATH=";
         if (!ld.empty()) { res += ld; res += ":"; }
         res += "$DABCSYS/lib:$LD_LIBRARY_PATH; fi;";
      } else
      if (!ld.empty()) {
         res += " if [ z $DABCSYS ] ; then echo DABCSYS not specified; exit 7; fi;";
         res += dabc::format(" export LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH;", ld.c_str());
      }

      if (!workdir.empty()) res += dabc::format(" cd %s;", ResolveEnv(workdir).c_str());

      if ((connstr!=0) && (ControlSequenceId(id)==1) && (ctrlkind != kindDim))
         res += dabc::format(" rm -f %s;", connstr);

      if (!debugger.empty()) {
         res += " ";
         res += debugger;
      }

      res += " $DABCSYS/bin/dabc_run ";

      if (!cfgfile.empty()) {
         res += cfgfile;
         if (!cfgid.empty()) {
            res += " -cfgid ";
            res += cfgid;
         }
      } else {
         res += topcfgfile;
         res += " -cfgid ";
         res += dabc::format("%u", id);
      }

      unsigned ctrldid = ControlSequenceId(id);
      if (ctrldid>0) ctrldid--;
               else ctrlkind = kindNone;

      if (ctrlkind == kindSctrl)
         res += " -sctrl ";
      else
      if (ctrlkind == kindDim) {
         std::string dimnode = Find1(contnode, "", xmlRunNode, xmlDIM_DNS_NODE);
         std::string dimport = Find1(contnode, "", xmlRunNode, xmlDIM_DNS_PORT);
         if (dimnode.empty())
            dimnode = GetEnv(xmlDIM_DNS_NODE);
         if (dimnode.empty()) {
            EOUT(("Cannot identify dim node"));
            return std::string("");
         }
         res += " -dim ";
         res += dimnode;
         if (!dimport.empty()) {
            res += ":";
            res += dimport;
         }
         res += " ";
      }

      res += dabc::format(" -nodeid %u -numnodes %u", ctrldid, NumControlNodes());

      if (ctrlkind != kindDim)
         if (connstr!=0) res += dabc::format(" -conn %s", connstr);

      if (kind == kindRun) res += " -run";
   } else

   if (kind == kindConn) {
      // this is way to get connection string
      if (connstr==0) {
         res += " echo No connection string specified; exit 1";
      } else
      if ((ControlSequenceId(id)==1) && (ctrlkind != kindDim)) {
         if (!workdir.empty()) res += dabc::format(" cd %s;", workdir.c_str());
         res += dabc::format(" if [ -f %s ] ; then cat %s; rm -f %s; fi", connstr, connstr, connstr);
      } else
         res = "";
   } else

   if (kind == kindKill) {
      res += dabc::format(" killall --quiet dabc_run; echo Kill on node %s done;", hostname.c_str());
   } else

   if (kind == kindStop) {
      res += dabc::format(" pkill -SIGINT dabc_run; echo Stop on node %s done;", hostname.c_str());
   }

   return res;
}
