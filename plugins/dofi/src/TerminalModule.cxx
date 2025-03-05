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
 * agreements as stated in LICENSE.fSPI_txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/


#include "dofi/TerminalModule.h"
#include "dabc/Manager.h"
#include "dabc/Publisher.h"

#include <unistd.h>
#include <vector>

#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//~ /** toggle configuration mode: send config from file as data block to driver (1), or write with single bus commands (0)*/
//~ #define DOFICMD_BLOCKCONFIG 1




//~ /* helper macro to check spi device in each command function*/
#define DOFI_ASSERT_DEVHANDLE if(!com.assert_command ()) return -1; \
  if(fFD_spi==0) return -2; \
  if (com.verboselevel) com.dump_command ();

dofi::TerminalModule::TerminalModule (const std::string &name, dabc::Command cmd) :
    dabc::ModuleAsync (name, cmd), fFD_spi(0),fConfigfile(nullptr), fLinecount(0)
{
	fSPI_Device = Cfg("SPI_Device", cmd).AsStr(DOFI_SPIDEV_DEFAULT); // may change these parameters in dabc xml config file
	fSPI_mode =Cfg("SPI_Mode", cmd).AsInt(0);
	fSPI_bits  =Cfg("SPI_Bits", cmd).AsInt(8);
	fSPI_speed=Cfg("SPI_Speed", cmd).AsInt(20000000);


	// below definitions for web server:

   fWorkerHierarchy.Create("DOFI", true);

   // web client has same command structure as dabc socket command:1
   //dabc::CommandDefinition cmddef = CreateCmdDef("DofiCommand");

   dabc::CommandDefinition cmddef = fWorkerHierarchy.CreateHChild("DofiCommand");
   cmddef.SetField(dabc::prop_kind, "DABC.Command");
    //cmddef.SetField(dabc::prop_auth, true); // require authentication
   cmddef.AddArg("COMMAND", "int", true, dofi::DOFI_READ);
   cmddef.AddArg("VERBOSELEVEL", "int", true, 0);
   cmddef.AddArg("HEXFORMAT", "int", true, 0);
   cmddef.AddArg("ADDRESS", "int", true, 0);
   cmddef.AddArg("VALUE", "uint", true, 0);
   cmddef.AddArg("REPEAT", "int", true, 1);
   cmddef.AddArg("FILENAME", "string", true, 1);

   dabc::Hierarchy ui = fWorkerHierarchy.CreateHChild("UI");
   ui.SetField(dabc::prop_kind, "DABC.HTML");
   ui.SetField("_UserFilePath", "dabc_plugins/dofi/htm/");
   ui.SetField("_UserFileMain", "main.htm");

   CreateTimer("update", 1.);
   //Publish(fWorkerHierarchy, std::string("/Control/") + fSPI_Device);
   PublishPars("dofi");



}

dofi::TerminalModule::~TerminalModule()
{
  SPI_close_device();
}

int dofi::TerminalModule::ExecuteCommand (dabc::Command cmd)
{
  int l_status = 0;

   DOUT2("dofi::TerminalModule::ExecuteCommand for %s",cmd.GetName());




  if (cmd.IsName ("DofiCommand") || cmd.IsName(dabc::CmdHierarchyExec::CmdName()))
  {

    if(cmd.IsName(dabc::CmdHierarchyExec::CmdName()))
    {
		 std::string cmdpath = cmd.GetStr("Item");
   	     DOUT2("dofi::TerminalModule::ExecuteCommand has command path %s",cmdpath.c_str());
		 // here suppress wrong hierarchy commmands
		if(cmdpath.compare("DofiCommand")!=0) return dabc::cmd_false;
	}


    dofi::Command theCommand;
    theCommand.ExtractDabcCommand (cmd);
    if (theCommand.verboselevel > 1)
    {
      DOUT0("Received the following commmand:");
      theCommand.dump_command ();
    }

    if(!theCommand.assert_command ()) return -1;
    fCommandAddress.clear (); // for broadcast case: do not clear resultsin inner functions!
    fCommandResults.clear ();
    fCommandMessage="OK";

    l_status = execute_command (theCommand);

    // put here all relevant return values:
    // for repeat read, we set all single read information for client:
    for (unsigned int r = 0; r < fCommandResults.size (); ++r)
    {
      std::string name = "VALUE_" + std::to_string (r);
      cmd.SetUInt (name.c_str (), fCommandResults[r]);
      std::string address = "ADDRESS_" + std::to_string (r);
      cmd.SetInt (address.c_str (), fCommandAddress[r]);
    }
    cmd.SetInt ("NUMRESULTS", fCommandResults.size());
    cmd.SetInt ("VALUE", l_status); // pass back return code
    cmd.SetStr ("MESSAGE",fCommandMessage.c_str());
    return l_status ? dabc::cmd_false : dabc::cmd_true;
  }
  return dabc::cmd_false;
}

