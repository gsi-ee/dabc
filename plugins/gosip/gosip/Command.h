#ifndef GOSIP_Command
#define GOSIP_Command

#include"dabc/Command.h"
#include "dabc/logging.h"
#include <stdarg.h>
#include <string.h>


/* this switches between first implementation (copy of gosipcmd C code)
 * and real C++*/
//#define GOSIP_COMMAND_PLAINC 1


#ifdef GOSIP_COMMAND_PLAINC
/* we need to implement printm here to also catch output of libmbspex*/

// this is for printm calls in current lib:
// argument is const because of compiler warnings...
void printm (const char *fmt, ...)
{
  char c_str[256];
  va_list args;
  va_start (args, fmt);
  vsprintf (c_str, fmt, args);
  DOUT0("%s", c_str);
  va_end (args);
}
// later we replace this by DOUT anyway...

/* this is to link printm from mbspex lib:*/
void printm (char *fmt, ...)
{
  char c_str[256];
  va_list args;
  va_start (args, fmt);
  vsprintf (c_str, fmt, args);
  printm ((const char*) c_str);
  va_end (args);
}
//// fortunately linker makes difference here JAM 8-12-22

#define RGOC_DEFAULTPORT 12345

/*** JAM 1-dec-2022: use independent copy of command structure definition here:
 * we could include this from $GOSIPDIR/tests/gosipcmd.h, but this is available on the command server node only*/

#define GOSIP_MAXTEXT 128
#define GOSIP_CMD_SIZE 256
#define GOSIP_CMD_MAX_ARGS 10

typedef enum
{
  GOSIP_NOP,
  GOSIP_RESET,
  GOSIP_INIT,
  GOSIP_READ,
  GOSIP_WRITE,
  GOSIP_SETBIT,
  GOSIP_CLEARBIT,
  GOSIP_CONFIGURE,
  GOSIP_VERIFY
} gos_cmd_id;

struct gosip_cmd
{
  char devnum; /**< logical device number of the pex device to open*/
  gos_cmd_id command; /**< command identifier*/
  char verboselevel; /**< level of debug 0=off*/
  char hexformat; /**< hexoutput(1) or decimal (0)*/
  char broadcast; /**< command is broadcasted to range of 0..sfp/chain*/
  char verify; /**< verify mode*/
  int fd_pex; /**< keep file descriptor here*/
  long sfp; /**< sfp chain (broadcast: max sfp)*/
  long slave; /**< slave id (broadcast: max slave)*/
  long address; /**< address on slave*/
  long value; /**< value to write, or read back*/
  long repeat; /**< number of words for incremental read*/

  char filename[GOSIP_MAXTEXT]; /**< optional name of configuration file*/
  FILE *configfile; /**< handle to configuration file*/
  int linecount; /**< configfile linecounter*/
  int errcount; /**< errorcount for verify*/
};

namespace gosip
{
// some global helper functions, not part of the libDabcGosip!

/* put values of gosipcommand structure to dabc command for transfer*/
void BuildDabCommand (dabc::Command &dest, struct gosip_cmd &src);

/* filll local gosip_cmd structure from received dabc command*/
void ExtractDabcCommand (struct gosip_cmd &dest, dabc::Command &src);

}

// implement them here, separately compiled for command server and rgoc client:

void gosip::BuildDabCommand (dabc::Command &dest, struct gosip_cmd &src)
{
  dest.SetInt ("DEVNUM", src.devnum);
  dest.SetInt ("COMMAND", src.command);
  dest.SetInt ("VERBOSELEVEL", src.verboselevel);
  dest.SetInt ("HEXFORMAT", src.hexformat);
  dest.SetInt ("BROADCAST", src.broadcast);
  dest.SetInt ("VERIFY", src.verify);
  dest.SetInt ("SFP", (int) src.sfp);
  dest.SetInt ("SLAVE", (int) src.slave);
  dest.SetInt ("ADDRESS", (int) src.address);
  dest.SetInt ("VALUE", (int) src.value);
  dest.SetInt ("REPEAT", (int) src.repeat);
  dest.SetStr ("FILENAME", src.filename);
}

