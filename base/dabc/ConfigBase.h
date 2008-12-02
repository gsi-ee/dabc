#ifndef DABC_ConfigBase
#define DABC_ConfigBase

#ifndef DABC_XmlEngine
#include "dabc/XmlEngine.h"
#endif


namespace dabc {

   extern const char* xmlRootNode;
   extern const char* xmlVersionAttr;
   extern const char* xmlContNode;
   extern const char* xmlDefualtsNode;
   extern const char* xmlNameAttr;
   extern const char* xmlClassAttr;
   extern const char* xmlValueAttr;
   extern const char* xmlRunNode;
   extern const char* xmlSshHost;
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
   extern const char* xmlDebugger;
   extern const char* xmlWorkDir;
   extern const char* xmlLogfile;
   extern const char* xmlLDPATH;
   extern const char* xmlConfigFile;
   extern const char* xmlConfigFileId;
   extern const char* xmlUserLib;
   extern const char* xmlUserFunc;
   extern const char* xmlDIM_DNS_NODE;
   extern const char* xmlDIM_DNS_PORT;

   extern const char* xmlXDAQPartition;
   extern const char* xmlXDAQContext;
   extern const char* xmlXDAQApplication;
   extern const char* xmlXDAQproperties;
   extern const char* xmlXDAQinstattr;
   extern const char* xmlXDAQurlattr;
   extern const char* xmlXDAQModule;
   extern const char* xmlXDAQdebuglevel;

   class ConfigBase {
      protected:
         enum ESshArgsKinds { kindTest, kindStart, kindStop, kindKill, kindConn };

         XmlEngine         fXml;
         XMLDocPointer_t   fDoc;
         int               fVersion;  // -1 - error, 0 - xdaq, 1 and more - dabc
         ConfigBase       *fPrnt;  // parent configuration with defaults for some variables

         // following variables work as 'environment'
         std::string            envDABCSYS;
         std::string            envDABCUSERDIR;     // dir with user plugin
         std::string            envDABCWORKDIR;     // dir where application is started
         std::string            envDABCNODEID;      // current node id
         std::string            envDABCNUMNODES;    // current number of nodes

         XMLNodePointer_t Dflts();

         bool NodeMaskMatch(XMLNodePointer_t node, XMLNodePointer_t mask);

         XMLNodePointer_t FindItemMatch(XMLNodePointer_t& lastmatch,
                                        XMLNodePointer_t node,
                                        const char* sub1 = 0,
                                        const char* sub2 = 0,
                                        const char* sub3 = 0);

         XMLNodePointer_t FindNodesMatch(XMLNodePointer_t& lastmatch,
                                         XMLNodePointer_t searchfold,
                                         XMLNodePointer_t topnode,
                                         XMLNodePointer_t node,
                                         const char* sub1 = 0,
                                         const char* sub2 = 0,
                                         const char* sub3 = 0);

         XMLNodePointer_t FindMatch(XMLNodePointer_t lastmatch,
                                    XMLNodePointer_t topnode,
                                    XMLNodePointer_t node,
                                    const char* sub1 = 0,
                                    const char* sub2 = 0,
                                    const char* sub3 = 0);

         const char* Find1(XMLNodePointer_t node,
                           const char* dflt,
                           const char* sub1,
                           const char* sub2 = 0,
                           const char* sub3 = 0);

         const char* FindN(XMLNodePointer_t node,
                           XMLNodePointer_t& prev,
                           const char* sub1,
                           const char* sub2 = 0,
                           const char* sub3 = 0);


         bool IsNodeName(XMLNodePointer_t node, const char* name);
         const char* GetAttr(XMLNodePointer_t node, const char* attr, const char* defvalue = 0);
         int  GetIntAttr(XMLNodePointer_t node, const char* attr, int defvalue = 0);
         XMLNodePointer_t FindChild(XMLNodePointer_t node, const char* name);
         XMLNodePointer_t FindContext(unsigned id);

         XMLNodePointer_t XDAQ_FindContext(unsigned instance);
         unsigned XDAQ_NumNodes();
         std::string XDAQ_ContextName(unsigned instance);
         std::string XDAQ_NodeName(unsigned instance);
         std::string XDAQ_SshArgs(unsigned instance, int kind, const char* topcfgfile, const char* topworkdir, const char* connstr);

      public:
         enum EControlKinds { kindNone, kindSctrl, kindDim };

         ConfigBase(const char* fname = 0);
         ~ConfigBase();

         // following methods implemented for both XDAQ/native xml formats

         bool IsOk() const { return fVersion>=0; }
         bool IsXDAQ() const { return fVersion==0; }
         bool IsNative() const { return fVersion>0; }

         // returns number of nodes in xml file
         unsigned NumNodes();

         // returns number of nodes, which must be control by the control system
         unsigned NumControlNodes();

         // defines sequence number in list of control nodes
         unsigned ControlSequenceId(unsigned id);

         // returns nodename of specified context
         std::string NodeName(unsigned id);

         // returns name of specified context
         std::string ContextName(unsigned id);

         // method used by run.sh script to produce command line when test(0), run(1), conn(2), kill(3) application
         std::string SshArgs(unsigned id = 0, int ctrlkind = kindNone, const char* skind = "run", const char* topcfgfile = 0, const char* topworkdir = 0, const char* connstr = 0);

         std::string ResolveEnv(const char* arg);

         bool HasContext(unsigned id);

         static bool ProduceClusterFile(const char* fname, int numnodes);
   };

}

#endif
