#include "flash_access.h"

/************************************************
 * private global variables
 ************************************************/

volatile Xuint32* const base_addr = (Xuint32*) XPAR_FLASH_ACCESS_0_BASEADDR; //VOLATILE IMPORTANT FOR CACHING REASONS!!!!!!
volatile Xuint32* const virtex_ok_disable_base_addr = (Xuint32*) XPAR_VIRTEX_OK_DISABLE_0_BASEADDR;

volatile Xint32 VirtexTimeout;
volatile Xint32 nTimeouts;


void printMemory2(Xuint8* target, Xint16 bytes)
{
   signed int b = bytes;
   int i,j;
   j = 0;
   xil_printf("\r\n\r\n");
   for(;b > 0;b -= 8)
   {
      for (i=0;i <8;i++)
      {
         xil_printf("%X ",*(target+i+j));
      }
      xil_printf("\r\n");
      j += 8;
   }
}


void swap_byteorder2(Xuint8* target, Xuint16 bytes)
{
   int i;
   char a[bytes];
   memcpy(a, target, bytes);
   for(i = 0; i < bytes; i++)
   {
      *(target + i) = a[bytes-1-i];
   }
}
   
//Wait till virtex ready signal, if no signal incoming send error through com and set virtextime to 1 to check later
int WaitVirtexReady()
{
   int nWaitCount;

   nWaitCount = 0;
   
   while (base_addr[CHECK_IDLE_ADDR] != 1 && nWaitCount++ < VIRTEX_READ_TIMEOUT);

   if (nWaitCount >= VIRTEX_READ_TIMEOUT)
   {
      xil_printf("----------Timeout while waiting for Virtex Ready Signal...----------\n");
      VirtexTimeout = 1;
      nTimeouts++;
      return(1);
   }
   return(0);
}

//Just for testing purpose, will be called by c program and c program will display output   
void TestSequence()
{
   Xuint32 tmp;
   base_addr[RESET_ADDR] = 0;
   WaitVirtexReady();
   
   xil_printf("Virtex Timeouts: %d\n\r", nTimeouts);
   
   tmp = base_addr[IDCODE_ADDR];
   xil_printf("Virtex Bus ID: %d\n\r", tmp);
}

