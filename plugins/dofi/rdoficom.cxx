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
#include <unistd.h>
#include <stdlib.h>

#include "dabc/Manager.h"

#include "dabc/api.h"

// take command definitions from here:
#include "dofi/Command.h"

/** need to define this here again, since we run independent of libDabcDofi:*/
char dofi::Command::CommandDescription[DOFI_MAXTEXT];


void goscmd_usage (const char *progname)
{
  printf ("***************************************************************************\n");

  printf (" %s (remote DOFI (Digital signals Over FIbre) commmand )\n", progname);
  printf (" v0.3 13-Feb-2023 by JAM (j.adamczewski@gsi.de)\n");
  printf ("***************************************************************************\n");
  printf (
      "  usage: %s [-h|-z] [[-r|-w|-s|-u]  [-d|-x] node[:port] [address [value [words]|[words]]]] \n",
      progname);
  printf ("\t Options:\n");
  printf ("\t\t -h        : display this help\n");
  printf ("\t\t -z        : reset (zero) muppet spi connection \n");
  printf ("\t\t -n        : reset (null) scalers \n");
  printf ("\t\t -e        : enable scalers \n");
  printf ("\t\t -f        : disable (fix) scalers \n");
  printf ("\t\t -r        : read from register \n");
  printf ("\t\t -w        : write to  register\n");
  printf ("\t\t -s        : set bits of given mask in  register\n");
  printf ("\t\t -u        : unset bits of given mask in  register\n");
  printf ("\t\t -c FILE   : configure registers with values from FILE.dof\n");
  printf ("\t\t -d        : debug mode (verbose output) \n");
  printf ("\t\t -x        : numbers in hex format (defaults: decimal, or defined by prefix 0x) \n");
  printf ("\t Arguments:\n");
  printf ("\t\t node:port - nodename of remote dofi command server (default port %d)\n", RDOC_DEFAULTPORT);
  printf ("\t\t address   - spi register on dofi board \n");
  printf ("\t\t value     - value to write tp register \n");
  printf ("\t\t words     - number of words to read/write/set incrementally\n");
  printf ("\t Examples:\n");
  printf ("\t  %s -z RA-11                   : master dofi reset of muppet board at node RA-11\n",
      progname);
  printf ("\t  %s -r -x 0x1000          : read from address 0x1000 and printout value\n",
      progname);
  printf ("\t  %s -r -x RA-9 0x1000 5        : read from address 0x1000 next 5 words\n",
      progname);
  printf ("\t  %s -w -x  RA-2 0x1000 0x2A     : write value 0x2A to address 0x1000\n",
      progname);
  printf (
      "\t  %s -w -x RA-9 20000 AB FF     : write value 0xAB to addresses 0x20000-0x200FF\n",
      progname);
  printf ("\t  %s -s     RA-9 0x200000 0x4      : set bit 100 on address 0x200000\n", progname);
  printf ("\t  %s -u     RA-9 0x200000 0x4 0xFF : unset bit 100 address 0x200000-0x2000FF\n",
      progname);
  printf ("*****************************************************************************\n");
  exit (0);
}

