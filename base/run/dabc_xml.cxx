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

#include "dabc/ConfigBase.h"

#include <iostream>
#include <cstring>
#include <cstdlib>

int main(int numc, char* args[])
{
   const char* configuration = nullptr;
   const char* workdir = nullptr;
   unsigned configid = 0;

   if(numc > 1) configuration = args[1];

   dabc::ConfigBase cfg(configuration);
   if (!cfg.IsOk() || (cfg.GetVersion()!=2)) return 7;

   int cnt = 2;
   while (cnt<numc) {

      const char* arg = args[cnt++];

      if (strcmp(arg,"-id")==0) {
         if (cnt < numc)
            configid = (unsigned) atoi(args[cnt++]);
      } else
      if (strcmp(arg,"-workdir")==0) {
         if (cnt < numc)
            workdir = args[cnt++];
      } else
      if (strcmp(arg,"-number")==0) {
         unsigned res = cfg.NumNodes();
         if (res==0) return 5;
         std::cout << res << std::endl;
         std::cout.flush();
      } else
      if (strcmp(arg,"-mode") == 0) {
         const char* kind = (cnt < numc) ? args[cnt++] : "start";
         std::string res = cfg.SshArgs(configid, kind, configuration, workdir);
         if (res.length()==0) return 7;
         std::cout << res << std::endl;
         std::cout.flush();
      } else
      if (strstr(arg,"-nodename")==arg) {
         std::string name = cfg.NodeName(configid);
         if (name.length() == 0) return 5;
         std::cout << name << std::endl;
         std::cout.flush();
      } else
      if (strstr(arg,"-contextname")==arg) {
         std::string name = cfg.ContextName(configid);
         if (name.length() == 0) return 5;
         std::cout << name << std::endl;
         std::cout.flush();
      }
   }

   return 0;
}