int flash_access_send(Xuint8 var, Xuint8 *giveback)
{
   xil_printf("flash_access_send: %c\n\r", var);
   
   static int nUartResetCount;
   static int nUartState;
   static unsigned int nUartAddress;
   static unsigned int nUartData;
   static unsigned int nUartBurstSize;
   static unsigned int nUartPPCAddress;
   static unsigned int nUartCounter;
   static unsigned char nUartChecksum;
   static int nUartDataPos;
   static int retVal;
   static int i;
   static int isInitialisized;
   
   if(isInitialisized == 0)
   {
      nUartResetCount = 0;
      nUartState = UART_STATE_IDLE;
      nUartAddress = 0;
      nUartData = 0;
      nUartBurstSize = 0;
      nUartPPCAddress = 0;
      nUartCounter = 0;
      nUartChecksum = 0;
      nUartDataPos = 0;
      retVal = 0;
      i = 0;
      VirtexTimeout = nTimeouts = 0;
      
      isInitialisized = 1;   
   }
   
   
   if (VirtexTimeout)                     //If we timeouted before reset this
   {
      nUartState = UART_STATE_IDLE;         //and go to idle state
      VirtexTimeout = 0;
   }
   if (var == FLASH_ACCESS_CMD_RESET)               //Reset..., when in IDLE no problem
   {                                       //Otherwise this could be user data, so wait for 100 reset command, user data must not consists of 100 commands then
      if (nUartState == UART_STATE_IDLE || ++nUartResetCount >= 100)
      {
         nUartState = UART_STATE_RESET;
      }
   }
   else
   {
      nUartResetCount = 0;
   }
   
   switch(nUartState)                     //We need some kind of statemachine to handle incoming data
   {
      case UART_STATE_IDLE:
         switch (var)
         {
            case FLASH_ACCESS_CMD_IDENT:
               xil_printf(IDENT "\n\r");
               break;
               
            case FLASH_ACCESS_CMD_TEST:
               xil_printf("Starting Test Sequence\n\r");
               TestSequence();
               xil_printf("Finished\n\r");
               break;
               
            case FLASH_ACCESS_CMD_READ:
               nUartState = UART_STATE_READ;
               break;
               
            case FLASH_ACCESS_CMD_WRITE:
               nUartState = UART_STATE_WRITE;
               nUartDataPos = 0;
               nUartData = 0;                     
               break;
               
            case FLASH_ACCESS_CMD_WRITE_BURST:
               nUartState = UART_STATE_WRITE_BURST;
               nUartDataPos = 0;
               nUartAddress = 0;
               nUartBurstSize = 0;
               nUartCounter = 0;
               nUartChecksum = 0;
                  break;
               
            case FLASH_ACCESS_CMD_READ_BURST:
               nUartState = UART_STATE_READ_BURST;
               nUartAddress = 0;
               nUartDataPos = 0;
               nUartBurstSize = 0;
               nUartChecksum = 0;
               break;
               
            case FLASH_ACCESS_CMD_WRITE_ADDRESS:
               nUartAddress = 0;
               nUartDataPos = 0;
               nUartBurstSize = 0;      
               nUartPPCAddress = 0;
               nUartState = UART_STATE_WRITE_ADDRESS;
               break;
               
            case FLASH_ACCESS_CMD_SET_DISABLE:
               virtex_ok_disable_base_addr[0] = 1;            //Should terminate soon anyway :|
               break;
               
            case FLASH_ACCESS_CMD_PING:
               xil_printf("p");
               break;
            default:
               xil_printf("Error: Unexspected Command Code: %d\n\r", var);
               break;
         }
         break;
         
      case UART_STATE_READ:      //Read From Flash/RAM
         nUartAddress = var;
         retVal = base_addr[nUartAddress];
         xil_printf("Read:%d\n\r", retVal);
         nUartState = UART_STATE_IDLE;
         break;
         
      case UART_STATE_WRITE:      //Write to Flash Interface (not RAM)
         if (nUartDataPos++ == 0)
         {
            nUartAddress = var;
         }
         else
         {
            nUartData <<= 8;      //Read single byte
            nUartData += var;
            if (nUartDataPos == 4)   //When last byte read we write it to actel
            {
               base_addr[nUartAddress] = nUartData;
               if (WaitVirtexReady()) break;
               xil_printf("Wrote %d to Address %d\n\r", nUartData, nUartAddress);
               nUartState = UART_STATE_IDLE;
               nUartDataPos = 0;
               nUartData = 0;
            }
         }
         break;
         
      case UART_STATE_WRITE_BURST:   //Write to RAM in Burst Mode
         if (nUartDataPos < 4)
         {
            nUartAddress <<= 8;      //Flash Address
            nUartAddress += var;
         }
         else if (nUartDataPos < 8)
         {
            nUartBurstSize <<= 8;   //Number of bytes to write
            nUartBurstSize += var;
         }
         else
         {
            if (nUartDataPos % WRITE_BURST_CONTROL_INTERVAL == 0)      //We want control bytes, interval must be below 100 because of the reset sequence
            {
               if (var != 255)            //Reset when Control byte wrong
               {
                  xil_printf("Burst transfer failed...\n\rControl Byte missing at %d\n\r", nUartDataPos);
                  nUartState = UART_STATE_RESET;
               }
               else
               {
                  xil_printf(".");               //Otherwise acknowledge the contol byte with .
               }
            }
            else
            {
               int nChipSel = nUartAddress & 0x800000;
                  
               base_addr[ADDRESS_REG_ADDR] = 0xAAA | nChipSel;      //Send 3 commands to flash to write single byte
               if (WaitVirtexReady()) break;
               base_addr[DATA_WRITE_ADDR] = 0xAA;
               if (WaitVirtexReady()) break;
               base_addr[COMMAND_REG_ADDR] = PCI_COMMAND_WRITE;
               if (WaitVirtexReady()) break;
                  
               base_addr[ADDRESS_REG_ADDR] = 0x555 | nChipSel;
               if (WaitVirtexReady()) break;
               base_addr[DATA_WRITE_ADDR] = 0x55;
               if (WaitVirtexReady()) break;
               base_addr[COMMAND_REG_ADDR] = PCI_COMMAND_WRITE;
               if (WaitVirtexReady()) break;

               base_addr[ADDRESS_REG_ADDR] = 0xAAA | nChipSel;
               if (WaitVirtexReady()) break;
               base_addr[DATA_WRITE_ADDR] = 0xA0;
               if (WaitVirtexReady()) break;
               base_addr[COMMAND_REG_ADDR] = PCI_COMMAND_WRITE;
               if (WaitVirtexReady()) break;
                  
               base_addr[ADDRESS_REG_ADDR] = nUartAddress++;         //Write byte to address and increase address by 1
               if (WaitVirtexReady()) break;
               base_addr[DATA_WRITE_ADDR] = var & 0xFF;
               if (WaitVirtexReady()) break;
               base_addr[COMMAND_REG_ADDR] = PCI_COMMAND_WRITE;
               if (WaitVirtexReady()) break;
               nUartCounter++;
               nUartChecksum ^= var;
               
               i = 0;                                             //Check if byte is actually written and give error if not
               do
               {
                  base_addr[COMMAND_REG_ADDR] = PCI_COMMAND_READ;
                  if (WaitVirtexReady()) break;
                  base_addr[DATA_READ_ADDR] = 1;
                  if (WaitVirtexReady()) break;
                  retVal = base_addr[DATA_READ_ADDR];
               } while ((retVal & 0xFF) != (var & 0xFF) && i++ < FLASH_WAIT_TIME);
               
               if (VirtexTimeout)
               {
                  break;
               }

               if (i >= FLASH_WAIT_TIME)
               {
                  xil_printf("Write failed %d at 0x%06X (Value is %d / 0x%08X)\n\r", var, nUartAddress - 1, retVal & 0xFF, retVal);
                  nUartState = UART_STATE_RESET;
               }
            }
         }
         nUartDataPos++;
         if (nUartDataPos > 8 && nUartDataPos >= nUartBurstSize)      //Here we are finished, give some info back to pc, pc shell check checksum
         {
            xil_printf("Burst Write completed successfully (%d bytes, Checksum %d)\n\r", nUartCounter, (unsigned int) nUartChecksum);
            nUartAddress = nUartBurstSize = nUartDataPos = 0;
            nUartState = UART_STATE_IDLE;
         }
         break;
         
      case UART_STATE_READ_BURST:                                    //Same as BurstWrite but for reading...
         if (nUartDataPos < 4)
         {
            nUartAddress <<= 8;
            nUartAddress += var;
            nUartDataPos++;
         }
         else if (nUartDataPos < 8)   
         {
            nUartBurstSize <<= 8;
            nUartBurstSize += var;
            nUartDataPos++;
         }
         if (nUartDataPos >= 8)
         {
            if (nUartDataPos != 8)
            {
               if (var != '.')
               {
                  xil_printf("Invalid Control Byte %d at Pos %d...\n\rAborting Burst read\n\r", var, nUartDataPos);
                  nUartState = UART_STATE_RESET;
                  break;
               }
            }
         
            int nUartBurstPos = 0;
            while (nUartBurstPos++ < READ_BURST_CONTROL_INTERVAL && nUartDataPos < nUartBurstSize + 8)
            {
               nUartDataPos++;
               
               base_addr[ADDRESS_REG_ADDR] = nUartAddress++;
               if (WaitVirtexReady()) break;
               base_addr[COMMAND_REG_ADDR] = PCI_COMMAND_READ;
               if (WaitVirtexReady()) break;
               base_addr[DATA_READ_ADDR] = 0;
               if (WaitVirtexReady()) break;
            
               char buffer = base_addr[DATA_READ_ADDR] & 0xFF;
               *giveback = buffer;
               nUartChecksum ^= buffer;
            }
            
            if (nUartDataPos >= nUartBurstSize + 8)
            {
               xil_printf("Burst Read completed successfully (%d bytes, Checksum %d)\n\r", nUartDataPos - 8, (unsigned int) nUartChecksum);
               nUartAddress = nUartBurstSize = nUartDataPos = 0;
               nUartState = UART_STATE_IDLE;
            }
         }
         
         break;
         
      case UART_STATE_WRITE_ADDRESS:         //Same as burst write but we take incoming data directly from address that is
         if (nUartDataPos < 4)               //passed from pc
         {
            nUartAddress <<= 8;
            nUartAddress += var;
            nUartDataPos++;
         }
         else if (nUartDataPos < 8)   
         {
            nUartPPCAddress <<= 8;
            nUartPPCAddress += var;
            nUartDataPos++;
         }            
         else if (nUartDataPos < 12)   
         {
            nUartBurstSize <<= 8;
            nUartBurstSize += var;
            nUartDataPos++;
         }
         
         if (nUartDataPos == 12)
         {
            xil_printf("PPC Starts write %d bytes from Address %X to Flash Address %X...", nUartBurstSize, nUartPPCAddress, nUartAddress);
         
            nUartDataPos = 0;
            while (nUartDataPos < nUartBurstSize)
            {
               int nChipSel = (nUartAddress + nUartDataPos) & 0x800000;
               
               var = *((Xuint8*) (nUartPPCAddress + nUartDataPos));
                  
               base_addr[ADDRESS_REG_ADDR] = 0xAAA | nChipSel;
               if (WaitVirtexReady()) break;
               base_addr[DATA_WRITE_ADDR] = 0xAA;
               if (WaitVirtexReady()) break;
               base_addr[COMMAND_REG_ADDR] = PCI_COMMAND_WRITE;
               if (WaitVirtexReady()) break;
                  
               base_addr[ADDRESS_REG_ADDR] = 0x555 | nChipSel;
               if (WaitVirtexReady()) break;
               base_addr[DATA_WRITE_ADDR] = 0x55;
               if (WaitVirtexReady()) break;
               base_addr[COMMAND_REG_ADDR] = PCI_COMMAND_WRITE;
               if (WaitVirtexReady()) break;

               base_addr[ADDRESS_REG_ADDR] = 0xAAA | nChipSel;
               if (WaitVirtexReady()) break;
               base_addr[DATA_WRITE_ADDR] = 0xA0;
               if (WaitVirtexReady()) break;
               base_addr[COMMAND_REG_ADDR] = PCI_COMMAND_WRITE;
               if (WaitVirtexReady()) break;
                  
               base_addr[ADDRESS_REG_ADDR] = nUartAddress + nUartDataPos++;
               if (WaitVirtexReady()) break;
               base_addr[DATA_WRITE_ADDR] = var;
               if (WaitVirtexReady()) break;
               base_addr[COMMAND_REG_ADDR] = PCI_COMMAND_WRITE;
               if (WaitVirtexReady()) break;
               
               i = 0;
               do
               {
                  base_addr[COMMAND_REG_ADDR] = PCI_COMMAND_READ;
                  if (WaitVirtexReady()) break;
                  base_addr[DATA_READ_ADDR] = 1;
                  if (WaitVirtexReady()) break;
               } while ((base_addr[DATA_READ_ADDR] & 0xFF) != (var & 0xFF) && i++ < FLASH_WAIT_TIME);
               
               if (VirtexTimeout) break;

               if (i >= FLASH_WAIT_TIME)
               {
                  xil_printf("Write failed %d at %X\n\r", var, nUartAddress + nUartDataPos - 1);
                  nUartState = UART_STATE_RESET;
                  break;
               }
               
               if (nUartDataPos % 1000 == 0)
               {
                  xil_printf(".");
               }
               /*
               if (XUartLite_Recv(&uart, &var, 1))
               {
                  print("PPC Writing to Flash from Address aborted...\n\r");
                  nUartState = UART_STATE_RESET;
                  break;
               }*/
            }
            
            if (nUartDataPos == nUartBurstSize)
            {
               xil_printf("Address Write to Flash completed\n\r");
            }
            else
            {
               if (VirtexTimeout)
               {
                  xil_printf("Virtex timeouted...\n\r");
               }
               xil_printf("Address Write to Flash failed\n\r");
            }
            nUartState = UART_STATE_IDLE;
            nUartAddress = nUartBurstSize = nUartDataPos = 0;
         }
         
         break;
         
      default:
         if (nUartState != UART_STATE_RESET)   xil_printf("Invalid state, reset required...\n\r");
         nUartState = UART_STATE_RESET;
         break;
   }
   
   if (nUartState == UART_STATE_RESET)
   {
         xil_printf("Resetting\n\r");
         nUartState = UART_STATE_IDLE;
         nUartAddress = 0;
         nUartPPCAddress = 0;
         nUartData = 0;
         nUartDataPos = 0;
         nUartCounter = 0;
         nUartResetCount = 0;
         base_addr[RESET_ADDR] = 0;
         WaitVirtexReady();
   }


   return 0;
}

