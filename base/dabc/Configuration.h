#ifndef DABC_Configuration
#define DABC_Configuration

#ifndef DABC_XmlEngine
#include "dabc/XmlEngine.h"
#endif

namespace dabc {

/*

   class XdaqConfiguration {
      public:
         XdaqConfiguration(const char* fname, bool showerr = true);
         ~XdaqConfiguration();

         bool IsOk() const { return fDoc!=0; }

         XMLNodePointer_t FindContext(unsigned instance = 0, bool showerr = true);
         unsigned NumNodes();
         String NodeName(unsigned instance = 0);
         bool LoadLibs(unsigned instance = 0);
         bool ReadPars(unsigned instance = 0);

      protected:
         XmlEngine         fXml;
         XMLDocPointer_t   fDoc;
   };
*/

   // __________________________________________________________________________

   class Configuration {
      protected:
         XmlEngine         fXml;
         XMLDocPointer_t   fDoc;
         int               fVersion;  // -1 - error, 0 - xdaq, 1 and more - dabc
         XMLNodePointer_t  fSelected; // selected node, recognized as root node
         Configuration    *fPrnt;  // parent configuration with defaults for some variables

         // following variables work as 'enviroment'
         String            envDABCSYS;
         String            envDABCUSERDIR;
         String            envDABCNODEID;

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

         bool IsNodeName(XMLNodePointer_t node, const char* name);
         const char* GetAttr(XMLNodePointer_t node, const char* attr, const char* defvalue = 0);
         int  GetIntAttr(XMLNodePointer_t node, const char* attr, int defvalue = 0);
         XMLNodePointer_t FindChild(XMLNodePointer_t node, const char* name);
         XMLNodePointer_t FindContext(unsigned id);

         XMLNodePointer_t XDAQ_FindContext(unsigned instance);
         unsigned XDAQ_NumNodes();
         String XDAQ_NodeName(unsigned instance);
         String XDAQ_SshArgs(unsigned instance, int kind, const char* topcfgfile, const char* topworkdir, const char* connstr);

      public:
         Configuration(const char* fname);
         ~Configuration();


         // following methods implemented for both XDAQ/native xml formats

         bool IsOk() const { return fVersion>=0; }
         bool IsXDAQ() const { return fVersion==0; }
         bool IsNative() const { return fVersion>0; }

         // returns number of nodes in xml file
         unsigned NumNodes();

         // returns nodename of specified context, to be implemented later
         String NodeName(unsigned id);

         // method used by run.sh script to produce command line when test(0), run(1), conn(2), kill(3) application
         String SshArgs(unsigned id = 0, int kind = 0, const char* topcfgfile = 0, const char* topworkdir = 0, const char* connstr = 0);


         bool HasContext(unsigned id);

         bool LoadLibs(unsigned id) { return true; }

         bool ReadPars(unsigned id) { return true; }

         String ResolveEnv(const char* arg);


         static bool ProduceClusterFile(const char* fname, int numnodes);
   };


}

#endif
