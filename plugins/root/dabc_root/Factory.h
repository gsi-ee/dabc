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
#ifndef DABC_ROOT_Factory
#define DABC_ROOT_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace dabc_root {

   class Factory : public dabc::Factory {
      public:
         Factory(const std::string& name) : dabc::Factory(name) {}

         virtual void Initialize();

         virtual dabc::Reference CreateObject(const std::string& classname, const std::string& objname, dabc::Command cmd);

         virtual dabc::DataInput* CreateDataInput(const std::string&);

         virtual dabc::DataOutput* CreateDataOutput(const std::string&);
   };


   /** This is dummy class, required that rootmap file is not empty and
    * ROOT knows which extra library should be load */
   class Test {
       Test() {}
       ~Test() {}
   };


   /** Method need to start HTTP server in ROOT session */
   void StartHttpServer(int port = 8080);

   /** Method need to connect to master application,
    * which is normally HTTP server for many client processes */
   void ConnectMaster(const char* master_url);
}

#endif