void flash_access_showBitfileInformation()
{
   Xuint32 nChip = 0;
   struct KibFileHeader* pHeader;
   pHeader = (struct KibFileHeader*) malloc(sizeof(struct KibFileHeader) + 1);
   if (pHeader == NULL)
   {
      xil_printf("Memory Allocation Error\r\n");
   }
   
   Xuint32 currentReadAddress = 0;
   int i;
   for(; nChip < 2; nChip++)
   {
      xil_printf("\r\nCollecting KIB header information from chip %d\r\n", nChip); 
   
      xil_printf("Reading File Header\r\n");
      
      currentReadAddress = 0x800000 * nChip;
      for(i = 0; i < sizeof(struct KibFileHeader); i++)
      {
         *((Xuint8*)pHeader + i) = flash_access_readByteFromFlashram(currentReadAddress + i);
      }
   
      if (strncmp(pHeader->ident, "XBF", 4) != 0)
      {
         xil_printf("No File Header found in Chip %d...\r\n", nChip);
      }
      
      if (pHeader->headerSize != 512)
      {
         xil_printf("Invalid Header Size\r\n");
      }
   
   
      xil_printf("\r\nBinfile informations in chip %d\r\n", nChip);
      xil_printf("Old Bitfilename: %s\r\n", pHeader->bitfileName);
      xil_printf("File uploaded at: ");
      xil_printf("Timestamp: %d\r\n", pHeader->timestamp);
//      PrintUnixTime(pHeader->timestamp);
      xil_printf("\r\n");
      xil_printf("Binfile size: %d bytes\r\n", pHeader->binfileSize);
      xil_printf("XOR Checksum: %d\r\n", pHeader->XORCheckSum);
   }
   free(pHeader);
}

