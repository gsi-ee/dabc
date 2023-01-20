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

extern "C"
{
  #include "mbspex/libmbspex.h"
}

#include "gosip/TerminalModule.h"
#include "dabc/Manager.h"

#include <unistd.h>
#include <vector>

/** toggle configuration mode: send config from file as data block to driver (1), or write with single bus commands (0)*/
#define GOSIPCMD_BLOCKCONFIG 1

/* we need to implement printm here to also catch output of libmbspex*/
void printm (char *fmt, ...)
{
  char c_str[256];
  va_list args;
  va_start (args, fmt);
  vsprintf (c_str, fmt, args);
  DOUT0("%s", c_str);
  va_end (args);
}


/* helper macro to check mbspex device in each command function*/
#define GOSIP_ASSERT_DEVHANDLE if(!com.assert_command ()) return -1; \
  open_device(com.devnum);\
  if(fFD_pex==0) return -2; \
  if (com.verboselevel) com.dump_command ();

gosip::TerminalModule::TerminalModule (const std::string &name, dabc::Command cmd) :
    dabc::ModuleAsync (name, cmd), fFD_pex(0),fDevnum(0),fConfigfile(0), fLinecount(0),fErrcount(0)
{
}

gosip::TerminalModule::~TerminalModule()
{
  close_device();
}

int gosip::TerminalModule::ExecuteCommand (dabc::Command cmd)
{
  int l_status = 0;
  if (cmd.IsName ("GosipCommand"))
  {
    gosip::Command theCommand;
    theCommand.ExtractDabcCommand (cmd);
    if (theCommand.verboselevel > 1)
    {
      DOUT0("Received the following commmand:");
      theCommand.dump_command ();
    }

    if(!theCommand.assert_command ()) return -1;
    fCommandAddress.clear (); // for broadcast case: do not clear resultsin inner functions!
    fCommandResults.clear ();
    fCommandSfp.clear ();
    fCommandSlave.clear ();
    fCommandMessage="OK";

    l_status = execute_command (theCommand);

    // put here all relevant return values:
    // for repeat read, we set all single read information for client:
    for (unsigned int r = 0; r < fCommandResults.size (); ++r)
    {
      std::string name = "VALUE_" + std::to_string (r);
      cmd.SetInt (name.c_str (), fCommandResults[r]);
      std::string address = "ADDRESS_" + std::to_string (r);
      cmd.SetInt (address.c_str (), fCommandAddress[r]);
      // for broadcasting, also remember sfp and slave ids!
      std::string sfp = "SFP_" + std::to_string (r);
      cmd.SetInt (sfp.c_str (), fCommandSfp[r]);
      std::string slave = "SLAVE_" + std::to_string (r);
      cmd.SetInt (slave.c_str (), fCommandSlave[r]);
    }
    cmd.SetInt ("NUMRESULTS", fCommandResults.size());
    cmd.SetInt ("VALUE", l_status); // pass back return code
    cmd.SetStr ("MESSAGE",fCommandMessage.c_str());
    return l_status ? dabc::cmd_false : dabc::cmd_true;
  }
  return dabc::cmd_false;
}

void gosip::TerminalModule::BeforeModuleStart ()
{
  DOUT0("Starting GOSIP command server module");

  open_device (fDevnum);
}

void gosip::TerminalModule::ProcessTimerEvent (unsigned)
{
}



int gosip::TerminalModule::open_device (int devnum)
{
  if(devnum!=fDevnum)
      {
        DOUT1("Open new mbspex handle for device number %d (previous:%d)",devnum, fDevnum);
        close_device();
        fDevnum=devnum;
      }
  fFD_pex = mbspex_open (fDevnum);
  if (fFD_pex < 0)
  {
    EOUT ("ERROR>> open /dev/pexor%d \n", fDevnum);
    //exit (1); // throw something later?
    fFD_pex=0;
    return -1;
  }
  return 0;
}

void gosip::TerminalModule::close_device ()
{
  if(fFD_pex) close (fFD_pex);
}

int gosip::TerminalModule::open_configuration (gosip::Command &com)
{
// JAM 12-12-22: note that configuration file is on command server file system
// TODO: may later provide mechanism for local config file on rgoc client host which may be send
// embedded into dabc Command and evaluated in execute command (via temporary file?)
    fConfigfile = fopen (com.filename, "r");
    if (fConfigfile == NULL)
    {
      EOUT (" Error opening Configuration File '%s': %s\n", com.filename, strerror (errno));
      return -1;
    }
  fLinecount = 0;
  return 0;
}

