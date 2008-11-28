#ifndef PCI_BoardCommands
#define PCI_BoardCommands

#include "dabc/Command.h"


#define DABC_PCI_COMMAND_SET_READ_REGION "SetPCIReadRegion"
#define DABC_PCI_COMMAND_SET_WRITE_REGION "SetPCIWriteRegion"

#define DABC_PCI_COMPAR_PORT     "Port"
#define DABC_PCI_COMPAR_BAR      "BAR"
#define DABC_PCI_COMPAR_ADDRESS  "Address"
#define DABC_PCI_COMPAR_SIZE     "Length"

#define ABB_COMPAR_BUFSIZE    "BufferSize"
#define ABB_COMPAR_QLENGTH    "RecvQueue"
#define ABB_COMPAR_STALONE    "Standalone"
#define ABB_COMPAR_POOL       "PoolName"


namespace pci {


/** this command sets region in pci memory for next data transfer read*/
  class CommandSetPCIReadRegion : public dabc::Command {
      public:
         CommandSetPCIReadRegion(unsigned int bar, unsigned int address, unsigned int length) :
            dabc::Command(DABC_PCI_COMMAND_SET_READ_REGION)
            {
               SetInt(DABC_PCI_COMPAR_BAR, bar);
               SetInt(DABC_PCI_COMPAR_ADDRESS, address);
               SetInt(DABC_PCI_COMPAR_SIZE, length);
            }
   };

/** this command sets region in pci memory for next data transfer write*/
  class CommandSetPCIWriteRegion : public dabc::Command {
      public:
         CommandSetPCIWriteRegion(unsigned int bar, unsigned int address, unsigned int length) :
            dabc::Command(DABC_PCI_COMMAND_SET_WRITE_REGION)
            {
               SetInt(DABC_PCI_COMPAR_BAR, bar);
               SetInt(DABC_PCI_COMPAR_ADDRESS, address);
               SetInt(DABC_PCI_COMPAR_SIZE, length);
            }
   };




}//namespace

#endif

