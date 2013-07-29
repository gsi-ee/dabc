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

#ifndef DABC_Configuration
#define DABC_Configuration

#ifndef DABC_ConfigBase
#include "dabc/ConfigBase.h"
#endif

#ifndef DABC_ConfigIO
#include "dabc/ConfigIO.h"
#endif

#include <list>


namespace dabc {

   class Object;
   class ConfigIO;

   /** \brief Full-functional class to reading configuration from xml files
    *
    * \ingroup dabc_all_classes
    *
    */

   class Configuration : public ConfigBase {
      friend class ConfigIO;

      protected:
         XMLNodePointer_t  fSelected; // selected context node

         std::string      fMgrHost;
         int              fMgrPort;
         std::string      fMgrName;
         int              fMgrNodeId;
         int              fMgrNumNodes;

         static std::string      fLocalHost;

      public:
         Configuration(const char* fname = 0);
         virtual ~Configuration();

         bool SelectContext(unsigned nodeid, unsigned numnodes);

         std::string MgrHost() const { return fMgrHost; }
         int MgrPort() const { return fMgrPort; }
         const char* MgrName() const { return fMgrName.c_str(); }
         int MgrNodeId() const { return fMgrNodeId; }
         int MgrNumNodes() const { return fMgrNumNodes; }

         std::string InitFuncName();
         std::string RunFuncName();
         int ShowCpuInfo();

         /** \brief Defines if control used in dabc node
          * \returns -1 - when control="false" specified, no control created
          *           0 - control parameter undefined, control will be used when NumNodes() > 1
          *          +1 - when control="true" specified */
         int UseControl();

         /** \brief Defines master DABC process, to which application will be connected
          * Master means all control and monitoring functions will be delegated to master.
          * Typically master provide http-server, which allows to control also all clients */
         std::string MasterName();

         int GetRunTime();
         bool NormalMainThread();
         std::string ThreadsLayout();

         /** Return default affinity of any newly created thread */
         std::string Affinity();

         /** Method is used to find xml nodes like Module, MemoryPool, Connection in
          * the xml file which should be used for autocreation */
         bool NextCreationNode(XMLNodePointer_t& prev, const char* nodename, bool check_name_for_multicast);

         std::string GetUserPar(const char* name, const char* dflt = 0);
         int GetUserParInt(const char* name, int dflt = 0);

         std::string ConetextAppClass();

         bool LoadLibs();

         static std::string GetLocalHost() { return fLocalHost; }

   };


}

#endif
