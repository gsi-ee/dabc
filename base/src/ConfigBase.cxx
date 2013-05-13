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

#include "dabc/ConfigBase.h"

#include <fnmatch.h>
#include <stdlib.h>
#include <string.h>

#include "dabc/logging.h"

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
   const char* xmlPortAttr         = "port";
   const char* xmlClassAttr        = "class";
   const char* xmlDeviceAttr       = "device";
   const char* xmlThreadAttr       = "thread";
   const char* xmlAutoAttr         = "auto";
   const char* xmlValueAttr        = "value";
   const char* xmlRunNode          = "Run";
   const char* xmlUserNode         = "User";
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
   const char* xmlActive           = "active";
   const char* xmlCopyCfg          = "copycfg";
   const char* xmlStdOut           = "stdout";
   const char* xmlErrOut           = "errout";
   const char* xmlNullOut          = "nullout";
   const char* xmlDebugger         = "debugger";
   const char* xmlWorkDir          = "workdir";
   const char* xmlDebuglevel       = "debuglevel";
   const char* xmlNoDebugPrefix    = "nodebugprefix";
   const char* xmlLogfile          = "logfile";
   const char* xmlLoglimit         = "loglimit";
   const char* xmlLoglevel         = "loglevel";
   const char* xmlRunTime          = "runtime";
   const char* xmlNormalMainThrd   = "normalmainthrd";
   const char* xmlNumSpecialThreads= "num_special_thrds";
   const char* xmlTaskset          = "taskset";
   const char* xmlLDPATH           = "LD_LIBRARY_PATH";
   const char* xmlUserLib          = "lib";
   const char* xmlInitFunc         = "func";
   const char* xmlRunFunc          = "runfunc";
   const char* xmlCpuInfo          = "cpuinfo";
   const char* xmlSocketHost       = "sockethost";
   const char* xmlUseControl       = "control";
}

dabc::ConfigBase::ConfigBase(const char* fname) :
   fDoc(0),
   fVersion(-1),
   envDABCSYS(),
   envDABCUSERDIR(),
   envDABCNODEID(),
   envContext()
{
   if (fname==0) return;

   fDoc = Xml::ParseFile(fname, true);

   XMLNodePointer_t rootnode = Xml::DocGetRootElement(fDoc);

   fVariables = 0;

   if (IsNodeName(rootnode, xmlRootNode)) {
      fVersion = GetIntAttr(rootnode, xmlVersionAttr, 1);
   } else {
      EOUT("Xml file %s not in dabc format", fname);
      Xml::FreeDoc(fDoc);
      fDoc = 0;
      fVersion = -1;
   }
}

dabc::ConfigBase::~ConfigBase()
{
   Xml::FreeDoc(fDoc);
   fDoc = 0;
}

dabc::XMLNodePointer_t dabc::ConfigBase::Variables()
{
   if ((fDoc==0) || (fVariables!=0)) return fVariables;

   fVariables = 0;
   XMLNodePointer_t rootnode = Xml::DocGetRootElement(fDoc);
   if (rootnode==0) return 0;

   XMLNodePointer_t node = Xml::GetChild(rootnode);
   while (node!=0) {
      if (IsNodeName(node, xmlVariablesNode)) break;
      node = Xml::GetNext(node);
   }

   fVariables = node;
   return fVariables;
}


bool dabc::ConfigBase::IsNodeName(XMLNodePointer_t node, const char* name)
{
   if ((node==0) || (name==0)) return false;

   const char* n = Xml::GetNodeName(node);

   return n==0 ? false : strcmp(n,name) == 0;
}

const char* dabc::ConfigBase::GetAttr(XMLNodePointer_t node, const char* attr, const char* defvalue)
{
   const char* res = Xml::GetAttr(node, attr);
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
   XMLNodePointer_t child = Xml::GetChild(node);
   while (child!=0) {
      if (IsNodeName(child, name)) return child;
      child = Xml::GetNext(child);
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

   XMLNodePointer_t subnode = Xml::GetChild(node);
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

      subnode = Xml::GetNext(subnode);
   }

   return 0;
}

