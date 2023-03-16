#ifndef DOFI_Command
#define DOFI_Command

#include"dabc/Command.h"
#include "dabc/logging.h"
#include <cstdarg>
#include <cstring>

#define RDOC_DEFAULTPORT 54321
#define DOFI_MAXTEXT 128
#define DOFI_CMD_SIZE 256
#define DOFI_CMD_MAX_ARGS 10

namespace dofi {



typedef enum
{
  DOFI_NOP,
  DOFI_RESET,
  DOFI_READ,
  DOFI_WRITE,
  DOFI_SETBIT,
  DOFI_CLEARBIT,
  DOFI_RESET_SCALER,
  DOFI_ENABLE_SCALER,
  DOFI_DISABLE_SCALER,
  DOFI_CONFIGURE
} dof_cmd_id;



   /** \brief DOFI command interface
    *
    * Contains command definitions for execution on raspi@muppet node.
    *
    */

   class Command {

   private:

   protected:

   public:


  dof_cmd_id command; /**< command identifier*/
  char verboselevel; /**< level of debug 0=off*/
  char hexformat; /**< hexoutput(1) or decimal (0)*/
  long address; /**< address on slave*/
  unsigned long long value; /**< value to write, or read back*/
  long repeat; /**< number of words for incremental read*/

  char filename[DOFI_MAXTEXT]; /**< optional name of configuration file*/
 

  /**********************************************************************/


     Command():
      command(dofi::DOFI_NOP),verboselevel(0),hexformat(0),
             address(-1),
             value(0), repeat(1)
             {
             }

     virtual ~Command(){;}


     bool set_command (dof_cmd_id id)
     {
       if (command == dofi::DOFI_NOP)
              {
                command = id;
                return true;
              }
              else
              {
                return false;
                //EOUT (" doficmd ERROR - conflicting command specifiers!\n");
              }
     }

     static char CommandDescription[DOFI_MAXTEXT];




     bool assert_command ()
     {
       int do_exit = 0;
       if (command == dofi::DOFI_NOP)
         do_exit = 1;
//       if (fd_pex < 0)
//         do_exit = 1;
       //~ if ((command != dofi::DOFI_CONFIGURE) && (command != dofi::DOFI_VERIFY) && (command != dofi::DOFI_RESET))
       //~ {
         //~ if (sfp < -1) /*allow broadcast statements -1*/
           //~ do_exit = 1;
         //~ if ((command != dofi::DOFI_INIT) && (slave < -1))
           //~ do_exit = 1;
         //~ if ((command != dofi::DOFI_INIT) && (address < -1))
           //~ do_exit = 1;
       //~ }
       //~ else
       //~ {
         //~ broadcast = 0;    // disable broadcast for configure, verify and reset if it was chosen by mistake
       //~ }

       //~ if ((command == dofi::DOFI_READ || command == dofi::DOFI_SETBIT || command == dofi::DOFI_CLEARBIT)
           //~ && (sfp == -1 || slave == -1))
       //~ {
         //~ broadcast = 1;    // allow implicit broadcast read switched by -1 argument for sfp or slave
       //~ }

       if (do_exit)
       {
         EOUT (" doficmd ERROR - illegal parameters \n");
       return false;
       }
       return true;
     }

     bool assert_arguments (int arglen)
  {
    int do_exit = 0;
    if ((command == dofi::DOFI_READ) && (arglen < 1))
      do_exit = 1;
    if ((command == dofi::DOFI_WRITE) && (arglen < 2))
      do_exit = 1;
    if ((command == dofi::DOFI_SETBIT) && (arglen < 2))
      do_exit = 1;
    if ((command == dofi::DOFI_CLEARBIT) && (arglen < 2))
      do_exit = 1;     
     if ((command == dofi::DOFI_CONFIGURE) && (arglen < 1))
      do_exit = 1; 
    if (do_exit)
    {
      EOUT (" doficmd ERROR - number of parameters not sufficient for command %d %s\n",command, get_description());
      return false;
    }
    return true;
  }