void gosip::ExtractDabcCommand (struct gosip_cmd &dest, dabc::Command &src)
{
  dest.devnum = src.GetInt ("DEVNUM");
  dest.command = (gos_cmd_id) (src.GetInt ("COMMAND"));
  dest.verboselevel = src.GetInt ("VERBOSELEVEL");
  dest.hexformat = src.GetInt ("HEXFORMAT");
  dest.broadcast = src.GetInt ("BROADCAST");
  dest.verify = src.GetInt ("VERIFY");
  dest.sfp = src.GetInt ("SFP");
  dest.slave = src.GetInt ("SLAVE");
  dest.address = src.GetInt ("ADDRESS");
  dest.value = src.GetInt ("VALUE");
  dest.repeat = src.GetInt ("REPEAT");
  strncpy (dest.filename, src.GetStr ("FILENAME").c_str (), GOSIP_MAXTEXT);

}

///////////// put all convenient functions here
// to be used by terminalmodule command server and client command exec
// TODO: later convert to class gosip::Command and these functions to static methods

/** check if parameters are set correctly*/
void goscmd_assert_command (struct gosip_cmd *com);

/** check if number of arguments does match the command*/
void goscmd_assert_arguments (struct gosip_cmd *com, int arglen);

/** initialize command structure to defaults*/
void goscmd_defaults (struct gosip_cmd *com);

/** set command structure from id. assert that no duplicate id setups appear */
void goscmd_set_command (struct gosip_cmd *com, gos_cmd_id id);

/** printout current command structure*/
void goscmd_dump_command (struct gosip_cmd *com);

////////////////////////////////////////////////

static char CommandDescription[GOSIP_MAXTEXT];

void goscmd_defaults (struct gosip_cmd *com)
{
  com->devnum = 0;
  com->command = GOSIP_NOP;
  com->verboselevel = 0;
  com->hexformat = 0;
  com->broadcast = 0;
  com->verify = 0;
  com->fd_pex = 0;    // dummy on remote client side, -1;
  com->sfp = -2;
  com->slave = -2;
  com->address = -1;
  com->value = 0;
  com->repeat = 1;
  com->errcount = 0;
  com->filename[0] = 0;
}

void goscmd_set_command (struct gosip_cmd *com, gos_cmd_id id)
{
  if (com->command == GOSIP_NOP)
  {
    com->command = id;
  }
  else
  {
    printm (" gosipcmd ERROR - conflicting command specifiers!\n");
    exit (1);
  }
}

void goscmd_assert_command (struct gosip_cmd *com)
{
  int do_exit = 0;
  if (com->command == GOSIP_NOP)
    do_exit = 1;
  if (com->fd_pex < 0)
    do_exit = 1;
  if ((com->command != GOSIP_CONFIGURE) && (com->command != GOSIP_VERIFY) && (com->command != GOSIP_RESET))
  {
    if (com->sfp < -1) /*allow broadcast statements -1*/
      do_exit = 1;
    if ((com->command != GOSIP_INIT) && (com->slave < -1))
      do_exit = 1;
    if ((com->command != GOSIP_INIT) && (com->address < -1))
      do_exit = 1;
  }
  else
  {
    com->broadcast = 0;    // disable broadcast for configure, verify and reset if it was chosen by mistake
  }

  if ((com->command == GOSIP_READ || com->command == GOSIP_SETBIT || com->command == GOSIP_CLEARBIT)
      && (com->sfp == -1 || com->slave == -1))
  {
    com->broadcast = 1;    // allow implicit broadcast read switched by -1 argument for sfp or slave
  }

  if (do_exit)
  {
    printm (" gosipcmd ERROR - illegal parameters \n");
    goscmd_dump_command (com);
    exit (1);
  }
}

void goscmd_assert_arguments (struct gosip_cmd *com, int arglen)
{
  int do_exit = 0;
  if ((com->command == GOSIP_INIT) && (arglen < 2))
    do_exit = 1;
  if ((com->command == GOSIP_READ) && (arglen < 3))
    do_exit = 1;
  if ((com->command == GOSIP_WRITE) && (arglen < 4))
    do_exit = 1;
  if ((com->command == GOSIP_SETBIT) && (arglen < 4))
    do_exit = 1;
  if ((com->command == GOSIP_CLEARBIT) && (arglen < 4))
    do_exit = 1;
  if (do_exit)
  {
    printm (" gosipcmd ERROR - number of parameters not sufficient for command!\n");
    exit (1);
  }

}

