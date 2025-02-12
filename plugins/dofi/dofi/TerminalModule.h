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


/** DABC Command server for Digital signal Over FIbre (DOFI)device
 * To be run on raspi connected to muppet board via SPI
 * SPI interface code by N.Kurz, EEL, GSI
 * v 0.1 09.02.2023 JAM <j.adamczewski@gsi.de> , EEL, GSI
 * */



#ifndef DOFI_TerminalModule
#define DOFI_TerminalModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#include "dofi/Command.h"

#include <cstdio>
#include <vector>
#include <string>
#include <sstream>

extern "C"
{
#include <linux/types.h>
#include <linux/spi/spidev.h>
}

/** we use this spi device for communication with muppet fpga */
#define DOFI_SPIDEV_DEFAULT "/dev/spidev1.2"


namespace dofi {

   /** \brief Abstract DOFI terminal module
    *
    * Just executes commands
    */

class TerminalModule : public dabc::ModuleAsync {


public:

	TerminalModule(const std::string &name, dabc::Command cmd = nullptr);

	virtual ~TerminalModule();

	void BeforeModuleStart() override;

	void ProcessTimerEvent(unsigned timer) override;



protected:


	/** interface callback of dabc module*/
	int ExecuteCommand(dabc::Command cmd) override;


	/** Actual command execution dispatcher*/
	int execute_command (dofi::Command &com);



	/** open configuration file*/
	int open_configuration (dofi::Command &com);

	/** close configuration file*/
	int close_configuration ();

	/** fill next values from configuration file into com structure.
	 * returns -1 if end of config is reached*/
	int next_config_values (dofi::Command &com);


	/** configure registers from file*/
	int configure (dofi::Command &com);

	/** write to slave address specified in command*/
	int write (dofi::Command &com);

	/** read from slave address specified in command*/
	int read (dofi::Command &com);

	/** set or clear bits of given mask in slave address register*/
	int changebits (dofi::Command &com);

	/** wrapper for all slave operations with incremental address io capabilities*/
	int busio (dofi::Command &com);




	/** reset muppet/dofi device*/
	int reset (dofi::Command &com);


	/** handles all commands that do not require arguments or return values */
	int single_command (dofi::Command &com);


private:



typedef enum
{

  DOFI_SPI_WRITE = 0x1,
  DOFI_SPI_READ = 0x2,
  DOFI_SPI_SCALER_DISABLE = 0x4,
  DOFI_SPI_SCALER_ENABLE = 0x8,
  DOFI_SPI_SCALER_RESET = 0xc
} spi_cmd_token;




	 /** open the spi device and initialize communication*/
	int SPI_open_device ();

	/** close currently active device handle*/
	void SPI_close_device();


	/** low level write data to SPI slave address */
	int SPI_write (uint16_t l_addrx, uint64_t l_wr_datx);

	/** low level read data from SPI slave address.
	 * Returns  the readout value */
	uint64_t SPI_read (uint16_t l_addrx);

	/** low level send single command of token com to  SPI slave
	 * Note that this is for commands that do not require any parameters
	 * or readback values*/
	int SPI_single_command (spi_cmd_token com);




	std::string fSPI_Device; /**< name of spi device for communication. */

	int fFD_spi; /**< file descriptor to driver /dev/spidev1.2"*/

	/** parameters for spi transfer: */
	uint8_t fSPI_mode;// = 0;

	uint8_t fSPI_bits ;//= 8;

	uint32_t fSPI_speed;// = 20000000;

	/** for ioctl to spi device: */
	struct spi_ioc_transfer fSPI_transfer;

	/** transmit buffer for ioctl to spi device: */
	uint8_t fSPI_tx[64];

	/** receive buffer for ioctl to spi device: */
	uint8_t fSPI_rx[64];



	FILE *fConfigfile; /**< handle to configuration file */

	int fLinecount; /**< configfile linecounter*/
  //~ int fErrcount; /**< errorcount for verify*/

	/*** keep result of most recent command call here */
	std::vector<unsigned long long> fCommandResults;

		 /*** keep address of most recent command call here*/
	std::vector<long> fCommandAddress;

		 /*** keep Sfp of most recent command call here, for broadcast*/
	std::vector<long> fCommandSfp;

		  /*** keep Slave of most recent command call here, for broadcast*/
	std::vector<long> fCommandSlave;

	/*** return message after command execution */
	std::string fCommandMessage;




   };
}


#endif