int gosip::TerminalModule::close_configuration ()
{
  fclose (fConfigfile);
  return 0;
}

int gosip::TerminalModule::next_config_values (gosip::Command &com)
{
  /* file parsing code was partially stolen from trbcmd.c Thanks Ludwig Maier et al. for this!*/
  int status = 0;
  size_t linelen = 0;
  char *cmdline;
  char cmd[GOSIP_CMD_MAX_ARGS][GOSIP_CMD_SIZE];
  int i, cmdlen;

  char *c = NULL;
  for (i = 0; i < GOSIP_CMD_MAX_ARGS; i++)
  {
    cmd[i][0] = '\0';
  }
  fLinecount++;
  status = getline (&cmdline, &linelen, fConfigfile);
  DOUT5("DDD got from file %s line %s !\n",com.filename, cmdline);
  if (status == -1)
  {
    if (feof (fConfigfile) != 0)
    {
      /* EOF reached */
      rewind (fConfigfile);
      return -1;
    }
    else
    {
      /* Error reading line */
      EOUT ("Error reading script-file\n");
      return -1;
    }
  }

  /* Remove newline and comments */
  if ((c = strchr (cmdline, '\n')) != NULL)
  {
    *c = '\0';
  }
  if ((c = strchr (cmdline, '#')) != NULL)
  {
    *c = '\0';
  }

  /* Split up cmdLine */
  sscanf (cmdline, "%s %s %s %s %s %s %s %s %s %s", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7],
      cmd[8], cmd[9]);

  for (i = 0, cmdlen = 0; i < GOSIP_CMD_MAX_ARGS; i++, cmdlen++)
  {
    if (cmd[i][0] == '\0')
    {
      DOUT5("got commandlen(%d)=%d",i,cmdlen);
      break;
    }
  }
  if (cmdlen == 0)
  {
    /* Empty Line */
    DOUT5("got empty line");
    return 0;
  }

  if (com.verboselevel > 1)
  {
    DOUT0 ("#Line %d:  %s\n", fLinecount, cmdline);
  }

  if (!(cmdlen >= 4))
  {
    EOUT ("Invalid configuration data at line %d, (cmdlen=%d)stopping configuration\n", fLinecount, cmdlen);
    return -1;
  }
  com.sfp = strtol (cmd[0], NULL, com.hexformat == 1 ? 16 : 0);
  com.slave = strtol (cmd[1], NULL, com.hexformat == 1 ? 16 : 0);
  com.address = strtoul (cmd[2], NULL, com.hexformat == 1 ? 16 : 0);
  com.value = strtoul (cmd[3], NULL, com.hexformat == 1 ? 16 : 0);
  if (cmdlen > 4)
  {
    // parse optional command identifier
    //if (strcasestr (cmd[4], "setbit") != 0)
    //JAM2017 -  this was not posix standard, gcc 6.3 doesnt like it. we do workaround:
    if ((strstr (cmd[4], "setbit") != 0) || (strstr (cmd[4], "SETBIT") != 0))
    {
      com.command = GOSIP_SETBIT;
    }
    //else if (strcasestr (cmd[4], "clearbit") != 0)
    if ((strstr (cmd[4], "clearbit") != 0) || (strstr (cmd[4], "CLEARBIT") != 0))
    {
      com.command = GOSIP_CLEARBIT;
    }
    else
    {
      // do not change mastercommand id (configure or verify)
    }

  }
  return status;
}

int gosip::TerminalModule::write (gosip::Command &com)
{
  int rev = 0;
  GOSIP_ASSERT_DEVHANDLE;
  rev = mbspex_slave_wr (fFD_pex, com.sfp, com.slave, com.address, com.value);
  if (rev != 0)
  {
    //printm ("ERROR on writing!\n"); /* we already have error output from mbspex*/
    //if (com->verboselevel)
    com.dump_command ();
    std::stringstream hexstream;
    hexstream << std::hex << com.address; // no better way to do hex formatting with std::string?  JAM :-(
    std::string text="ERROR on writing sfp:"+std::to_string(com.sfp)+" ,slave:"+std::to_string(com.slave)+" to address: 0x" + hexstream.str() +"\n";
    if (fCommandMessage.compare("OK")==0)
      fCommandMessage=text;
    else
      fCommandMessage+=text; }
  return rev;
}