char* goscmd_get_description (struct gosip_cmd *com)
{
  switch (com->command)
  {
    case GOSIP_RESET:
      snprintf (CommandDescription, GOSIP_MAXTEXT, "Reset pexor/kinpex board");
      break;
    case GOSIP_INIT:
      snprintf (CommandDescription, GOSIP_MAXTEXT, "Initialize sfp chain");
      break;
    case GOSIP_READ:
      snprintf (CommandDescription, GOSIP_MAXTEXT, "Read value");
      break;
    case GOSIP_WRITE:
      snprintf (CommandDescription, GOSIP_MAXTEXT, "Write value");
      break;
    case GOSIP_CONFIGURE:
      snprintf (CommandDescription, GOSIP_MAXTEXT, "Configure");
      break;
    case GOSIP_VERIFY:
      snprintf (CommandDescription, GOSIP_MAXTEXT, "Verify   ");
      break;

    case GOSIP_SETBIT:
      snprintf (CommandDescription, GOSIP_MAXTEXT, "Set Bitmask");
      break;
    case GOSIP_CLEARBIT:
      snprintf (CommandDescription, GOSIP_MAXTEXT, "Clear Bitmask");
      break;
    default:
      snprintf (CommandDescription, GOSIP_MAXTEXT, "Unknown command");
      break;
  };
  return CommandDescription;
}

void goscmd_dump_command (struct gosip_cmd *com)
{
  //printm (" gosipcmd dump: \n");
  com->hexformat == 1 ? printm (" Command  :0x%x (%s)", com->command, goscmd_get_description (com)) :
                        printm (" Command: %d (%s)", com->command, goscmd_get_description (com));
  com->hexformat == 1 ? printm ("\t device: 0x%x", com->devnum) : printm ("\t device: %d", com->devnum);
  com->hexformat == 1 ? printm ("\t sfp: 0x%lx", com->sfp) : printm ("\t sfp: %ld", com->sfp);
  com->hexformat == 1 ? printm ("\t slave: 0x%lx", com->slave) : printm (" \t slave: %ld", com->slave);
  com->hexformat == 1 ? printm ("\t address: 0x%lx", com->address) : printm ("\t address: %ld", com->address);
  com->hexformat == 1 ? printm ("\t value: 0x%lx", com->value) : printm ("\t value: %ld", com->value);
  com->hexformat == 1 ? printm ("\t repeat: 0x%lx", com->repeat) : printm ("\t repeat: %ld", com->repeat);
  printm ("\t broadcast: %d \n", com->broadcast);

//  if ((com->command == GOSIP_CONFIGURE) || (com->command == GOSIP_VERIFY))
//  printm (" \t config file    :%s \n", com->filename);
}

int goscmd_output (struct gosip_cmd *com)
{
  if (com->verboselevel)
  {
    com->hexformat ?
        printm ("SFP: 0x%lx Module: 0x%lx Address: 0x%lx  Data: 0x%lx", com->sfp, com->slave, com->address,
            com->value) :
        printm ("SFP: %ld Module: %ld Address: %ld  Data: %ld ", com->sfp, com->slave, com->address, com->value);    //JAM22: not \n for DOUT mode
  }
  else
  {
    com->hexformat ? printm ("0x%lx", com->value) : printm ("%ld", com->value);    //JAM22: not \n for DOUT mode
  }
  return 0;
}


#else //GOSIP_COMMAND_PLAINC
// here improved C++ style implementtation JAM 08-12-22:

#define RGOC_DEFAULTPORT 12345


#define GOSIP_MAXTEXT 128
#define GOSIP_CMD_SIZE 256
#define GOSIP_CMD_MAX_ARGS 10





namespace gosip {



typedef enum
{
  GOSIP_NOP,
  GOSIP_RESET,
  GOSIP_INIT,
  GOSIP_READ,
  GOSIP_WRITE,
  GOSIP_SETBIT,
  GOSIP_CLEARBIT,
  GOSIP_CONFIGURE,
  GOSIP_VERIFY
} gos_cmd_id;



   /** \brief GOSIP command interface
    *
    * Contains command definitions for execution on kinpex node.
    *
    */

   class Command {

   private:

   protected:

