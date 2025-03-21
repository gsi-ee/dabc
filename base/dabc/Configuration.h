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
         XMLNodePointer_t    fSelected{nullptr}; // selected context node

         std::string         fMgrHost;
         int                 fMgrPort;
         std::string         fMgrName;
         int                 fMgrNodeId;
         int                 fMgrNumNodes;

         static std::string  fLocalHost;

      public:
         Configuration(const char *fname = nullptr);
         virtual ~Configuration();

         bool SelectContext(unsigned nodeid, unsigned numnodes);

         std::string MgrHost() const { return fMgrHost; }
         int MgrPort() const { return fMgrPort; }
         const char *MgrName() const { return fMgrName.c_str(); }
         int MgrNodeId() const { return fMgrNodeId; }
         int MgrNumNodes() const { return fMgrNumNodes; }

         std::string InitFuncName();
         std::string RunFuncName();
         int ShowCpuInfo();
         bool UseSlowTime();

         /** \brief Defines if control used in dabc node
          * \returns -1 - when control="false" specified, no control created
          *           0 - control parameter undefined, control will be used when NumNodes() > 1
          *          +1 - when control="true" specified */
         int UseControl();

         /** \brief Defines master DABC process, to which application will be connected
          * Master means all control and monitoring functions will be delegated to master.
          * Typically master provide http-server, which allows to control also all clients */
         std::string MasterName();

         /** \brief Returns configured time to run DABC process */
         double GetRunTime();

         /** \brief Returns time, required to halt DABC process */
         double GetHaltTime();

         /** \brief Returns time, required for normal thread stopping */
         double GetThrdStopTime();

         bool NormalMainThread();
         std::string ThreadsLayout();

         /** Return default affinity of any newly created thread */
         std::string Affinity();

         /** \brief Returns true when publisher should be created
          * \returns -1 - when publisher="false" specified
          *           0 - when publisher not specified
          *          +1 - when publisher="true" specified */
         int  WithPublisher();

         /** Method is used to find xml nodes like Module, MemoryPool, Connection in
          * the xml file which should be used for auto-creation */
         bool NextCreationNode(XMLNodePointer_t& prev, const char *nodename, bool check_name_for_multicast);

         std::string GetUserPar(const char *name, const char *dflt = nullptr);
         int GetUserParInt(const char *name, int dflt = 0);

         std::string ConetextAppClass();

         bool LoadLibs();

         static std::string GetLocalHost() { return fLocalHost; }

         static std::string GetLibsDir();

         static std::string GetPluginsDir();

   };

}

#endif
