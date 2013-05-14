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

#ifndef DABC_ConfigBase
#define DABC_ConfigBase

#ifndef DABC_XmlEngine
#include "dabc/XmlEngine.h"
#endif


namespace dabc {

   extern const char* xmlRootNode;
   extern const char* xmlVersionAttr;
   extern const char* xmlContext;
   extern const char* xmlApplication;
   extern const char* xmlAppDfltName;
   extern const char* xmlDefualtsNode;
   extern const char* xmlVariablesNode;
   extern const char* xmlNameAttr;
   extern const char* xmlHostAttr;
   extern const char* xmlPortAttr;
   extern const char* xmlClassAttr;
   extern const char* xmlDeviceAttr;
   extern const char* xmlThreadAttr;
   extern const char* xmlUseacknAttr;
   extern const char* xmlOptionalAttr;
   extern const char* xmlTimeoutAttr;
   extern const char* xmlAutoAttr;
   extern const char* xmlValueAttr;
   extern const char* xmlRunNode;
   extern const char* xmlUserNode;
   extern const char* xmlSshUser;
   extern const char* xmlSshPort;
   extern const char* xmlSshInit;
   extern const char* xmlSshTest;
   extern const char* xmlSshTimeout;
   extern const char* xmlDABCSYS;
   extern const char* xmlDABCUSERDIR;
   extern const char* xmlDABCWORKDIR;
   extern const char* xmlDABCNODEID;
   extern const char* xmlDABCNUMNODES;
   extern const char* xmlActive;
   extern const char* xmlCopyCfg;
   extern const char* xmlStdOut;
   extern const char* xmlErrOut;
   extern const char* xmlNullOut;
   extern const char* xmlDebugger;
   extern const char* xmlWorkDir;
   extern const char* xmlDebuglevel;
   extern const char* xmlNoDebugPrefix;
   extern const char* xmlLogfile;
   extern const char* xmlLoglevel;
   extern const char* xmlLoglimit;
   extern const char* xmlRunTime;
   extern const char* xmlNormalMainThrd;
   extern const char* xmlNumSpecialThreads;
   extern const char* xmlThreadsLayout;
   extern const char* xmlTaskset;
   extern const char* xmlLDPATH;
   extern const char* xmlUserLib;
   extern const char* xmlInitFunc;
   extern const char* xmlRunFunc;
   extern const char* xmlCpuInfo;
   extern const char* xmlSocketHost;
   extern const char* xmlUseControl;

   class ConfigIO;

   class ConfigBase {

      friend class ConfigIO;

      protected:
         enum ESshArgsKinds { kindTest, kindCopy, kindStart, kindRun, kindStop, kindKill, kindGetlog, kindDellog };

         XMLDocPointer_t   fDoc;
         int               fVersion;  // -1 - error, 0 - xdaq, 1 and more - dabc
         XMLNodePointer_t  fVariables;

         // following variables work as 'environment'
         std::string       envDABCSYS;
         std::string       envDABCUSERDIR;     // dir with user plugin
         std::string       envDABCWORKDIR;     // dir where application is started
         std::string       envDABCNODEID;      // current node id (controlled node number)
         std::string       envDABCNUMNODES;    // current number of nodes
         std::string       envHost;            // host name of current context
         std::string       envContext;         // name of current context

         XMLNodePointer_t RootNode();
         XMLNodePointer_t Variables();

         bool NodeMaskMatch(XMLNodePointer_t node, XMLNodePointer_t mask);

         XMLNodePointer_t FindItemMatch(XMLNodePointer_t& lastmatch,
                                        XMLNodePointer_t node,
                                        const char* sub1 = 0,
                                        const char* sub2 = 0,
                                        const char* sub3 = 0);

         XMLNodePointer_t FindMatch(XMLNodePointer_t lastmatch,
                                    XMLNodePointer_t node,
                                    const char* sub1 = 0,
                                    const char* sub2 = 0,
                                    const char* sub3 = 0);

         std::string Find1(XMLNodePointer_t node,
                           const std::string& dflt,
                           const char* sub1,
                           const char* sub2 = 0,
                           const char* sub3 = 0);

         std::string FindN(XMLNodePointer_t node,
                           XMLNodePointer_t& prev,
                           const char* sub1,
                           const char* sub2 = 0,
                           const char* sub3 = 0);

         std::string GetEnv(const char* name);

         static bool IsNodeName(XMLNodePointer_t node, const char* name);
         const char* GetAttr(XMLNodePointer_t node, const char* attr, const char* defvalue = 0);
         int  GetIntAttr(XMLNodePointer_t node, const char* attr, int defvalue = 0);

         std::string GetNodeValue(XMLNodePointer_t node);
         std::string GetAttrValue(XMLNodePointer_t node, const char* name);

         XMLNodePointer_t FindChild(XMLNodePointer_t node, const char* name);
         XMLNodePointer_t FindContext(unsigned id);

         static bool IsWildcard(const char* str);

         /** Identifies, if node can be identified as valid context - no any wildcards in names */
         static bool IsContextNode(XMLNodePointer_t node);

      public:
         ConfigBase(const char* fname = 0);
         ~ConfigBase();

         // following methods implemented for both XDAQ/native xml formats

         bool IsOk() const { return fVersion>=0; }

         int GetVersion() const { return fVersion; }

         /** returns number of nodes in xml file */
         unsigned NumNodes();

         /** returns nodename of specified context */
         std::string NodeName(unsigned id);

         /** returns communication port of specified context */
         int NodePort(unsigned id);

         /** returns configured (initial) state of the node */
         bool NodeActive(unsigned id);

         /** returns name of specified context */
         std::string ContextName(unsigned id);

         /** method used by run.sh script to produce command line */
         std::string SshArgs(unsigned id = 0, const char* skind = "run", const char* topcfgfile = 0, const char* topworkdir = 0);

         /** Replaces entries like ${name} be variable value */
         std::string ResolveEnv(const std::string& arg);
   };

}

#endif