   public:


  char devnum; /**< logical device number of the pex device to open*/
  gos_cmd_id command; /**< command identifier*/
  char verboselevel; /**< level of debug 0=off*/
  char hexformat; /**< hexoutput(1) or decimal (0)*/
  char broadcast; /**< command is broadcasted to range of 0..sfp/chain*/
  char verify; /**< verify mode*/
  //int fd_pex; /**< keep file descriptor here TODO: move to terminal class!*/
  long sfp; /**< sfp chain (broadcast: max sfp)*/
  long slave; /**< slave id (broadcast: max slave)*/
  long address; /**< address on slave*/
  long value; /**< value to write, or read back*/
  long repeat; /**< number of words for incremental read*/

  char filename[GOSIP_MAXTEXT]; /**< optional name of configuration file*/
 // FILE *configfile; /**< handle to configuration file */
  int linecount; /**< configfile linecounter*/
  int errcount; /**< errorcount for verify*/



  /**********************************************************************/


     Command():
      devnum(0),command(gosip::GOSIP_NOP),verboselevel(0),hexformat(0),
             broadcast(0), verify(0),sfp(-2),slave(-2),address(-1),
             value(0), repeat(1), linecount(0), errcount(0)
             {
                 filename[0] = 0;
             }

     virtual ~Command(){;}


     bool set_command (gos_cmd_id id)
     {
       if (command == gosip::GOSIP_NOP)
              {
                command = id;
                return true;
              }
              else
              {
                return false;
                //EOUT (" gosipcmd ERROR - conflicting command specifiers!\n");
              }
     }

     static char CommandDescription[GOSIP_MAXTEXT];




     bool assert_command ()
     {
       int do_exit = 0;
       if (command == gosip::GOSIP_NOP)
         do_exit = 1;
//       if (fd_pex < 0)
//         do_exit = 1;
       if ((command != gosip::GOSIP_CONFIGURE) && (command != gosip::GOSIP_VERIFY) && (command != gosip::GOSIP_RESET))
       {
         if (sfp < -1) /*allow broadcast statements -1*/
           do_exit = 1;
         if ((command != gosip::GOSIP_INIT) && (slave < -1))
           do_exit = 1;
         if ((command != gosip::GOSIP_INIT) && (address < -1))
           do_exit = 1;
       }
       else
       {
         broadcast = 0;    // disable broadcast for configure, verify and reset if it was chosen by mistake
       }

       if ((command == gosip::GOSIP_READ || command == gosip::GOSIP_SETBIT || command == gosip::GOSIP_CLEARBIT)
           && (sfp == -1 || slave == -1))
       {
         broadcast = 1;    // allow implicit broadcast read switched by -1 argument for sfp or slave
       }

       if (do_exit)
       {
         EOUT (" gosipcmd ERROR - illegal parameters \n");
         //goscmd_dump_command (com);
         //exit (1);
       return false;
       }
       return true;
     }

     bool assert_arguments (int arglen)
  {
    int do_exit = 0;
    if ((command == gosip::GOSIP_INIT) && (arglen < 2))
      do_exit = 1;
    if ((command == gosip::GOSIP_READ) && (arglen < 3))
      do_exit = 1;
    if ((command == gosip::GOSIP_WRITE) && (arglen < 4))
      do_exit = 1;
    if ((command == gosip::GOSIP_SETBIT) && (arglen < 4))
      do_exit = 1;
    if ((command == gosip::GOSIP_CLEARBIT) && (arglen < 4))
      do_exit = 1;
    if (do_exit)
    {
      EOUT (" gosipcmd ERROR - number of parameters not sufficient for command!\n");
      return false;
    }
    return true;
  }