bool dabc::ConfigBase::NodeMaskMatch(XMLNodePointer_t node, XMLNodePointer_t mask)
{
   if ((node==0) || (mask==0)) return false;

   if (!IsNodeName(node, Xml::GetNodeName(mask))) return false;

   // this code compares attributes of two nodes and verify that they are matching
   XMLAttrPointer_t xmlattr = Xml::GetFirstAttr(node);

   bool isanyattr(xmlattr!=0), isanywildcard(false);

   while (xmlattr!=0) {

      const char* attrname = Xml::GetAttrName(xmlattr);

      const char* value = Xml::GetAttr(node, attrname);

      const char* maskvalue = Xml::GetAttr(mask, attrname);

      if (IsWildcard(maskvalue)) isanywildcard = true;

      if ((value!=0) && (maskvalue!=0))
         if (fnmatch(maskvalue, value, FNM_NOESCAPE)!=0) return false;

      xmlattr = Xml::GetNextAttr(xmlattr);
   }

   // it is required that matching nodes should contain at least one attribute with wildcard,
   // otherwise we will enter main node itself again and again
   if (isanyattr && !isanywildcard) return false;

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


   XMLNodePointer_t subfolder = Xml::GetChild(RootNode());

   while (subfolder!=0) {
      if (NodeMaskMatch(node, subfolder)) {

         nextmatch = FindItemMatch(lastmatch, subfolder, sub1, sub2, sub3);

         if (nextmatch!=0) return nextmatch;
      }

      subfolder = Xml::GetNext(subfolder);
   }

   return 0;
}

std::string dabc::ConfigBase::GetNodeValue(XMLNodePointer_t node)
{
   if (node==0) return std::string();
   const char* value = Xml::GetAttr(node, xmlValueAttr);
   if (value==0) value = Xml::GetNodeContent(node);
   return ResolveEnv(value ? value : "");
}

std::string dabc::ConfigBase::GetAttrValue(XMLNodePointer_t node, const char* name)
{
   const char* res = Xml::GetAttr(node, name);
   if (res==0) return std::string();
   return ResolveEnv(res);
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

   XMLNodePointer_t rootnode = Xml::DocGetRootElement(fDoc);
   if (rootnode==0) return 0;
   XMLNodePointer_t node = Xml::GetChild(rootnode);
   unsigned cnt = 0;
   while (node!=0) {
      if (IsContextNode(node)) cnt++;
      node = Xml::GetNext(node);
   }
   return cnt;
}

std::string dabc::ConfigBase::NodeName(unsigned id)
{
   XMLNodePointer_t contnode = FindContext(id);
   if (contnode == 0) return std::string("");
   const char* host = Xml::GetAttr(contnode, xmlHostAttr);
   if (host == 0) return std::string("");
   return ResolveEnv(host);
}

int dabc::ConfigBase::NodePort(unsigned id)
{
   XMLNodePointer_t contnode = FindContext(id);
   if (contnode == 0) return 0;
   const char* sbuf = Xml::GetAttr(contnode, xmlPortAttr);
   if (sbuf == 0) return 0;
   std::string s = ResolveEnv(sbuf);
   if (s.empty()) return 0;
   int res = 0;
   if (!dabc::str_to_int(s.c_str(), &res)) res = 0;
   return res;
}

bool dabc::ConfigBase::NodeActive(unsigned id)
{
   XMLNodePointer_t contnode = FindContext(id);
   if (contnode == 0) return false;

   return Find1(contnode, "", xmlRunNode, xmlActive) != "false";
}

std::string dabc::ConfigBase::ContextName(unsigned id)
{
   XMLNodePointer_t contnode = FindContext(id);
   std::string oldhost = envHost;
   envHost = NodeName(id);
   const char* name = Xml::GetAttr(contnode, xmlNameAttr);
   std::string res = name ? ResolveEnv(name) : "";
   if (res.empty()) res = envHost;
   envHost = oldhost;
   return res;
}

bool dabc::ConfigBase::IsWildcard(const char* str)
{
   if (str==0) return false;

   return (strchr(str,'*')!=0) || (strchr(str,'?')!=0);
}

dabc::XMLNodePointer_t dabc::ConfigBase::RootNode()
{
   if (fDoc==0) return 0;
   return Xml::DocGetRootElement(fDoc);
}

bool dabc::ConfigBase::IsContextNode(XMLNodePointer_t node)
{
   if (node==0) return false;

   if (!IsNodeName(node, xmlContext)) return false;

   const char* host = Xml::GetAttr(node, xmlHostAttr);
   if (IsWildcard(host)) return false;

   const char* name = Xml::GetAttr(node, xmlNameAttr);

   if (IsWildcard(name)) return false;

   return true;
}


dabc::XMLNodePointer_t dabc::ConfigBase::FindContext(unsigned id)
{
   if (fDoc==0) return 0;
   XMLNodePointer_t rootnode = Xml::DocGetRootElement(fDoc);
   if (rootnode==0) return 0;
   XMLNodePointer_t node = Xml::GetChild(rootnode);
   unsigned cnt = 0;

   while (node!=0) {
      if (IsContextNode(node)) {
         if (cnt++ == id) return node;
      }
      node = Xml::GetNext(node);
   }

   return 0;
}