int main (int argc, char *argv[])
{

  // here put doficmd functionalities:
  int l_status = 0;
  int opt;
  char cmd[DOFI_CMD_MAX_ARGS][DOFI_CMD_SIZE];
  unsigned int cmdLen = 0;
  unsigned int i;
  dofi::Command theCommand;

   /* get arguments*/
   optind = 1;
   while ((opt = getopt (argc, argv, "hzwrsunc:dx")) != -1)
    {
     switch (opt)
     {
       case '?':
         goscmd_usage (basename (argv[0]));
         exit (EXIT_FAILURE);
       case 'h':
         goscmd_usage (basename (argv[0]));
         exit (EXIT_SUCCESS);
       case 'w':
         theCommand.set_command(dofi::DOFI_WRITE);
         break;
       case 'r':
         theCommand.set_command(dofi::DOFI_READ);  
         break;
       case 's':
         theCommand.set_command(dofi::DOFI_SETBIT);
         break;
       case 'u':
         theCommand.set_command(dofi::DOFI_CLEARBIT);
         break;
         
        case 'c':
         theCommand.set_command(dofi::DOFI_CONFIGURE);
         strncpy (theCommand.filename, optarg, DOFI_MAXTEXT);
         break;
       case 'z':
         theCommand.set_command(dofi::DOFI_RESET);
         break;
       case 'n':
         theCommand.set_command(dofi::DOFI_RESET_SCALER);
         break;
       case 'e':
         theCommand.set_command(dofi::DOFI_ENABLE_SCALER);
         break;
       case 'f':
         theCommand.set_command(dofi::DOFI_DISABLE_SCALER);
         break;
       case 'd':
         theCommand.verboselevel = 1; /*strtol(optarg, NULL, 0); later maybe different verbose level*/
         break;
       case 'x':
         theCommand.hexformat = 1;
         break;
       default:
         break;
     }
   }

   /* get parameters:*/
   cmdLen = argc - optind;
   //printf("- argc:%d optind:%d cmdlen:%d \n",argc, optind, cmdLen);
   if(!theCommand.assert_arguments(cmdLen)) return -1;
   for (i = 0; (i < cmdLen) && (i < DOFI_CMD_MAX_ARGS); i++)
   {
     if (argv[optind + i])
       strncpy (cmd[i], argv[optind + i], DOFI_CMD_SIZE);
     else
       DOUT1 ("warning: argument at position %d is empty!", optind + i);
   }

   std::string nodename = cmd[0];
   std::size_t found = nodename.find (":");
   if (found == std::string::npos)
   {
     nodename = nodename + ":" + std::to_string ((int) RDOC_DEFAULTPORT);    // use default port if not given in nodename
   }
   nodename = dabc::MakeNodeName (nodename);

   
     // if ((theCommand.command == DOFI_READ) || (theCommand.command == DOFI_WRITE))
     theCommand.address = strtoul (cmd[1], NULL, theCommand.hexformat == 1 ? 16 : 0);
     if (cmdLen > 2)
     {
       if (theCommand.command == dofi::DOFI_READ)
         theCommand.repeat = strtoul (cmd[2], NULL, theCommand.hexformat == 1 ? 16 : 0);
       else
         theCommand.value = strtoul (cmd[2], NULL, theCommand.hexformat == 1 ? 16 : 0);
     }
     if (cmdLen > 3)
     {
       theCommand.repeat = strtoul (cmd[3], NULL, theCommand.hexformat == 1 ? 16 : 0);
     }

   if(!theCommand.assert_command()) return -2;
   if (theCommand.verboselevel > 0)
   {
     DOUT1 ("parsed the following commmand:\n\t");
     theCommand.dump_command ();
   }
 // here connect to command server
   if (!dabc::CreateManager ("rdoc", 0))
   {
     printf ("Fail to create manager\n");
     return 1;
   }
   dabc::lgr ()->SetDebugMask (dabc::Logger::lMessage);    // suppress output of name and time
   auto stamp = dabc::TimeStamp::Now ();

   DOUT2("Did create manager\n");
   DOUT2("Using node name %s\n", nodename.c_str ());

   if (!dabc::ConnectDabcNode (nodename))
   {
     printf ("Fail to connect to node %s\n", nodename.c_str ());
     return 1;
   }
   auto tm1 = stamp.SpentTillNow (true);
   std::string module_name = nodename + "/dofi";

   dabc::Command dcmd ("DofiCommand");
   theCommand.BuildDabCommand(dcmd);
   dcmd.SetReceiver (module_name);
   dcmd.SetTimeout (10);

   int res = dabc::mgr.Execute (dcmd);
   auto tm2 = stamp.SpentTillNow (true);
   if (theCommand.verboselevel > 0)
   {
     DOUT0 ("Connect to node %s takes %5.3f ms\n", nodename.c_str (), tm1 * 1e3);
     DOUT0 ("Command execution: res = %s Value = %d takes %5.3f ms\n", (res == dabc::cmd_true ? "Ok" : "Fail"),
         dcmd.GetInt ("VALUE"), tm2 * 1e3);
   }
   //evaluate result and print
   if (res == dabc::cmd_true)
   {
     // here treat result of repeated read if any:
     int numresults=dcmd.GetInt ("NUMRESULTS", 0);// dynamic number of return addresses in case of broadcast read!
     for (int r = 0; r < numresults; ++r)
     {
       std::string name = "VALUE_" + std::to_string (r);
       theCommand.value = dcmd.GetUInt (name.c_str (), -1);
       std::string address = "ADDRESS_" + std::to_string (r);
       theCommand.address = dcmd.GetUInt (address.c_str (), -1);
       if (theCommand.value != -1)
         theCommand.output();
     }
   }
   else
   {
     EOUT ("!!!!!!! Remote command execution failed with returncode %d\n",  dcmd.GetInt ("VALUE"));
     theCommand.dump_command();
   }

   // here optional info or error message:
   std::string mess= dcmd.GetStr ("MESSAGE", "OK");
   if(mess.compare("OK")!=0)
     {
       DOUT0 (mess.c_str());
     }

  return l_status;
}