Xuint8 flash_access_readByteFromFlashram(Xuint32 address)
{
   Xuint32 retVal = 0;
   writeToFlashAccessModule(ADDRESS_REG_ADDR, address);
   writeToFlashAccessModule(COMMAND_REG_ADDR, PCI_COMMAND_READ);
   writeToFlashAccessModule(DATA_READ_ADDR, 0);
   retVal = readFromFlashAccessModule(DATA_READ_ADDR);
   //xil_printf("Read from Flash at: %X Value: %d\r\n", address, retVal);
   return retVal & 0xff;
}
   
//gives back an error code
Xuint8 flash_access_writeByteToFlashram(Xuint32 address, Xuint8 data)
{      
   int retVal=0;
   int nChipSel = address & 0x800000;
   
   while(1)
   {
      base_addr[ADDRESS_REG_ADDR] = 0xAAA | nChipSel;      //Send 3 commands to flash to write single byte
      if (WaitVirtexReady()) continue;
      base_addr[DATA_WRITE_ADDR] = 0xAA;
      if (WaitVirtexReady()) continue;
      base_addr[COMMAND_REG_ADDR] = PCI_COMMAND_WRITE;
      if (WaitVirtexReady()) continue;
         
      base_addr[ADDRESS_REG_ADDR] = 0x555 | nChipSel;
      if (WaitVirtexReady()) continue;
      base_addr[DATA_WRITE_ADDR] = 0x55;
      if (WaitVirtexReady()) continue;
      base_addr[COMMAND_REG_ADDR] = PCI_COMMAND_WRITE;
      if (WaitVirtexReady()) continue;
   
      base_addr[ADDRESS_REG_ADDR] = 0xAAA | nChipSel;
      if (WaitVirtexReady()) continue;
      base_addr[DATA_WRITE_ADDR] = 0xA0;
      if (WaitVirtexReady()) continue;
      base_addr[COMMAND_REG_ADDR] = PCI_COMMAND_WRITE;
      if (WaitVirtexReady()) continue;
         
      base_addr[ADDRESS_REG_ADDR] = address;         //Write byte to address
      if (WaitVirtexReady()) continue;
      base_addr[DATA_WRITE_ADDR] = data & 0xFF;
      if (WaitVirtexReady()) continue;
      base_addr[COMMAND_REG_ADDR] = PCI_COMMAND_WRITE;
      if (WaitVirtexReady()) continue;
      
      break;
   }
   
   //nUartChecksum ^= data;
   
   int i = 0;                                    //Check if byte is actually written and give error if not
   do
   {
      base_addr[COMMAND_REG_ADDR] = PCI_COMMAND_READ;
      if (WaitVirtexReady()) break;
      base_addr[DATA_READ_ADDR] = 1;
      if (WaitVirtexReady()) break;
      retVal = base_addr[DATA_READ_ADDR];
   } while ((retVal & 0xFF) != (data & 0xFF) && i++ < FLASH_WAIT_TIME);
   
   if (VirtexTimeout)
   {
      return 0;
   }

   if (i >= FLASH_WAIT_TIME)
   {
      xil_printf("Write failed %d at 0x%06X (Value is %d / 0x%08X)\n\r", data, address, retVal & 0xFF, retVal);
      return 0;
   }

   return 1;
}         