int gosip::TerminalModule::read (gosip::Command &com)
{
  int rev = 0;
  GOSIP_ASSERT_DEVHANDLE;
  rev = mbspex_slave_rd (fFD_pex, com.sfp, com.slave, com.address, &(com.value));
  if (rev == 0)
  {
    if (com.command == GOSIP_READ)
      com.output ();    // only do output if we have explicit read, suppress during verify
  }
  else
  {
    com.dump_command ();
    std::stringstream hexstream;
    hexstream << std::hex << com.address; // no better way to do hex formatting with std::string?  JAM :-(
    std::string text="ERROR on reading sfp:"+std::to_string(com.sfp)+" ,slave:"+std::to_string(com.slave)+" from address: 0x" + hexstream.str() +"\n";

    if (fCommandMessage.compare("OK")==0)
      fCommandMessage=text;
    else
      fCommandMessage+=text;

  }
  return rev;
}

int gosip::TerminalModule::changebits (gosip::Command &com)
{
  int rev = 0;
  long bitmask = 0;
  GOSIP_ASSERT_DEVHANDLE;
  bitmask = com.value;
  std::stringstream hexstream;
  hexstream << std::hex << com.address; // no better way to do hex formatting with std::string?  JAM :-(
  rev = mbspex_slave_rd (fFD_pex, com.sfp, com.slave, com.address, &(com.value));
  if (rev != 0)
  {
      EOUT ("ERROR on reading in change bit at address 0x%lx!\n", com.address);
    fCommandMessage+=  "ERROR on reading  in changebits at sfp:"+std::to_string(com.sfp)+" ,slave:"
        +std::to_string(com.slave)+", address:0x"
        + hexstream.str();

  }

  switch (com.command)
  {
    case GOSIP_SETBIT:
      com.value |= bitmask;
      break;
    case GOSIP_CLEARBIT:
      com.value &= ~bitmask;
      break;
    default:
      break;
  }
  rev = mbspex_slave_wr (fFD_pex, com.sfp, com.slave, com.address, com.value);
  if (rev != 0)
  {
    EOUT ("ERROR on writing in change bit at address 0x%lx!\n", com.address);
    fCommandMessage+=  "ERROR on writing  in changebits at sfp:"+std::to_string(com.sfp)+" ,slave:"+std::to_string(com.slave)+", address:0x"
        + hexstream.str();

  }
  com.value = bitmask;    // restore in case of broadcast command
  return rev;
}

int gosip::TerminalModule::busio (gosip::Command &com)
{
  int rev = 0;
  int cursor = 0;
  long savedaddress = 0;
  GOSIP_ASSERT_DEVHANDLE;
  savedaddress = com.address;
  while (cursor < com.repeat)
  {
    switch (com.command)
    {
      case GOSIP_READ:
        rev = this->read (com);
        // TODO: store last repeat results for sending back to client.
        break;
      case GOSIP_WRITE:
        rev = this->write (com);
        break;
      case GOSIP_SETBIT:
      case GOSIP_CLEARBIT:
        rev = this->changebits (com);
        break;
      default:
        EOUT ("NEVER COME HERE: goscmd_busio called with wrong command %s", com.get_description ());
        return -1;
    }

    fCommandSfp.push_back (com.sfp);     // mind broadcast mode
    fCommandSlave.push_back (com.slave); // mind broadcast mode
    fCommandAddress.push_back (com.address); // mind repeat mode
    fCommandResults.push_back (com.value); // actual read or modified value
    cursor++;
    com.address += 4;
  }    // while
  com.address = savedaddress;    // restore initial base address for slave broadcast option
  return rev;
}

int  gosip::TerminalModule::reset (gosip::Command &com)
{
  GOSIP_ASSERT_DEVHANDLE;
  int rev=mbspex_reset (fFD_pex);
  fCommandMessage= (rev ? "ERROR on resetting gosip!" : "Resetted gosip.");
  return rev;
}

int  gosip::TerminalModule::init (gosip::Command &com)
{
  GOSIP_ASSERT_DEVHANDLE;
  int rev=mbspex_slave_init (fFD_pex, com.sfp, com.slave);
  fCommandMessage= rev ? "!ERROR when initializing " : "Initialized ";
  fCommandMessage += "sfp "+ std::to_string (com.sfp) + " for "+ std::to_string (com.slave) + " slaves";
  return rev;
}

