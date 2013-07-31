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

      /** \brief Method starts HTTP server in ROOT session
       *
       * After method call one could in any web browser open page with
       * address http://your_host_name:port and browse objects registered in gROOT directory.
       * It is all histograms, canvases, files. If necessary, one could add own objects to gROOT.
       *
       * sync_timer parameter defines that kind of timer is used. When true (default),
       * synchronous timer is created which can process http-server commands only when
       * gSystem->ProcessEvents() are called. It is a case in normal ROOT session and it is
       * recommended way of working. Advantage - one exactly knows and can control
       * when http server can interrupt program execution.
       *
       * If no any gSystem->ProcessEvents() calls provided in user code and
       * it is not possible to add such code, one should use asynchronous timer.
       * Such timer may lead to problems on objects streaming,
       * while asynchronous timer can interrupt program at any point of execution  */
      static bool StartHttpServer(int port = 8080, bool sync_timer = true);

      /** Method need to connect to master DABC application,
       * which is normally HTTP server for many client processes
       * Meaning of sync_timer variable see in StartHttpServer() documentation */
      static bool ConnectMaster(const char* master_url, bool sync_timer = true);

};

#endif