void writeToFlashAccessModule(Xuint8 address, Xuint32 value)
{
   base_addr[address] = value;

   if (!WaitVirtexReady());
      //xil_printf("Wrote %d to Address %d\r\n", value, address);
}
void writeDataToActel(Xuint32 address, Xuint32 data)
{
   writeToFlashAccessModule(ADDRESS_REG_ADDR, address);
   writeToFlashAccessModule(DATA_WRITE_ADDR, data);
   writeToFlashAccessModule(COMMAND_REG_ADDR, PCI_COMMAND_WRITE);
}

Xuint32 readFromFlashAccessModule(Xuint8 address)
{
   Xuint32 retVal;
   retVal = base_addr[address];
//   xil_printf("Read:%d\n\r ... printmemory2()\r\n", retVal);
//   printMemory2((Xuint8*)&retVal,4);
   return retVal;
}

void flash_access_chipErase(Xuint8 nChip)
{
   //set markers 0 used for checking if flash is erased
   flash_access_writeByteToFlashram(nChip * 0x800000         , 0);
   flash_access_writeByteToFlashram(nChip * 0x800000 + 0x400000, 0);
   flash_access_writeByteToFlashram(nChip * 0x800000 + 0x7FFFFF, 0);
      
   writeDataToActel(0xAAA | (nChip * 0x800000), 0xAA);
   writeDataToActel(0x555 | (nChip * 0x800000), 0x55);
   writeDataToActel(0xAAA | (nChip * 0x800000), 0x80);
   writeDataToActel(0xAAA | (nChip * 0x800000), 0xAA);
   writeDataToActel(0x555 | (nChip * 0x800000), 0x55);
   writeDataToActel(0xAAA | (nChip * 0x800000), 0x10);
   
   //wait for erase
   xil_printf("Waiting for chip erase ...");
   int i = 0;
   while(flash_access_checkIfChipErased(nChip) == 0)
   {
      xil_printf(".");
      for(i = 0; i < 100000000; i++){}
         
   }
   xil_printf("\r\nChip erase completed!\r\n");
}

