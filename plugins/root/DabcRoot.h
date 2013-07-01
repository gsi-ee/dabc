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
#ifndef DabcRoot_h
#define DabcRoot_h

class DabcRoot {

   public:

      /** Method need to start HTTP server in ROOT session */
      static bool StartHttpServer(int port = 8080);

      /** Method need to connect to master application,
       * which is normally HTTP server for many client processes */
      static bool ConnectMaster(const char* master_url);

};

#endif
