// $Id: mbsprint.cxx 2226 2014-04-04 07:32:45Z linev $

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

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "dabc/string.h"

#include "mbs/api.h"

int usage(const char* errstr = 0)
{
   if (errstr!=0) {
      printf("Error: %s\n\n", errstr);
   }

   printf("utility for accessing remote MBS nodes\n");
   printf("mbscmd nodename [args]\n\n");
   printf("Additional arguments:\n");
   printf("   -logport number         - port number of log channel (-1 - off, default 6007)\n");
   printf("   -cmdport number         - port number of command channel (-1 - off, default 6019)\n");

   return errstr ? 1 : 0;
}



int main(int argc, char* argv[])
{
   if (argc<2) return usage();

   int logport(6007), cmdport(6019);

   int n = 1;
   while (++n<argc) {
      if ((strcmp(argv[n],"-logport")==0) && (n+1<argc)) { dabc::str_to_int(argv[++n], &logport); } else
      if ((strcmp(argv[n],"-cmdport")==0) && (n+1<argc)) { dabc::str_to_int(argv[++n], &cmdport); } else
      return usage("Unknown option");
   }

   mbs::MonitorHandle ref = mbs::MonitorHandle::Connect(argv[1], cmdport, logport);

   dabc::SetDebugLevel(0);

   //ref.MbsCmd("sta logrem");

   ref.MbsCmd("type event");

   ref.MbsCmd("show rate");

   dabc::Sleep(0.5);

   ref.Disconnect();

   return 0;
}
