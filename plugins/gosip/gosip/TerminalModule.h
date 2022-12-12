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

#ifndef GOSIP_TerminalModule
#define GOSIP_TerminalModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif


#ifndef GOSIP_COMMAND_PLAINC
#include "gosip/Command.h"
#endif
#include <stdio.h>

#include <vector>



/****************************************************************/



namespace gosip {

   /** \brief Abstract GOSIP terminal module
    *
    * Just executes commands
    */

   class TerminalModule : public dabc::ModuleAsync {
      protected:



         int ExecuteCommand(dabc::Command cmd) override;

#ifndef GOSIP_COMMAND_PLAINC
// JAM command execution functions are put here for C++ implementation:



/** printout current command structure*/
int execute_command (gosip::Command &com);

/** open the pex device of defined id number and set file descriptor.
 * do nothing if device of given devnum is already open.*/
int open_device (int devnum);

/** close currently active device handle*/
void close_device();

/** open configuration file*/
int open_configuration (gosip::Command &com);

/** close configuration file*/
int close_configuration ();

/** fill next values from configuration file into com structure.
 * returns -1 if end of config is reached*/
int next_config_values (gosip::Command &com);

/** write to slave address specified in command*/
int write (gosip::Command &com);

/** read from slave address specified in command*/
int read (gosip::Command &com);

/** set or clear bits of given mask in slave address register*/
int changebits (gosip::Command &com);

/** wrapper for all slave operations with incremental address io capabilities*/
int busio (gosip::Command &com);

/** initialize sfp chain*/
int init (gosip::Command &com);

/** reset mbspex device*/
int reset (gosip::Command &com);

/** load register values from configuration file*/
int configure (gosip::Command &com);

/** compare register values with configuration file*/
int verify (gosip::Command &com);

/** compare register values with configuration file, primitive function*/
int verify_single (gosip::Command &com);

/** broadcast: loop command operation over several slaves*/
int broadcast (gosip::Command &com);


#endif


#ifndef GOSIP_COMMAND_PLAINC

      protected:
          int fFD_pex; /**< file descriptor to driver /dev/mbspex*/

          int fDevnum; /**< most recent mbspex device number (mostly always 0) */

          FILE *fConfigfile; /**< handle to configuration file */

          int fLinecount; /**< configfile linecounter*/

          int fErrcount; /**< errorcount for verify*/

          /*** keep result of most recent command call here */
          std::vector<long> fCommandResults;

                 /*** keep address of most recent command call here*/
          std::vector<long> fCommandAddress;

                 /*** keep Sfp of most recent command call here, for broadcast*/
          std::vector<long> fCommandSfp;

                  /*** keep Slave of most recent command call here, for broadcast*/
          std::vector<long> fCommandSlave;
#endif


      public:

         TerminalModule(const std::string &name, dabc::Command cmd = nullptr);

         virtual ~TerminalModule();

         void BeforeModuleStart() override;

         void ProcessTimerEvent(unsigned timer) override;

#ifdef GOSIP_COMMAND_PLAINC
         /*** keep result of most recent command call here
          * TODO: later move to Command class*/
         static std::vector<long> fCommandResults;

         /*** keep address of most recent command call here*/
         static std::vector<long> fCommandAddress;

         /*** keep Sfp of most recent command call here, for broadcast*/
          static std::vector<long> fCommandSfp;

          /*** keep Slave of most recent command call here, for broadcast*/
          static std::vector<long> fCommandSlave;
#endif
   };
}


#endif
