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
#ifndef DABC_ROOT_functions
#define DABC_ROOT_functions

namespace dabc_root {

   /** This is dummy class, required that rootmap file is not empty and
    * ROOT knows which extra library should be load */
   class Test {
       Test() {}
       ~Test() {}
   };


   /** Method need to start HTTP server in ROOT session */
   bool StartHttpServer(int port = 8080);

   /** Method need to connect to master application,
    * which is normally HTTP server for many client processes */
   bool ConnectMaster(const char* master_url);

}

#endif
