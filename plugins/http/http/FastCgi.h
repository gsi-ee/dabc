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

#ifndef HTTP_FastCgi
#define HTTP_FastCgi

#ifndef HTTP_Server
#include "http/Server.h"
#endif

#ifndef DABC_threads
#include "dabc/threads.h"
#endif

namespace http {

   /** \brief %Server provides fastcgi access to DABC
    *
    */

   class FastCgi : public Server  {
      protected:
         int   fCgiPort{0};                     //!< configured fast cgi port, default 9000
         bool  fDebugMode{false};               //!< when true, only debug info returned by server
         int   fSocket{0};                      //!< fastcgi server socket
         dabc::PosixThread* fThrd{nullptr};     //!< thread where fastcgi mainloop will run

         void OnThreadAssigned() override;
      public:
         FastCgi(const std::string &name, dabc::Command cmd = nullptr);
         virtual ~FastCgi();

         const char* ClassName() const override { return "FastCgiServer"; }

         static void* RunFunc(void* arg);
   };
}

#endif
