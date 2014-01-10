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

   /** \brief %Server provides http access to DABC
    *
    */

   class FastCgi : public Server  {
      protected:
         int   fCgiPort;                 //!< configured fast cgi port, default 9000
         bool  fDebugMode;               //!< when true, only debug info returned by server
         int   fSocket;                  //!< fastcgi server socket
         dabc::PosixThread* fThrd;       //!< thread where fastcgi mainloop will run

         virtual void OnThreadAssigned();
      public:
         FastCgi(const std::string& name, dabc::Command cmd = 0);
         virtual ~FastCgi();

         virtual const char* ClassName() const { return "FastCgiServer"; }

         static void* RunFunc(void* arg);
   };
}

#endif