std::string dabc::ConfigBase::ResolveEnv(const std::string& arg)
{
   if (arg.empty()) return arg;

   std::string name = arg;

   XMLNodePointer_t vars = Variables();

   size_t pos1, pos2;

   while ((pos1 = name.find("${")) != name.npos) {

      pos2 = name.find("}");

      if ((pos1>pos2) || (pos2==name.npos)) {
         EOUT("Wrong variable parenthesis %s", arg.c_str());
         return arg;
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

std::string dabc::ConfigBase::SshArgs(unsigned id, const char* skind, const char* topcfgfile, const char* topworkdir)
{
   if (skind==0) skind = "test";

   int kind = -1;
   if (strcmp(skind, "kill")==0) kind = kindKill; else
   if (strcmp(skind, "copycfg")==0) kind = kindCopy; else
   if (strcmp(skind, "start")==0) kind = kindStart; else
   if (strcmp(skind, "stop")==0) kind = kindStop; else
   if (strcmp(skind, "test")==0) kind = kindTest; else
   if (strcmp(skind, "run")==0) kind = kindRun; else
   if (strcmp(skind, "getlog")==0) kind = kindGetlog; else
   if (strcmp(skind, "dellog")==0) kind = kindDellog; else

   if (kind<0) return std::string("");

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
   bool copycfg = (Find1(contnode, "", xmlRunNode, xmlCopyCfg) == "true");
   std::string dabc_stdout = Find1(contnode, "", xmlRunNode, xmlStdOut);
   std::string dabc_errout = Find1(contnode, "", xmlRunNode, xmlErrOut);
   bool nullout = (Find1(contnode, "", xmlRunNode, xmlNullOut) == "true");
   std::string tasksetargs = Find1(contnode, "", xmlRunNode, xmlTaskset);
   std::string logfile = Find1(contnode, "", xmlRunNode, xmlLogfile);

   std::string workdir = envDABCWORKDIR;
   std::string dabcsys = envDABCSYS;
   std::string userdir = envDABCUSERDIR;

   std::string envdabcsys = GetEnv("DABCSYS");

   if (topcfgfile==0) {
      EOUT("Config file not defined");
      return std::string("");
   }

   if (workdir.empty()) workdir = topworkdir;

   std::string workcfgfile = topcfgfile;
   if (copycfg)
       workcfgfile = dabc::format("node%03u_%s", id, topcfgfile);

   std::string copycmd, logcmd;

   bool backgr(false), addcopycmd(false);

   if (copycfg) {
      copycmd = "scp -q ";
      if (!portid.empty())
         copycmd += dabc::format("-P %s ", portid.c_str());

      copycmd += dabc::format("%s ", topcfgfile);

      if (!username.empty()) {
         copycmd += username;
         copycmd += "@";
      }

      copycmd += hostname;
      copycmd += ":";

      if (!workdir.empty()) {
         copycmd += workdir;
         if (workdir[workdir.length()-1] != '/') copycmd += "/";
      }

      copycmd += workcfgfile;
   }

   logcmd = "ssh -x ";

   if (!portid.empty()) {
      logcmd += "-p ";
      logcmd += portid;
      logcmd += " ";
   }

   if (!timeout.empty()) {
      logcmd += "-o ConnectTimeout=";
      logcmd += timeout;
      logcmd += " ";
   }

   if (!username.empty()) {
      logcmd += username;
      logcmd += "@";
   }

   logcmd += hostname;

   if (kind == kindTest) {
      // this is a way to get test arguments

      if (!initcmd.empty()) res += dabc::format("%s; ", initcmd.c_str());

      if (!testcmd.empty()) res += dabc::format("%s; ", testcmd.c_str());

      if (!dabcsys.empty())
         res += dabc::format("if [[ ! -d %s ]] ; then echo DABCSYS = %s missed; exit 11; fi; ", dabcsys.c_str(), dabcsys.c_str());
      else
      if (!envdabcsys.empty())
         res += dabc::format("if [[ (( \\${#DABCSYS}==0 )) && (! -d %s) ]] ; then echo DABCSYS = %s missed; exit 11; fi; ", envdabcsys.c_str(), envdabcsys.c_str());
      else
         res += "if (( \\${#DABCSYS}==0 )) ; then echo DABCSYS not specified; exit 7; fi; ";

      if (!workdir.empty())
         res += dabc::format(" if [[ ! -d %s ]] ; then echo workdir = %s missed; exit 12; fi; ", workdir.c_str(), workdir.c_str());

      if (!copycfg && !workcfgfile.empty()) {
         if (!workdir.empty()) res += dabc::format(" cd %s;", workdir.c_str());
         res += dabc::format(" if [[ ! -f %s ]] ; then echo cfgfile = %s missed; exit 12; fi; ", workcfgfile.c_str(), workcfgfile.c_str());
      }

      res += dabc::format(" echo Test on node %s done;", hostname.c_str());

   } else

   if (kind == kindCopy) {
     // copy in extra config file

     if (copycmd.empty()) logcmd = "echo noop";
                     else logcmd = copycmd;

   } else

   if ((kind == kindStart) || (kind == kindRun)) {

      // add config copy command at the very beginning

      if (copycfg && !copycmd.empty()) addcopycmd = true;

      // this is main run arguments

      if (!initcmd.empty()) res += dabc::format("%s; ", initcmd.c_str());

      std::string ld;
      if (!ldpath.empty()) ld += ldpath;
      if (!userdir.empty()) { if (!ld.empty()) ld += ":"; ld += userdir; }

      if (!dabcsys.empty()) {
         res += dabc::format("export DABCSYS=%s;", dabcsys.c_str());
         res += " export LD_LIBRARY_PATH=";
         if (!ld.empty()) { res += ld; res += ":"; }
         res += "\\$DABCSYS/lib:\\$LD_LIBRARY_PATH;";
      } else
      if (!envdabcsys.empty()) {
         res += dabc::format( "if [[ (( \\${#DABCSYS}==0 )) && -d %s ]] ; then export DABCSYS=%s;", envdabcsys.c_str(), envdabcsys.c_str());
         res += " export LD_LIBRARY_PATH=";
         if (!ld.empty()) { res += ld; res += ":"; }
         res += "\\$DABCSYS/lib:\\$LD_LIBRARY_PATH; fi;";
      } else
      if (!ld.empty()) {
         res += " if (( \\${#DABCSYS}==0 )) ; then echo DABCSYS not specified; exit 7; fi;";
         res += dabc::format(" export LD_LIBRARY_PATH=%s:\\$LD_LIBRARY_PATH;", ld.c_str());
      }

      if (!workdir.empty()) res += dabc::format(" cd %s;", workdir.c_str());

      if (!tasksetargs.empty() && (tasksetargs!="false")) {
         res += " taskset ";
         res += tasksetargs;
      }

      if (!debugger.empty() && (debugger!="false")) {
         if (debugger == "true")
            res += " gdb -q -x \\$DABCSYS/base/run/gdbcmd.txt --args";
         else {
            res += " ";
            res += debugger;
         }
      }

      res += dabc::format(" \\$DABCSYS/bin/dabc_exe %s -nodeid %u -numnodes %u",
                           workcfgfile.c_str(), id, NumNodes());

      if (kind == kindRun) res += " -run";

      if (nullout) {
         res += " >/dev/null 2>/dev/null";
      } else {
         if (!dabc_stdout.empty()) { res += " >"; res+=dabc_stdout; }
         if (!dabc_errout.empty()) { res += " 2>"; res+=dabc_errout; }
      }

      backgr = true;

   } else

   if ((kind == kindKill) || (kind == kindStop)) {
      if (copycfg && !workcfgfile.empty()) {
         if (!workdir.empty()) res += dabc::format(" cd %s;", workdir.c_str());
         res += dabc::format(" rm -f %s;", workcfgfile.c_str());
      }

      if (kind == kindKill)
         res += dabc::format(" killall --quiet dabc_exe; echo Kill on node %s done;", hostname.c_str());
      else
         res += dabc::format(" pkill -SIGINT dabc_exe; echo Stop on node %s done;", hostname.c_str());
   } else

   if (kind == kindGetlog) {

      if (logfile.empty())
        logcmd = dabc::format("echo no logfile on node id %u  host %u", id, hostname.c_str());
     else {

        addcopycmd = true;
        copycmd = dabc::format("echo Copy logfile %s from node %s", logfile.c_str(), hostname.c_str());

        logcmd = "scp -q ";
        if (!portid.empty())
            logcmd += dabc::format("-P %s ", portid.c_str());

        if (!username.empty()) {
           logcmd += username;
           logcmd += "@";
        }

        logcmd += hostname;
        logcmd += ":";

        // add path to workdir if logfile defined whithout absolute path
        if (!workdir.empty() && (logfile[0]!='/')) {
          logcmd += workdir;
          if (workdir[workdir.length()-1] != '/') logcmd += "/";
        }

        logcmd += logfile;

        logcmd += " .";
      }
   } else

   if (kind == kindDellog) {

      if (!workdir.empty()) res += dabc::format(" cd %s;", workdir.c_str());

      if (logfile.empty())
        res += dabc::format(" echo no logfile on node %s;", hostname.c_str());
      else
        res += dabc::format(" rm -f %s; echo Del logfile %s on node %s", logfile.c_str(), logfile.c_str(), hostname.c_str());
   }

   if (!res.empty())
      logcmd = logcmd + " " + res;

   if (backgr)
      logcmd = std::string("&") + logcmd;

   // when many commands, use bash format ([0]="arg1" [1]="arg2")
   if (addcopycmd)
      logcmd = std::string("([0]=\"") + copycmd + "\" [1]=\"" + logcmd + "\")";
   else
      logcmd = std::string("\"") + logcmd + "\"";

   return logcmd;
}