Xuint8 flash_access_checkIfChipErased(Xuint32 nChip)
{
   if( flash_access_readByteFromFlashram(nChip * 0x800000)          == 255         &&
      flash_access_readByteFromFlashram(nChip * 0x800000 + 0x400000)   == 255         &&
      flash_access_readByteFromFlashram(nChip * 0x800000 + 0x7FFFFF)   == 255         )
   {
      return 1;
   }
   return 0;
}

Xuint8 flash_access_flashBitfile(Xuint8* bitfile, Xuint8 positionNumber)
{
   Xuint32 nChip = positionNumber;//only two right now
   struct KibFileHeader* pHeader = (struct KibFileHeader*) bitfile;
   Xuint32 writeStartAddress = nChip * 0x800000;
   unsigned long long int j;
   int i;

   xil_printf("\r\nKIB header informations in DDR Ram going to chip %d\r\n", nChip);
   xil_printf("Old Bitfilename: %s\r\n", pHeader->bitfileName);
   xil_printf("File uploaded at: ");
   xil_printf("Timestamp: %d\r\n", pHeader->timestamp);

   xil_printf("KIB header size: %d bytes\r\n", pHeader->headerSize);
   xil_printf("Binfile size: %d bytes\r\n", pHeader->binfileSize);
   xil_printf("XOR Checksum: %d\r\n", pHeader->XORCheckSum);
   
   
   xil_printf("Erase Chip %d\r\n", positionNumber);

   flash_access_chipErase(nChip);
   while(flash_access_checkIfChipErased(nChip) == 0)
   {
      for(j = 0; j < 25000000; j++){}
      xil_printf(".");
   }
   xil_printf("Chip %d erased.\r\n", positionNumber);

   xil_printf("Writing KIB file from DDR to Chip at position %d\r\n", positionNumber);    

   Xuint32 writelen = pHeader->binfileSize;
   Xuint32 t = pHeader->headerSize;
   writelen += t;

   xil_printf("Writelen is: %d\r\n", writelen);
      
   for(i = 0; i < writelen; i++)
   {
      flash_access_writeByteToFlashram(writeStartAddress + i, *(bitfile + i));
   }   
   
   /* this will be checked seperatly
   xil_printf("Bitfile written ... doing readback ...\r\n");
   int wrongBytes = 0;
   Xuint8 xor = 0;
   Xuint8 byte;
   for(i = 0; i < writelen; i++)
   {
      byte = flash_access_readByteFromFlashram(writeStartAddress + i);
      if(i >= pHeader->headerSize)//XOR goes over binfile only
         xor ^= byte;
      if(*(bitfile + i) != byte)
         wrongBytes++;
   }   
   
   if(wrongBytes != 0)
   {
      xil_printf("Bitfile write failed %d written bytes where wrong.\r\n", wrongBytes);
      return 0;
   }
   else
   {
      xil_printf("Bitfile successfully written!!\r\n");
      xil_printf("Readback XOR checksum is %d\r\n", xor);
      xil_printf("XOR checksum cumputed at pc loading the bitfile was %d\r\n", pHeader->XORCheckSum);
   }
   */
   return 1;
}

