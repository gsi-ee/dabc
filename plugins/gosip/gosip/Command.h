#ifndef GOSIP_Command
#define GOSIP_Command

#include"dabc/Command.h"
#include "dabc/logging.h"
#include <stdarg.h>
#include <string.h>

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

#endif //GOSIP_Command
