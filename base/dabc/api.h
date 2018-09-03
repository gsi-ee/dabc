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

#ifndef DABC_api
#define DABC_api

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

namespace dabc {

   /** Function should be used to create manager instance
    * \par cmd_port defines if socket communication channel will be created
    *  cmd_port < 0  - nothing will be done (default)
    *  cmd_port == 0 - communication channel without server socket
    *  cmd_port > 0  - communication channel with specified server socket */
   extern bool CreateManager(const std::string &name, int cmd_port = -1);

   /** Method is used to install DABC-specific Ctrl-C handler
    * It allows to correctly stop program execution when pressing Ctrl-C */
   extern bool InstallSignalHandlers();

   /** Returns true when CtrlC was pressed in handler */
   extern bool CtrlCPressed();

   /** \brief Function can be used to destroy manager
    * \details Could be used to ensure that all dabc-relevant objects are destroyed */
   extern bool DestroyManager();

   /** \brief Function creates node name, which can be supplied as receiver of dabc commands */
   extern std::string MakeNodeName(const std::string &arg);

   /** \brief Function establish connection with specified dabc node
    *
    * \details Address should look like dabc://nodname:port */
   extern bool ConnectDabcNode(const std::string &nodeaddr);

   /** \brief Function request hierarchy of objects on remote node */
   extern Hierarchy GetNodeHierarchy(const std::string &nodeaddr);
}

#endif