void flash_access_copyBitfile2to1()
{
   int i, j;
   xil_printf("Erase Chip %d\r\n", 0);

   flash_access_chipErase(0);
   while(flash_access_checkIfChipErased(0) == 0)
   {
      for(j = 0; j < 25000000; j++){}
      xil_printf(".");
   }
   xil_printf("Chip %d erased.Copy bitfile ...\r\n", 0);
   
   Xuint8 byte;
   for(i = 0; i < 1000000; i++)
   {
      byte = flash_access_readByteFromFlashram(0x800000 + (512 + i));
      flash_access_writeByteToFlashram(512 + i, byte);
   }   
   
   xil_printf("Copy bitfile header...\r\n");
   
   byte = flash_access_readByteFromFlashram(0x800000 + 7);
   flash_access_writeByteToFlashram(4, byte);
   byte = flash_access_readByteFromFlashram(0x800000 + 6);
   flash_access_writeByteToFlashram(5, byte);
   byte = flash_access_readByteFromFlashram(0x800000 + 5);
   flash_access_writeByteToFlashram(6, byte);
   byte = flash_access_readByteFromFlashram(0x800000 + 4);
   flash_access_writeByteToFlashram(7, byte);
   

   byte = flash_access_readByteFromFlashram(0x800000 + 11);
   flash_access_writeByteToFlashram(8, byte);
   byte = flash_access_readByteFromFlashram(0x800000 + 10);
   flash_access_writeByteToFlashram(9, byte);
   byte = flash_access_readByteFromFlashram(0x800000 + 9);
   flash_access_writeByteToFlashram(10, byte);
   byte = flash_access_readByteFromFlashram(0x800000 + 8);
   flash_access_writeByteToFlashram(11, byte);
   
   byte = flash_access_readByteFromFlashram(0x800000);
   flash_access_writeByteToFlashram(0, byte);
   byte = flash_access_readByteFromFlashram(0x800000 + 1);
   flash_access_writeByteToFlashram(1, byte);
   byte = flash_access_readByteFromFlashram(0x800000 + 2);
   flash_access_writeByteToFlashram(2, byte);
   byte = flash_access_readByteFromFlashram(0x800000 + 3);
   flash_access_writeByteToFlashram(3, byte);
   
   
   xil_printf("Finished!!\r\n");
}

