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

#include <cstdio>
#include <cstring>
#include <ctime>
#include <vector>

#include "mbs/api.h"

int usage(const char *errstr = nullptr)
{
   if (errstr) printf("Error: %s\n\n", errstr);

   printf("Utility for execute commands on remote MBS node\n");
   printf("   mbscmd nodename [args] -cmd cmd1 [-cmd cmd2] [-cmd cmd3] ...\n");
   printf("Arguments:\n");
   printf("   -logport number      - port number of log channel (-1 - off, default 6007)\n");
   printf("   -cmdport number      - port number of command channel (-1 - off, default 6019)\n");
   printf("   -prompter            - connect to MBS prompter (same as -cmdport 6006)\n");
   printf("   -stat                - enable reading of status record from port 6008 (default off)\n");
   printf("   -cmd mbs_command     - MBS command to execute (can be any number)\n");
   printf("   -tmout time          - timeout for command execution (default 5 sec)\n");
   printf("   -wait time           - wait time at the end of utility (default 1 sec)\n");

   return errstr ? 1 : 0;
}


int main(int argc, char* argv[])
{
   if (argc < 2) return usage();

   int logport = 6007, cmdport = 6019, statport = 0;
   double tmout = 5., waittm = 1.;

   std::vector<std::string> cmds;

   int n = 1;
   while (++n<argc) {
      if (strcmp(argv[n],"-prompter") == 0) { cmdport = 6006; } else
      if (strcmp(argv[n],"-stat") == 0) { statport = 6008; } else
      if ((strcmp(argv[n],"-logport") == 0) && (n+1<argc)) { dabc::str_to_int(argv[++n], &logport); } else
      if ((strcmp(argv[n],"-cmdport") == 0) && (n+1<argc)) { dabc::str_to_int(argv[++n], &cmdport); } else
      if ((strcmp(argv[n],"-tmout") == 0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &tmout); } else
      if ((strcmp(argv[n],"-wait") == 0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &waittm); } else
      if ((strcmp(argv[n],"-cmd") == 0) && (n+1<argc)) { cmds.emplace_back(argv[++n]); } else
      usage("Unknown option");
   }

   mbs::MonitorHandle ref = mbs::MonitorHandle::Connect(argv[1], cmdport, logport, statport);

   dabc::SetDebugLevel(0);

   //ref.MbsCmd("sta logrem");

   for (auto &cmd : cmds)
      ref.MbsCmd(cmd, tmout);

   dabc::Sleep(waittm);

   ref.Disconnect();

   return 0;
}