     char* get_description ()
     {
       switch (command)
       {
         case dofi::DOFI_RESET:
           snprintf (CommandDescription, DOFI_MAXTEXT, "Reset dofi SPI connection");
           break;
         case dofi::DOFI_READ:
           snprintf (CommandDescription, DOFI_MAXTEXT, "Read value");
           break;
         case dofi::DOFI_WRITE:
           snprintf (CommandDescription, DOFI_MAXTEXT, "Write value");
           break;
         case dofi::DOFI_RESET_SCALER:
           snprintf (CommandDescription, DOFI_MAXTEXT, "Reset scalers");
           break;
         case dofi::DOFI_ENABLE_SCALER:
           snprintf (CommandDescription, DOFI_MAXTEXT, "Enable scalers");
           break;
          case dofi::DOFI_DISABLE_SCALER:
           snprintf (CommandDescription, DOFI_MAXTEXT, "Disable scalers");
           break;  

         case dofi::DOFI_SETBIT:
           snprintf (CommandDescription, DOFI_MAXTEXT, "Set Bitmask");
           break;
         case dofi::DOFI_CLEARBIT:
           snprintf (CommandDescription, DOFI_MAXTEXT, "Clear Bitmask");
           break;
          case dofi::DOFI_CONFIGURE:
           snprintf (CommandDescription, DOFI_MAXTEXT, "Configure from file");
           break; 
           
         default:
           snprintf (CommandDescription, DOFI_MAXTEXT, "Unknown command");
           break;
       };
       return CommandDescription;
     }

     void dump_command ()
     {
		 //printm (" doficmd dump: \n");
		 hexformat == 1 ? DOUT0 (" Command  :0x%x (%s)", command, get_description ()) :
		 DOUT0 (" Command: %d (%s)", command, get_description ());
		 if ((command == DOFI_CONFIGURE)) // || (command == DOFI_VERIFY))
		 {
			DOUT0 (" \t config file    :%s \n", filename);
		 }
		 else
		 {
			 hexformat == 1 ? DOUT0 ("\t address: 0x%lx", address) : DOUT0 ("\t address: %ld", address);
			 hexformat == 1 ? DOUT0 ("\t value: 0x%llx", value) : DOUT0 ("\t value: %lld", value);
			 hexformat == 1 ? DOUT0 ("\t repeat: 0x%lx", repeat) : DOUT0 ("\t repeat: %ld", repeat);
		 //  DOUT0 ("\t broadcast: %d \n", broadcast);
		 }
     }


     void output ()
     {
       if (verboselevel)
       {
         hexformat ?
             DOUT0 ("Address: 0x%lx  Data: 0x%llx", address, value) :
             DOUT0 ("Address: %ld  Data: %lld ", address, value);
       }
       else
       {
         hexformat ? DOUT0 ("0x%llx", value) : DOUT0 ("%lld", value);
       }
     }


     void BuildDabCommand (dabc::Command &dest)
     {
       dest.SetInt ("COMMAND", command);
       dest.SetInt ("VERBOSELEVEL", verboselevel);
       dest.SetInt ("HEXFORMAT", hexformat);
       dest.SetInt ("ADDRESS", (int) address);
       dest.SetUInt ("VALUE", (unsigned) value);
       dest.SetInt ("REPEAT", (int) repeat);
       dest.SetStr ("FILENAME", filename);
     }

     void ExtractDabcCommand (dabc::Command &src)
     {
       command = (dof_cmd_id) (src.GetInt ("COMMAND"));
       verboselevel = src.GetInt ("VERBOSELEVEL");
       hexformat = src.GetInt ("HEXFORMAT");
       address = src.GetInt ("ADDRESS");
       value = src.GetUInt ("VALUE");
       repeat = src.GetInt ("REPEAT");
       strncpy (filename, src.GetStr ("FILENAME").c_str (), DOFI_MAXTEXT);
     }
   };
}

#endif //DOFI_Command
