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
#ifndef ROC_FACTORY_H
#define ROC_FACTORY_H

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

#ifndef DABC_ReferencesVector
#include "dabc/ReferencesVector.h"
#endif

#ifndef ROC_Board
#include "roc/Board.h"
#endif

namespace roc {

   class UdpDevice;

   class Factory: public dabc::Factory,
                  public base::BoardFactory {
      protected:

         dabc::ReferencesVector fDevs;

      public:

         Factory(const std::string& name);

         virtual dabc::Application* CreateApplication(const std::string& classname, dabc::Command cmd);

         virtual dabc::Module* CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd);

         virtual dabc::Device* CreateDevice(const std::string& classname, const std::string& devname, dabc::Command cmd);

         virtual void ShowDebug(int lvl, const char* msg);

         virtual bool IsFactoryFor(const char* url);

         virtual base::Board* DoConnect(const char* name, base::ClientRole role);

         virtual bool DoClose(base::Board* brd);

   };

}

#endif

