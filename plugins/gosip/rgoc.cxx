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

#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>
#include <stdlib.h>

#include "dabc/Manager.h"

#include "dabc/api.h"

// take command definitions from here:
#include "gosip/Command.h"

/** need to define this here again, since we run independent of libDabcGosip:*/
char gosip::Command::CommandDescription[GOSIP_MAXTEXT];


void goscmd_usage (const char *progname)
{
  printf ("***************************************************************************\n");

  printf (" %s (remote gosipcmd) for dabc and mbspex library  \n", progname);
  printf (" v0.7 19-Jan-2023 by JAM (j.adamczewski@gsi.de)\n");
  printf ("***************************************************************************\n");
  printf (
      "  usage: %s [-h|-z] [[-i|-r|-w|-s|-u] [-b] | [-c|-v FILE] [-n DEVICE |-d|-x] node[:port] sfp slave [address [value [words]|[words]]]] \n",
      progname);
  printf ("\t Options:\n");
  printf ("\t\t -h        : display this help\n");
  printf ("\t\t -z        : reset (zero) pexor/kinpex board \n");
  printf ("\t\t -i        : initialize sfp chain \n");
  printf ("\t\t -r        : read from register \n");
  printf ("\t\t -w        : write to  register\n");
  printf ("\t\t -s        : set bits of given mask in  register\n");
  printf ("\t\t -u        : unset bits of given mask in  register\n");
  printf ("\t\t -b        : broadcast io operations to all slaves in range (0-sfp)(0-slave)\n");
  printf ("\t\t -c FILE   : configure registers with values from FILE.gos\n");
  printf ("\t\t -v FILE   : verify register contents (compare with FILE.gos)\n");
  printf ("\t\t -n DEVICE : specify device number N (/dev/pexorN, default:0) \n");
  printf ("\t\t -d        : debug mode (verbose output) \n");
  printf ("\t\t -x        : numbers in hex format (defaults: decimal, or defined by prefix 0x) \n");
  printf ("\t Arguments:\n");
  printf ("\t\t node:port - nodename of remote gosip command server (default port 12345)\n");
  printf ("\t\t sfp       - sfp chain- -1 to broadcast all registered chains \n");
  printf ("\t\t slave     - slave id at chain, or total number of slaves. -1 for internal broadcast\n");
  printf ("\t\t address   - register on slave \n");
  printf ("\t\t value     - value to write on slave \n");
  printf ("\t\t words     - number of words to read/write/set incrementally\n");
  printf ("\t Examples:\n");
  printf ("\t  %s -z -n 1 x86l-59                   : master gosip reset of board /dev/pexor1 at node x86l-59\n",
      progname);
  printf ("\t  %s -i x86l-59 0 24                   : initialize chain at sfp 0 with 24 slave devices\n", progname);
  printf ("\t  %s -r -x x86l-59 1 0 0x1000          : read from sfp 1, slave 0, address 0x1000 and printout value\n",
      progname);
  printf ("\t  %s -r -x x86l-59 0 3 0x1000 5        : read from sfp 0, slave 3, address 0x1000 next 5 words\n",
      progname);
  printf (
      "\t  %s -r -b  x86l-113 1 3 0x1000 10      : broadcast read from sfp (0..1), slave (0..3), address 0x1000 next 10 words\n",
      progname);
  printf (
      "\t  %s -r --  x86l-42 -1 -1 0x1000 10     : broadcast read from address 0x1000, next 10 words from all registered slaves\n",
      progname);
  printf ("\t  %s -w -x  x86l-113 0 3 0x1000 0x2A     : write value 0x2A to sfp 0, slave 3, address 0x1000\n",
      progname);
  printf (
      "\t  %s -w -x  x86l-113 1 0 20000 AB FF     : write value 0xAB to sfp 1, slave 0, to addresses 0x20000-0x200FF\n",
      progname);
  printf (
      "\t  %s -w -b  localhost 1  3 0x20004c 1    : broadcast write value 1 to address 0x20004c on sfp (0..1) slaves (0..3)\n",
      progname);
  printf (
      "\t  %s -w --  x86l-113 -1 -1 0x20004c 1    : write value 1 to address 0x20004c on all registered slaves (internal driver broadcast)\n",
      progname);
  printf ("\t  %s -s     x86l-113 0 0 0x200000 0x4      : set bit 100 on sfp0, slave 0, address 0x200000\n", progname);
  printf ("\t  %s -u     x86l-113 0 0 0x200000 0x4 0xFF : unset bit 100 on sfp0, slave 0, address 0x200000-0x2000FF\n",
      progname);
  printf ("\t  %s x86l-113  -x -c run42.gos           : write configuration values from file run42.gos to slaves \n",
      progname);
  printf ("*****************************************************************************\n");
  exit (0);
}

