// $Id$

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

#include <cstdio>
#include <cstring>
#include <string>

#include "dabc/Manager.h"

#include "dabc/api.h"

//#define DOGMA_DEBUG 1

#ifdef DOGMA_DEBUG
#define printdeb( args... )  printf( args );
#else
#define printdeb( args...) ;
#endif



int usage(const char* errstr = nullptr)
{
   if (errstr)
      printf("Error: %s\n\n", errstr);

   printf("Remote command test utility for dogma\n");
   printf("\tUsage: dogmacmd <nodename>:12345 [nloops]\n");

   return errstr ? 1 : 0;
}

int main(int argc, char* argv[])
{
   int nloop=1;
   if ((argc < 2) || !strcmp(argv[1], "-help") || !strcmp(argv[1], "?"))
      return usage();
   if(argc == 3)
      {
         nloop=atoi(argv[2]);
         printdeb("Will execute command %d times.\n",nloop);
      }
   if (!dabc::CreateManager("shell", 0)) {
      printf("Fail to create manager\n");
      return 1;
   }

   auto stamp = dabc::TimeStamp::Now();

   printdeb("Did create manager\n");

   std::string nodename = dabc::MakeNodeName(argv[1]);

   printdeb("Build node name %s\n", nodename.c_str());

   if (!dabc::ConnectDabcNode(nodename)) {
      printf("Fail to connect to node %s\n", nodename.c_str());
      return 1;
   }

   auto tm1 = stamp.SpentTillNow(true);



   std::string module_name = nodename + "/dogma";

   dabc::Command cmd("GenericRead");
   cmd.SetReceiver(module_name);
   cmd.SetInt("Addr", 0x1000);
   cmd.SetTimeout(10);
   int res=0;
   for(int t=0; t<nloop; ++t)
   {
      res = dabc::mgr.Execute(cmd);
   }
   auto tm2 = stamp.SpentTillNow(true);
   printf("Connect to node %s takes %5.3f ms\n", nodename.c_str(), tm1*1e3);
   printf("Command execution for %d times: last res = %s Value = %d takes %5.3f ms (%5.3f ms each)\n", nloop, (res == dabc::cmd_true ? "Ok" : "Fail"), cmd.GetInt("Value"), tm2*1e3, tm2*1e3/nloop);

   return 0;
}