void dofi::TerminalModule::BeforeModuleStart ()
{
  DOUT0("Starting DOFI command server module");
  SPI_open_device ();
}

void dofi::TerminalModule::ProcessTimerEvent (unsigned)
{
}




int dofi::TerminalModule::open_configuration (dofi::Command &com)
{
// JAM 10-02-23 - code taken from gosip plugin and adjusted to dofi
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

int dofi::TerminalModule::close_configuration ()
{
  fclose (fConfigfile);
  return 0;
}

int dofi::TerminalModule::next_config_values (dofi::Command &com)
{
  /* file parsing code was partially stolen from trbcmd.c Thanks Ludwig Maier et al. for this!*/
  int status = 0;
  size_t linelen = 0;
  char *cmdline;
  char cmd[DOFI_CMD_MAX_ARGS][DOFI_CMD_SIZE];
  int i, cmdlen;

  char *c = NULL;
  for (i = 0; i < DOFI_CMD_MAX_ARGS; i++)
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

  for (i = 0, cmdlen = 0; i < DOFI_CMD_MAX_ARGS; i++, cmdlen++)
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

  if (!(cmdlen >= 2))
  {
    EOUT ("Invalid configuration data at line %d, (cmdlen=%d)stopping configuration\n", fLinecount, cmdlen);
    return -1;
  }
  com.address = strtoul (cmd[0], NULL, com.hexformat == 1 ? 16 : 0);
  com.value = strtoul (cmd[1], NULL, com.hexformat == 1 ? 16 : 0);
  if (cmdlen > 2)
  {
    // parse optional command identifier
    //if (strcasestr (cmd[2], "setbit") != 0)
    //JAM2017 -  this was not posix standard, gcc 6.3 doesnt like it. we do workaround:
    if ((strstr (cmd[2], "setbit") != nullptr) || (strstr (cmd[2], "SETBIT") != nullptr))
    {
      com.command = DOFI_SETBIT;
    }
    //else if (strcasestr (cmd[4], "clearbit") != 0)
    if ((strstr (cmd[2], "clearbit") != nullptr) || (strstr (cmd[2], "CLEARBIT") != nullptr))
    {
      com.command = DOFI_CLEARBIT;
    }
    else
    {
      // do not change mastercommand id (configure or verify)
    }

  }
  return status;
}

int dofi::TerminalModule::write (dofi::Command &com)
{
  DOFI_ASSERT_DEVHANDLE;
  uint16_t address=(com.address & 0xFFFF);
  uint64_t data=com.value;
  try
  {
    SPI_write(address, data);
    return 0;
  }
  catch(dabc::Exception &ex)
  {
    com.dump_command ();
    std::stringstream hexstream;
    hexstream << std::hex << com.address; // no better way to do hex formatting with std::string?  JAM :-(
    std::string text="ERROR on writing to address: 0x" + hexstream.str() +": ";
    text += ex.ItemName() + " - "+ ex.what() + "\n";

    if (fCommandMessage.compare("OK")==0)
      fCommandMessage=text;
    else
      fCommandMessage+=text;

    EOUT("%s", text.c_str()); // for local server log
  	com.dump_command ();
    return -1;   // TODO: throw further here?
    }
  return 0;
}

int dofi::TerminalModule::read (dofi::Command &com)
{
  DOFI_ASSERT_DEVHANDLE;
  uint16_t address=(com.address & 0xFFFF);
  try
  {
     com.value=SPI_read(address);
  	 return 0;
   }
  catch(dabc::Exception &ex)
  {
    std::stringstream hexstream;
    hexstream << std::hex << com.address; // no better way to do hex formatting with std::string?  JAM :-(
    std::string text="ERROR on reading from address: 0x" + hexstream.str() +": ";
	text += ex.ItemName() + " - "+ ex.what() + "\n";
    if (fCommandMessage.compare("OK")==0)
      fCommandMessage=text;
    else
      fCommandMessage+=text;
  	EOUT("%s", text.c_str()); // for local server log
  	com.dump_command ();
  return -1; // TODO: throw further here?
  }

  return 0;
}

int dofi::TerminalModule::changebits (dofi::Command &com)
{
  int rev = 0;
  unsigned long long bitmask = 0;
  DOFI_ASSERT_DEVHANDLE;
  bitmask = com.value;
  std::stringstream hexstream;
  hexstream << std::hex << com.address; // no better way to do hex formatting with std::string?  JAM :-(
  rev = this->read(com);
  if (rev != 0)
  {
      EOUT ("ERROR on reading in change bit at address 0x%lx!\n", com.address);
    fCommandMessage+=  "ERROR on reading  in changebits at address 0x:"
        + hexstream.str();
	return rev;
  }

  switch (com.command)
  {
    case DOFI_SETBIT:
      com.value |= bitmask;
      break;
    case DOFI_CLEARBIT:
      com.value &= ~bitmask;
      break;
    default:
      break;
  }
  rev = this->write(com);
  if (rev != 0)
  {
	  EOUT ("ERROR on writing in change bit at address 0x%lx!\n", com.address);
	  fCommandMessage+=  "ERROR on writing  in changebits at address:0x"
	  + hexstream.str();
	  return rev;
  }
  com.value = bitmask;    // restore in case of broadcast command
  return rev;
}

int dofi::TerminalModule::busio (dofi::Command &com)
{
  int rev = 0;
  int cursor = 0;
  long savedaddress = 0;
  DOFI_ASSERT_DEVHANDLE;
  savedaddress = com.address;
  while (cursor < com.repeat)
  {
    switch (com.command)
    {
      case DOFI_READ:
        rev = this->read (com);
        // TODO: store last repeat results for sending back to client.
        break;
      case DOFI_WRITE:
        rev = this->write (com);
        break;
      case DOFI_SETBIT:
      case DOFI_CLEARBIT:
        rev = this->changebits (com);
        break;
      default:
        EOUT ("NEVER COME HERE: goscmd_busio called with wrong command %s", com.get_description ());
        return -1;
    }

    fCommandAddress.push_back (com.address); // mind repeat mode
    fCommandResults.push_back (com.value); // actual read or modified value
    cursor++;
    com.address += 1; //4; only for PCIe bus etc.
  }    // while
  com.address = savedaddress;    // restore initial base address for slave broadcast option
  return rev;
}

int  dofi::TerminalModule::reset (dofi::Command &com)
{
  DOFI_ASSERT_DEVHANDLE;
  // JAM 10-02-2023: for the moment, there is no dedicated reset command for the fpga
  // we just close and open spi connection again:
  SPI_close_device();
  try
  {
  	SPI_open_device();
  }
   catch(dabc::Exception &ex)
   {
    std::string text="Error on resetting dofi SPI defice: ";
	text += ex.ItemName() + " - "+ ex.what() + "\n";
      fCommandMessage=text;
  	EOUT("%s", text.c_str()); // for local server log
  	com.dump_command ();
  return -1; // TODO: throw further here?
  }
  fCommandMessage= ("Resetted dofi SPI device.");
  return 0;
}

int dofi::TerminalModule::configure (dofi::Command &com)
{
  int rev = 0;
  dof_cmd_id mastercommand;

  DOFI_ASSERT_DEVHANDLE;
  if (open_configuration (com) < 0)
    return -1;
  DOUT0("Configuring from file %s - \n", com.filename);
  mastercommand = com.command;
  try{
		  while ((rev = next_config_values (com)) != -1)
		  {
			if (rev == 0)
			  continue;    // skip line

			// TODO: put together configuration structure to be executed in kernel module "atomically"
			// requires new ioctl "configure"
			if ((com.command == DOFI_SETBIT) || (com.command == DOFI_CLEARBIT))
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

		  } // while

  }// try
  catch(dabc::Exception &ex)
  {
    std::string text="ERROR during configuration from file";
    text += com.filename;
    text += ex.ItemName();
    text += " - ";
    text += ex.what();
    text += "\n";
    if (fCommandMessage.compare("OK")==0)
      fCommandMessage=text;
    else
      fCommandMessage+=text;
  	EOUT("%s", text.c_str()); // for local server log
  	com.dump_command ();
  return -1; // TODO: throw further here?
  }

  DOUT0 ("Done.\n");
  fCommandMessage= "Configuration done ";
  fCommandMessage+= "from file ";
  fCommandMessage+= com.filename;
  close_configuration ();
  return rev;
}

int dofi::TerminalModule::single_command (dofi::Command &com)
{
	 DOFI_ASSERT_DEVHANDLE;
	 spi_cmd_token token;
	 switch (com.command)
  {
	case dofi::DOFI_RESET_SCALER:
       token=DOFI_SPI_SCALER_RESET;
        break;
    case dofi::DOFI_ENABLE_SCALER:
 		token=DOFI_SPI_SCALER_ENABLE;
         break;
 	case dofi::DOFI_DISABLE_SCALER:
  		 token=DOFI_SPI_SCALER_DISABLE;
  		 break;
  	default:
  		EOUT("NEVER COME HERE: dofi::TerminalModule::single_command with not suitable command %d (%s)", com.command, com.get_description());
  	return -1;
  	break;

  }

try
  {
     SPI_single_command(token);
  	 return 0;
   }
  catch(dabc::Exception &ex)
  {
    std::string text="ERROR on command -";
    text += com.get_description();
    text += " - ";
	text += ex.ItemName() + " - "+ ex.what() + "\n";
    if (fCommandMessage.compare("OK")==0)
      fCommandMessage=text;
    else
      fCommandMessage+=text;
  	EOUT("%s", text.c_str()); // for local server log
  	com.dump_command ();
  return -1; // TODO: throw further here?
  }
}




int dofi::TerminalModule::execute_command (dofi::Command &com)
{
  int rev = 0;

  switch (com.command)
  {
    case DOFI_RESET:
      rev = reset (com);
      break;
    //~ case DOFI_INIT:
      //~ rev = init (com);
      //~ break;
    case DOFI_READ:
    case DOFI_WRITE:
    case DOFI_SETBIT:
    case DOFI_CLEARBIT:
      rev = busio (com);
      break;

    case DOFI_CONFIGURE:
      rev = configure (com);
      break;

    case dofi::DOFI_RESET_SCALER:
    case dofi::DOFI_ENABLE_SCALER:
 	case dofi::DOFI_DISABLE_SCALER:
 	rev= single_command(com);
       break;


    default:
      EOUT ("Error: Unknown command %d \n", com.command);
      rev = -2;
      break;
  };
  return rev;
}


//------------------------------------------------------------------------------------------
// JAM23: below low level functions, adjusted from Nik's spi_muppet1_rw :


int dofi::TerminalModule::SPI_open_device ()
{
  int ret=0;
  if(fFD_spi) return -2;
  DOUT0("Opening spi handle..");
  fFD_spi = open(fSPI_Device.c_str(), O_RDWR);
  if (fFD_spi < 0)
  {
    EOUT ("ERROR>> open %s\n", fSPI_Device.c_str());
    throw dabc::Exception("Error when opening handle!", "dofi::TerminalModule::open_device() ");
    }


  ret = ioctl(fFD_spi, SPI_IOC_WR_MODE, &fSPI_mode);
  if (ret == -1 ) throw dabc::Exception("Can't set spi mode", "dofi::TerminalModule::open_device() ");


  ret = ioctl(fFD_spi, SPI_IOC_RD_MODE, &fSPI_mode);
  if (ret == -1) throw dabc::Exception("Can't get spi mode", "dofi::TerminalModule::open_device() ");

   ret = ioctl(fFD_spi, SPI_IOC_WR_BITS_PER_WORD, &fSPI_bits);
   if (ret == -1) throw dabc::Exception("Can't set bits per word", "dofi::TerminalModule::open_device() ");

   ret = ioctl(fFD_spi, SPI_IOC_RD_BITS_PER_WORD, &fSPI_bits);
   if (ret == -1) throw dabc::Exception("Can't get bits per word", "dofi::TerminalModule::open_device() ");

   ret = ioctl(fFD_spi, SPI_IOC_WR_MAX_SPEED_HZ, &fSPI_speed);
   if (ret == -1) throw dabc::Exception("Can't set max speed Hz", "dofi::TerminalModule::open_device() ");

   ret = ioctl(fFD_spi, SPI_IOC_RD_MAX_SPEED_HZ, &fSPI_speed);
   if (ret == -1) throw dabc::Exception("Can't get max speed Hz", "dofi::TerminalModule::open_device() ");

  fSPI_transfer.tx_buf = (unsigned long) fSPI_tx;
  fSPI_transfer.rx_buf = (unsigned long) fSPI_rx;
  fSPI_transfer.speed_hz      = fSPI_speed;
  fSPI_transfer.bits_per_word = fSPI_bits;

  DOUT0("Initialized spi device %s", fSPI_Device.c_str());
  DOUT0(" - spi mode: \t%d", fSPI_mode);
  DOUT0(" - bits per word: \t%d", fSPI_bits);
  DOUT0(" - max speed: %d Hz (%d KHz)\n", fSPI_speed, fSPI_speed/1000);
  return 0;
}






int dofi::TerminalModule::SPI_write (uint16_t l_addrx, uint64_t l_wr_datx)
{
  int ret=0;
  fSPI_tx[10] = (dofi::TerminalModule::DOFI_SPI_WRITE & 0xff);  // 0x01 write to slave
  fSPI_tx[9]  = ( l_addrx         & 0xff);   	// register address, low  8 bits
  fSPI_tx[8]  = ((l_addrx >> 8)   & 0xff);   	// register address, high 8 bits
  fSPI_tx[7]  =  l_wr_datx        & 0xff;
  fSPI_tx[6]  = (l_wr_datx >>  8) & 0xff;
  fSPI_tx[5]  = (l_wr_datx >> 16) & 0xff;
  fSPI_tx[4]  = (l_wr_datx >> 24) & 0xff;
  fSPI_tx[3]  = (l_wr_datx >> 32) & 0xff;
  fSPI_tx[2]  = (l_wr_datx >> 40) & 0xff;
  fSPI_tx[1]  = (l_wr_datx >> 48) & 0xff;
  fSPI_tx[0]  = (l_wr_datx >> 56) & 0xff;
  fSPI_transfer.len = 11;

  DOUT5("0x%x fSPI_tx  ", l_addrx);
  for (unsigned int i = 0; i < fSPI_transfer.len; i++)
  {
    DOUT5("%.2X ", fSPI_tx[i]);
  }
  DOUT5("\n");


  ret = ioctl(fFD_spi, SPI_IOC_MESSAGE(1), &fSPI_transfer);
  if (ret < 1)
  {
  	std::string message="Can't send ioctl message to spi kernel module " + fSPI_Device;
	throw dabc::Exception(message, "dofi::TerminalModule::SPI_write() ");
  }
   DOUT3("spi write: addr: 0x%x, data: 0x%llx \n", l_addrx, l_wr_datx);
 return ret;
}

//------------------------------------------------------------------------------------------

uint64_t dofi::TerminalModule::SPI_read (uint16_t l_addrx)
{
  int ret=0;
  uint64_t l_rd_datx, l_rd_datx_tmp;

  fSPI_tx[10] = (DOFI_SPI_READ & 0xff); //0x02; read from slave
  fSPI_tx[9]  = ( l_addrx        & 0xff);   // register address, low  8 bits
  fSPI_tx[8]  = ((l_addrx >> 8)  & 0xff);   // register address, high 8 bits
  fSPI_tx[7]  = 0;
  fSPI_tx[6]  = 0;
  fSPI_tx[5]  = 0;
  fSPI_tx[4]  = 0;
  fSPI_tx[3]  = 0;
  fSPI_tx[2]  = 0;
  fSPI_tx[1]  = 0;
  fSPI_tx[0]  = 0;
  fSPI_transfer.len = 11;

  ret = ioctl(fFD_spi, SPI_IOC_MESSAGE(1), &fSPI_transfer);
  if (ret < 1)
  	{
  	std::string message="Can't send first ioctl message to spi kernel module " + fSPI_Device;
	throw dabc::Exception(message, "dofi::TerminalModule::SPI_read() ");
  }
  fSPI_tx[10] = (DOFI_SPI_READ & 0xff); // read from slave
  fSPI_tx[9]  = ( l_addrx        & 0xff);   // register address, low  8 bits
  fSPI_tx[8]  = ((l_addrx >> 8)  & 0xff);   // register address, high 8 bits
  fSPI_tx[7]  = 0;
  fSPI_tx[6]  = 0;
  fSPI_tx[5]  = 0;
  fSPI_tx[4]  = 0;
  fSPI_tx[3]  = 0;
  fSPI_tx[2]  = 0;
  fSPI_tx[1]  = 0;
  fSPI_tx[0]  = 0;
  fSPI_transfer.len = 11;

  ret = ioctl(fFD_spi, SPI_IOC_MESSAGE(1), &fSPI_transfer);
  if (ret < 1)
  {
	  std::string message="Can't send second ioctl message to spi kernel module ";
	  message += fSPI_Device;
	  throw dabc::Exception(message, "dofi::TerminalModule::SPI_read() ");
	}


  DOUT5("0x%x rx  ", l_addrx);
  for (unsigned int i = 0; i < fSPI_transfer.len; i++)
  {
    DOUT5("%.2X ", fSPI_rx[i]);
  }
  DOUT5("\n");

  l_rd_datx = 0;
  l_rd_datx_tmp = fSPI_rx[3]; l_rd_datx_tmp <<= 56; l_rd_datx += l_rd_datx_tmp;
  l_rd_datx_tmp = fSPI_rx[4]; l_rd_datx_tmp <<= 48; l_rd_datx += l_rd_datx_tmp;
  l_rd_datx_tmp = fSPI_rx[5]; l_rd_datx_tmp <<= 40; l_rd_datx += l_rd_datx_tmp;
  l_rd_datx_tmp = fSPI_rx[6]; l_rd_datx_tmp <<= 32; l_rd_datx += l_rd_datx_tmp;
  l_rd_datx_tmp = fSPI_rx[7]; l_rd_datx_tmp <<= 24; l_rd_datx += l_rd_datx_tmp;
  l_rd_datx_tmp = fSPI_rx[8]; l_rd_datx_tmp <<= 16; l_rd_datx += l_rd_datx_tmp;
  l_rd_datx_tmp = fSPI_rx[9]; l_rd_datx_tmp <<=  8; l_rd_datx += l_rd_datx_tmp;
  l_rd_datx_tmp = fSPI_rx[10];                      l_rd_datx += l_rd_datx_tmp;

  DOUT3("spi  read: addr: 0x%x, data: 0x%llx \n", l_addrx, l_rd_datx);
  return l_rd_datx;
}

//------------------------------------------------------------------------------------------



int dofi::TerminalModule::SPI_single_command (spi_cmd_token token)
{
  fSPI_tx[10] = (token & 0xFF);     // token known to spi slave
  fSPI_tx[9]  = 0;
  fSPI_tx[8]  = 0;
  fSPI_tx[7]  = 0;
  fSPI_tx[6]  = 0;
  fSPI_tx[5]  = 0;
  fSPI_tx[4]  = 0;
  fSPI_tx[3]  = 0;
  fSPI_tx[2]  = 0;
  fSPI_tx[1]  = 0;
  fSPI_tx[0]  = 0;
  fSPI_transfer.len = 11;

  DOUT5("0x%x fSPI_tx  ", l_addrx);
  for (unsigned int i = 0; i < fSPI_transfer.len; i++)
  {
    DOUT5("%.2X ", fSPI_tx[i]);
  }
  DOUT5("\n");

  int ret = ioctl(fFD_spi, SPI_IOC_MESSAGE(1), &fSPI_transfer);
  if (ret < 1)
  	{
	  std::string message="Can't send ioctl message to spi kernel module ";
	  message += fSPI_Device;
  	  throw dabc::Exception(message, "dofi::TerminalModule::SPI_single_command() ");
	}
  return ret;
}


void dofi::TerminalModule::SPI_close_device()
{
  if(fFD_spi) close (fFD_spi);
  fFD_spi=0;
}