     char* get_description ()
     {
       switch (command)
       {
         case gosip::GOSIP_RESET:
           snprintf (CommandDescription, GOSIP_MAXTEXT, "Reset pexor/kinpex board");
           break;
         case gosip::GOSIP_INIT:
           snprintf (CommandDescription, GOSIP_MAXTEXT, "Initialize sfp chain");
           break;
         case gosip::GOSIP_READ:
           snprintf (CommandDescription, GOSIP_MAXTEXT, "Read value");
           break;
         case gosip::GOSIP_WRITE:
           snprintf (CommandDescription, GOSIP_MAXTEXT, "Write value");
           break;
         case gosip::GOSIP_CONFIGURE:
           snprintf (CommandDescription, GOSIP_MAXTEXT, "Configure");
           break;
         case gosip::GOSIP_VERIFY:
           snprintf (CommandDescription, GOSIP_MAXTEXT, "Verify   ");
           break;

         case gosip::GOSIP_SETBIT:
           snprintf (CommandDescription, GOSIP_MAXTEXT, "Set Bitmask");
           break;
         case gosip::GOSIP_CLEARBIT:
           snprintf (CommandDescription, GOSIP_MAXTEXT, "Clear Bitmask");
           break;
         default:
           snprintf (CommandDescription, GOSIP_MAXTEXT, "Unknown command");
           break;
       };
       return CommandDescription;
     }

     void dump_command ()
     {
       //printm (" gosipcmd dump: \n");
      hexformat == 1 ? DOUT0 (" Command  :0x%x (%s)", command, get_description ()) :
                             DOUT0 (" Command: %d (%s)", command, get_description ());
      hexformat == 1 ? DOUT0 ("\t device: 0x%x", devnum) : DOUT0 ("\t device: %d", devnum);
      hexformat == 1 ? DOUT0 ("\t sfp: 0x%lx", sfp) : DOUT0 ("\t sfp: %ld", sfp);
      hexformat == 1 ? DOUT0 ("\t slave: 0x%lx", slave) : DOUT0 (" \t slave: %ld", slave);
      hexformat == 1 ? DOUT0 ("\t address: 0x%lx", address) : DOUT0 ("\t address: %ld", address);
      hexformat == 1 ? DOUT0 ("\t value: 0x%lx", value) : DOUT0 ("\t value: %ld", value);
      hexformat == 1 ? DOUT0 ("\t repeat: 0x%lx", repeat) : DOUT0 ("\t repeat: %ld", repeat);
       DOUT0 ("\t broadcast: %d \n", broadcast);

     //  if ((com->command == GOSIP_CONFIGURE) || (com->command == GOSIP_VERIFY))
     //  DOUT0 (" \t config file    :%s \n", com->filename);
     }


     void output ()
     {
       if (verboselevel)
       {
         hexformat ?
             DOUT0 ("SFP: 0x%lx Module: 0x%lx Address: 0x%lx  Data: 0x%lx", sfp, slave, address,
                 value) :
             DOUT0 ("SFP: %ld Module: %ld Address: %ld  Data: %ld ", sfp, slave, address, value);
       }
       else
       {
         hexformat ? DOUT0 ("0x%lx", value) : DOUT0 ("%ld", value);
       }
     }


     void BuildDabCommand (dabc::Command &dest)
     {
       dest.SetInt ("DEVNUM", devnum);
       dest.SetInt ("COMMAND", command);
       dest.SetInt ("VERBOSELEVEL", verboselevel);
       dest.SetInt ("HEXFORMAT", hexformat);
       dest.SetInt ("BROADCAST", broadcast);
       dest.SetInt ("VERIFY", verify);
       dest.SetInt ("SFP", (int) sfp);
       dest.SetInt ("SLAVE", (int) slave);
       dest.SetInt ("ADDRESS", (int) address);
       dest.SetInt ("VALUE", (int) value);
       dest.SetInt ("REPEAT", (int) repeat);
       dest.SetStr ("FILENAME", filename);
     }

     void ExtractDabcCommand (dabc::Command &src)
     {
       devnum = src.GetInt ("DEVNUM");
       command = (gos_cmd_id) (src.GetInt ("COMMAND"));
       verboselevel = src.GetInt ("VERBOSELEVEL");
       hexformat = src.GetInt ("HEXFORMAT");
       broadcast = src.GetInt ("BROADCAST");
       verify = src.GetInt ("VERIFY");
       sfp = src.GetInt ("SFP");
       slave = src.GetInt ("SLAVE");
       address = src.GetInt ("ADDRESS");
       value = src.GetInt ("VALUE");
       repeat = src.GetInt ("REPEAT");
       strncpy (filename, src.GetStr ("FILENAME").c_str (), GOSIP_MAXTEXT);

     }




   };





}

#endif //GOSIP_COMMAND_PLAINC




#endif //GOSIP_Command