int gosip::TerminalModule::configure (gosip::Command &com)
{
  int rev = 0;
#ifdef GOSIPCMD_BLOCKCONFIG
  int numconfs = 0;
  struct pex_bus_config theConfig;
#else
  gos_cmd_id mastercommand;
#endif

  GOSIP_ASSERT_DEVHANDLE;
  if (open_configuration (com) < 0)
    return -1;
  DOUT0("Configuring from file %s - \n", com.filename);
#ifndef GOSIPCMD_BLOCKCONFIG
  mastercommand = com.command;
#endif
  numconfs = 0;
  while ((rev = next_config_values (com)) != -1)
  {
    if (rev == 0)
      continue;    // skip line

    // TODO: put together configuration structure to be executed in kernel module "atomically"
    // requires new ioctl "configure"
#ifdef GOSIPCMD_BLOCKCONFIG
    if (com.verboselevel)
    {
      DOUT0 ("Config: %d", numconfs);
      com.dump_command ();
    }
    theConfig.param[numconfs].sfp = com.sfp;
    theConfig.param[numconfs].slave = com.slave;
    theConfig.param[numconfs].address = com.address;
    theConfig.param[numconfs].value = com.value;
    theConfig.numpars = ++numconfs;
    if (numconfs >= PEX_MAXCONFIG_VALS)
    {
      // JAM 2016: workaround for configurations above 60 entries:
      // need to send it in separate chunks
      rev = mbspex_slave_config (fFD_pex, &theConfig);
      if (rev)
        break;
      numconfs = 0;    // fill next config bundle
      // break;
    }
#else
    if ((com.command == GOSIP_SETBIT) || (com.command == GOSIP_CLEARBIT))
    {
      if ((rev=changebits (com)) != 0)
      break;
      com.command = mastercommand;    // reset command descriptor
    }
    else
    {
      if ((rev=this->write (com)) != 0)
      break;
    }
#endif

  }
#ifdef GOSIPCMD_BLOCKCONFIG
  rev = mbspex_slave_config (fFD_pex, &theConfig);
#endif

  if (rev)
    {
      EOUT ("ERROR during configuration!\n");
      fCommandMessage= "ERROR during configuration ";
    }
    else
    {

      DOUT0 ("Done.\n");
      fCommandMessage= "Configuration done ";
    }
  fCommandMessage+= "from file ";
  fCommandMessage+= com.filename;
  close_configuration ();
  return rev;

}

int gosip::TerminalModule::verify (gosip::Command &com)
{
  int rev = 0;
  gos_cmd_id mastercommand;
  GOSIP_ASSERT_DEVHANDLE;
  mastercommand = com.command;
  if (open_configuration (com) < 0)
    return -1;
  DOUT0 ("Verifying actual configuration with file %s - \n", com.filename);
  while ((rev = next_config_values (com)) != -1)
  {
    if (rev == 0)
      continue;    // skip line

    if ((com.sfp == -1 || com.slave == -1))
    {
      com.broadcast = 1;    // allow implicit broadcast read switched by -1 argument for sfp or slave
      com.verify = 1;    // mark special mode
      rev = execute_command (com);    // this will handle broadcast
    }
    else
    {
      rev = verify_single (com);
    }
    if (rev)
    {
      DOUT0 ("Verify read ERROR for line %d from configuration file %s\n", fLinecount, com.filename);
    }
    com.command = mastercommand;    // reset command descriptor

  }    // while file

  EOUT ("Verify found %d errors\n", com.errcount);
  fCommandMessage="Verify found " + std::to_string (com.errcount) + " errors.";
  close_configuration ();
  return 0;

}

int gosip::TerminalModule::verify_single  (gosip::Command &com)
{
  int haserror = 0;
  long checkvalue = com.value;    // this is reference value from file

  if (this->read (com) != 0)
    return -1;

  switch (com.command)
  {
    case GOSIP_SETBIT:
      if ((checkvalue & com.value) != checkvalue)
        haserror = 1;
      break;
    case GOSIP_CLEARBIT:
      if ((checkvalue & ~com.value) != checkvalue)
        haserror = 1;
      break;
    default:
      if (checkvalue != com.value)
        haserror = 1;
      break;

  };

  if (haserror)
  {
    fErrcount++;
    if (com.verboselevel)
    {
      DOUT0 (" Verify ERROR %d at sfp %ld slave %ld address 0x%lx : readback=0x%lx, config=0x%lx", fErrcount,
          com.sfp, com.slave, com.address, com.value, checkvalue);
      if (com.command != gosip::GOSIP_VERIFY)
        DOUT0 (" (%s)", com.get_description ());
      else
        DOUT0 (" ");
    }
  }
  com.value = checkvalue;    // restore my soul for broadcast mode
  return 0;

}