int main (int argc, char *argv[])
{

  // here put gosipcmd functionalities:
  int l_status = 0;
  int opt;
  char cmd[GOSIP_CMD_MAX_ARGS][GOSIP_CMD_SIZE];
  unsigned int cmdLen = 0;
  unsigned int i;
  gosip::Command theCommand;

   /* get arguments*/
   optind = 1;
   while ((opt = getopt (argc, argv, "hzwrsuin:c:v:dxb")) != -1)
   {
     switch (opt)
     {
       case '?':
         goscmd_usage (basename (argv[0]));
         exit (EXIT_FAILURE);
       case 'h':
         goscmd_usage (basename (argv[0]));
         exit (EXIT_SUCCESS);
       case 'n':
         theCommand.devnum = strtol (optarg, NULL, 0);
         break;
       case 'w':
         theCommand.set_command(gosip::GOSIP_WRITE);
         break;
       case 'r':
         theCommand.set_command(gosip::GOSIP_READ);  // JAM new implementation with C++ command class here:



         break;
       case 's':
         theCommand.set_command(gosip::GOSIP_SETBIT);
         break;
       case 'u':
         theCommand.set_command(gosip::GOSIP_CLEARBIT);
         break;
       case 'z':
         theCommand.set_command(gosip::GOSIP_RESET);
         break;
       case 'i':
         theCommand.set_command(gosip::GOSIP_INIT);
         break;
       case 'c':
         theCommand.set_command(gosip::GOSIP_CONFIGURE);
         strncpy (theCommand.filename, optarg, GOSIP_MAXTEXT);
         break;
       case 'v':
         theCommand.set_command(gosip::GOSIP_VERIFY);
         strncpy (theCommand.filename, optarg, GOSIP_MAXTEXT);
         break;
       case 'd':
         theCommand.verboselevel = 1; /*strtol(optarg, NULL, 0); later maybe different verbose level*/
         break;
       case 'x':
         theCommand.hexformat = 1;
         break;
       case 'b':
         theCommand.broadcast = 1;
         break;
       default:
         break;
     }
   }

   /* get parameters:*/
   cmdLen = argc - optind;
   /*printf("- argc:%d optind:%d cmdlen:%d \n",argc, optind, cmdLen);*/
   if(!theCommand.assert_arguments(cmdLen)) return -1;
   for (i = 0; (i < cmdLen) && (i < GOSIP_CMD_MAX_ARGS); i++)
   {
     if (argv[optind + i])
       strncpy (cmd[i], argv[optind + i], GOSIP_CMD_SIZE);
     else
       DOUT1 ("warning: argument at position %d is empty!", optind + i);
   }

   std::string nodename = cmd[0];
   std::size_t found = nodename.find (":");
   if (found == std::string::npos)
   {
     nodename = nodename + ":" + std::to_string ((int) RGOC_DEFAULTPORT);    // use default port if not given in nodename
   }
   nodename = dabc::MakeNodeName (nodename);

   if ((theCommand.command == gosip::GOSIP_CONFIGURE) || (theCommand.command == gosip::GOSIP_VERIFY))
   {

     // get list of addresses and values from file later
   }
   else
   {
     theCommand.sfp = strtol (cmd[1], NULL, theCommand.hexformat == 1 ? 16 : 0);
     theCommand.slave = strtol (cmd[2], NULL, theCommand.hexformat == 1 ? 16 : 0); /* note: we might have negative values for broadcast here*/

     // if ((theCommand.command == GOSIP_READ) || (theCommand.command == GOSIP_WRITE))
     theCommand.address = strtoul (cmd[3], NULL, theCommand.hexformat == 1 ? 16 : 0);
     if (cmdLen > 4)
     {
       if (theCommand.command == gosip::GOSIP_READ)
         theCommand.repeat = strtoul (cmd[4], NULL, theCommand.hexformat == 1 ? 16 : 0);
       else
         theCommand.value = strtoul (cmd[4], NULL, theCommand.hexformat == 1 ? 16 : 0);
     }
     if (cmdLen > 5)
     {
       theCommand.repeat = strtoul (cmd[5], NULL, theCommand.hexformat == 1 ? 16 : 0);
     }

   }
   if(!theCommand.assert_command()) return -2;
   if (theCommand.verboselevel > 0)
   {
     DOUT1 ("parsed the following commmand:\n\t");
     theCommand.dump_command ();
   }
 // here connect to command server
   if (!dabc::CreateManager ("rgoc", 0))
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
   std::string module_name = nodename + "/gosip";

   dabc::Command dcmd ("GosipCommand");
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
       theCommand.value = dcmd.GetInt (name.c_str (), -1);
       std::string address = "ADDRESS_" + std::to_string (r);
       theCommand.address = dcmd.GetInt (address.c_str (), -1);
       std::string sfp = "SFP_" + std::to_string (r);
       theCommand.sfp = dcmd.GetInt (sfp.c_str (), -1);
       std::string slave = "SLAVE_" + std::to_string (r);
       theCommand.slave = dcmd.GetInt (slave.c_str (), -1);
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