void flash_access_copyBitfile()
{
   int i, j;
   xil_printf("Erase Chip %d\r\n", 1);

   flash_access_chipErase(1);
   while(flash_access_checkIfChipErased(1) == 0)
   {
      for(j = 0; j < 25000000; j++){}
      xil_printf(".");
   }
   xil_printf("Chip %d erased.Copy bitfile ...\r\n", 1);
   
   Xuint8 byte;
   for(i = 0; i < 1000000; i++)
   {
      byte = flash_access_readByteFromFlashram(512 + i);
      flash_access_writeByteToFlashram(0x800000 + (512 + i), byte);
   }   
   
   xil_printf("Copy bitfile header...\r\n");
      
   byte = flash_access_readByteFromFlashram(0);
   flash_access_writeByteToFlashram(0x800000, byte);
   byte = flash_access_readByteFromFlashram(1);
   flash_access_writeByteToFlashram(0x800000 + 1, byte);
   byte = flash_access_readByteFromFlashram(2);
   flash_access_writeByteToFlashram(0x800000 + 2, byte);
   byte = flash_access_readByteFromFlashram(3);
   flash_access_writeByteToFlashram(0x800000 + 3, byte);
   
   
   xil_printf("Finished!!\r\n");
}
void flash_access_compareBitfiles()
{
   int i;
   xil_printf("Comparing bitfiles from both chips...\r\n");
   
   Xuint8 byte1,byte2;
   for(i = 0; i < 1000000; i++)
   {
      byte1 = flash_access_readByteFromFlashram(512 + i);
      byte2 = flash_access_readByteFromFlashram(0x800000 + (512 + i));
      if(byte1 != byte2)
      {
         xil_printf("Bitfiles differ at position %X + 512. Value chip 0 is %d [dec].Value chip 1 is %d [dec]. \r\n", i,byte1,byte2);
      }
   }   
   
   
   xil_printf("Finished!!\r\n");
}

Xuint8 flash_access_calcXor()
{
   Xuint32 nChip = 0;
   struct KibFileHeader* pHeader;
   pHeader = (struct KibFileHeader*) malloc(sizeof(struct KibFileHeader) + 1);
   if (pHeader == NULL)
   {
      xil_printf("Memory Allocation Error\r\n");
   }
   
   Xuint32 currentReadAddress = 0;
   int i;
   for(; nChip < 1; nChip++)
   {
      currentReadAddress = 0x800000 * nChip;
      for(i = 0; i < sizeof(struct KibFileHeader); i++)
      {
         *((Xuint8*)pHeader + i) = flash_access_readByteFromFlashram(currentReadAddress + i);
      }
   
      if (strncmp(pHeader->ident, "XBF", 4) != 0)
      {
         xil_printf("No File Header found in Chip %d...\r\n", nChip);
      }
      
      if (pHeader->headerSize != 512)
      {
         xil_printf("Invalid Header Size\r\n");
      }
   }
   
   xil_printf("Doing readout ...\r\n");
   Xuint8 xor = 0;
   Xuint8 byte;
   int bytesChecked = 0;
   for(i = 0; i < pHeader->binfileSize; i++)
   {
      byte = flash_access_readByteFromFlashram(pHeader->headerSize + i);
      xor ^= byte;
      bytesChecked++;
      if(i < 20)
         xil_printf("%X ", byte);
   }   
   xil_printf("\r\nCheck started at %d\r\n", pHeader->headerSize);
   xil_printf("Calculated XOR checksum is %d\r\n", xor);
   xil_printf("Bytes checked %d\r\n", bytesChecked);
   
   free(pHeader);
   return xor;
}

Xuint8 flash_access_calcKIBXor(int nChip)
{
   int i;
   struct KibFileHeader* pHeader;
   pHeader = (struct KibFileHeader*) malloc(sizeof(struct KibFileHeader) + 1);
   
   if (pHeader == NULL)
   {
      xil_printf("Memory Allocation Error\r\n");
   }
   
   Xuint32 currentReadAddress = 0;
   currentReadAddress = 0x800000 * nChip;
   
   for(i = 0; i < sizeof(struct KibFileHeader); i++)
   {
      *(((Xuint8*)pHeader) + i) = flash_access_readByteFromFlashram(currentReadAddress + i);
   }

   if (strncmp(pHeader->ident, "XBF", 4) != 0)
   {
      xil_printf("No File Header found in Chip %d...\r\n", nChip);
   }
   
   if (pHeader->headerSize != 512)
   {
      xil_printf("Invalid Header Size\r\n");
   }
   
   xil_printf("Calculating XOR sum in Chip %d ...\r\n", nChip);
   Xuint8 xor = 0;
   Xuint8 byte;
   int bytesChecked = 0;
   
   for(i = 0; i < (pHeader->binfileSize + pHeader->headerSize); i++)
   {
      byte = flash_access_readByteFromFlashram(currentReadAddress + i);
      xor ^= byte;
      bytesChecked++;
   }   
   xil_printf("Calculated XOR checksum is %d\r\n", xor);
   xil_printf("Bytes checked %d\r\n", bytesChecked);
   
   free(pHeader);
   return xor;
}