int gosip::TerminalModule::broadcast (gosip::Command &com)
{
  int rev = 0;
  int slavebroadcast=0, sfpbroadcast = 0;
  long sfpmax;
  long slavemax;
  if (com.verboselevel)
    com.dump_command ();


  com.broadcast = 0;
  // TODO: add here mode for real broadcast over all configured slaves
  if (com.sfp < 0)
    sfpbroadcast = 1;
  if (com.slave < 0)
    slavebroadcast = 1;
  struct pex_sfp_links slavesetup;
  if (mbspex_get_configured_slaves (fFD_pex, &slavesetup) < 0)
    return -1;
  DOUT3 ("goscmd_broadcast with sfpbroadcast:%d slavebroadcast:%d", sfpbroadcast,slavebroadcast);
  if (sfpbroadcast) /* broadcast over sfps*/
  {
    for (com.sfp = 0; com.sfp < PEX_SFP_NUMBER; ++com.sfp)
    {
      slavemax = slavesetup.numslaves[com.sfp];
      if (slavemax == 0)
        continue;

      if (slavebroadcast) /* also broadcast all slaves*/
      {
        for (com.slave = 0; com.slave < slavemax; ++com.slave)
        {
          rev = execute_command (com);
          if (rev != 0)
          {
            EOUT ("Error in sfp and slave broadcast at sfp:%ld slave:%ld\n", com.sfp, com.slave);
          }
        }
      }
      else /* single slave at all sfps:*/
      {
        rev = execute_command (com);
        if (rev != 0)
        {
          EOUT ("Error in sfp broadcast at sfp:%ld slave:%ld\n", com.sfp, com.slave);
        }
      }

    }    // for com->sfp
  }    // if sfp broadcast
  else
  {

    if (slavebroadcast)
    {
      /* broadcast all slaves at single sfp:*/
      slavemax = slavesetup.numslaves[com.sfp];
      for (com.slave = 0; com.slave < slavemax; ++com.slave)
      {
        rev = execute_command (com);
        if (rev != 0)
        {
          EOUT ("Error in slave broadcast at sfp:%ld slave:%ld\n", com.sfp, com.slave);
        }
      }
    }
    else
    {
      /* explicit broadcast over given boundaries: (old style)*/
      sfpmax = com.sfp;
      slavemax = com.slave;
      for (com.sfp = 0; com.sfp <= sfpmax; ++com.sfp)
      {
        for (com.slave = 0; com.slave <= slavemax; ++com.slave)
        {
          DOUT3 ("Explicit broadcast (0..%ld) (0..%ld) at sfp:%ld slave:%ld", sfpmax,slavemax, com.sfp, com.slave);
          rev = execute_command (com);
          if (rev != 0)
          {
            EOUT ("Error in broadcast (0..%ld) (0..%ld) at sfp:%ld slave:%ld\n", sfpmax, slavemax, com.sfp,
                com.slave);
          }
        }    // for slave
      }    // for sfp
    }    // end no slave broadcast
  }    // end sfp not broadcast
  return rev;
}

int gosip::TerminalModule::execute_command (gosip::Command &com)
{
  int rev = 0;

  if (com.broadcast)
    return broadcast (com); /* dispatch from top level to broadcast mode*/
  if (com.verify)
    return verify_single (com); /* for broadcast mode verify, if execute_command is invoked by goscmd_broadcast*/
  switch (com.command)
  {
    case GOSIP_RESET:
      rev = reset (com);
      break;
    case GOSIP_INIT:
      rev = init (com);
      break;
    case GOSIP_READ:
    case GOSIP_WRITE:
    case GOSIP_SETBIT:
    case GOSIP_CLEARBIT:
      rev = busio (com);
      break;
    case GOSIP_CONFIGURE:
      rev = configure (com);
      break;
    case GOSIP_VERIFY:
      rev = verify (com);
      break;
    default:
      EOUT ("Error: Unknown command %d \n", com.command);
      rev = -2;
      break;
  };
  return rev;
}



