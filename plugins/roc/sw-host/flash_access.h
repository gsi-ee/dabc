#ifndef FLASH_ACCESS_H
#define FLASH_ACCESS_H
/*
 *
 * first Version by David Rohr using Uart only interface
 * revised by Stefan Mueller-Klieser
 */

#define FA_VERBOSE 1

// Located in: ppc405_0/include/xparameters.h
#include "xparameters.h"

//?
#include "xbasic_types.h"
#include "xutil.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


//====================================================

//constant IDCODE_ADDR      : std_logic_vector(0 to 2) := "000";
#define IDCODE_ADDR 0
//constant CHECK_IDLE_ADDR  : std_logic_vector(0 to 2) := "001";
#define CHECK_IDLE_ADDR 1
//constant RESET_ADDR       : std_logic_vector(0 to 2) := "010";
#define RESET_ADDR 2
//constant GEN_WRITE_ADDR   : std_logic_vector(0 to 2) := "011";
#define GEN_WRITE_ADDR 3   
//constant DATA_READ_ADDR   : std_logic_vector(0 to 2) := "100";
#define DATA_READ_ADDR 4
//constant DATA_WRITE_ADDR  : std_logic_vector(0 to 2) := "101";
#define DATA_WRITE_ADDR 5
//constant ADDRESS_REG_ADDR : std_logic_vector(0 to 2) := "110";
#define ADDRESS_REG_ADDR 6
//constant COMMAND_REG_ADDR : std_logic_vector(0 to 2) := "111";   
#define COMMAND_REG_ADDR 7

#define PCI_COMMAND_WRITE 2
#define PCI_COMMAND_READ 4

/* Write Sequence:
WriteFlash(0xAAA | nChip, 0xAA);
WriteFlash(0x555 | nChip, 0x55);
WriteFlash(0xAAA | nChip, 0xA0);
WriteFlash(Address, Data);*/   

#define FLASH_ACCESS_CMD_RESET 'a'         //Send a to reset, repeat 100 a to force reset even if in Burst Mode
#define FLASH_ACCESS_CMD_IDENT 'b'         //Will Answer Ident String
#define FLASH_ACCESS_CMD_READ 'c'         //Send "c" [Addr] to read from Actel Addr
#define FLASH_ACCESS_CMD_WRITE 'd'         //Send "d" [Addr] [Byte2] [Byte1] [Byte0] to write 24 bit to Addr
#define FLASH_ACCESS_CMD_TEST 'e'         //Send to run test sequence
#define FLASH_ACCESS_CMD_WRITE_BURST 'f'   //Send "f", 4 byte Addr, 4 byte length, data
                                 //Every 90th byte must be control byte 255, initial f is not counted for this
                                 //Length must consider 8 bytes for addr and length and control bytes but not nitial f
#define FLASH_ACCESS_CMD_READ_BURST 'g'      //Send "g", 4 byte Addr, 4 byte length, will require control byte "." every 4 bytes to ensure no fifo overflow
#define FLASH_ACCESS_CMD_WRITE_ADDRESS 'h'   //Write from Address to Flash, this is a direct PPC Address for example in DDR RAM Address Space
                                 //Send "h" 4 byte Flash Addr, 4 byte PPC Addr, 4 byte Length
                                 //Will send '.' for each 1000 bytes written to flash
                                 //Send any char to abort!
#define FLASH_ACCESS_CMD_PING 'p'         //Should answer with pong 'p'
#define FLASH_ACCESS_CMD_SET_DISABLE 'q'    //Set Disable state for virtex_ok signal ('q' is nice for quit btfw :|)
                     

#define UART_STATE_IDLE 0
#define UART_STATE_READ 1
#define UART_STATE_WRITE 2
#define UART_STATE_READ_BURST 3
#define UART_STATE_WRITE_BURST 4
#define UART_STATE_RESET 5
#define UART_STATE_WRITE_ADDRESS 6

#define IDENT "PC to Network to Virtex to Actel to Flash Interface v1.0"

#define WRITE_BURST_CONTROL_INTERVAL 80
#define READ_BURST_CONTROL_INTERVAL 16

#define VIRTEX_READ_TIMEOUT 100000
#define FLASH_WAIT_TIME 100000

struct KibFileHeader
{
   Xuint8 ident[4];
   Xuint32 headerSize;
   Xuint32 binfileSize;
   Xuint8 XORCheckSum;
   Xuint8 bitfileName[65];
   Xuint32 timestamp;
};

int WaitVirtexReady();
void TestSequence();
int flash_access_send(Xuint8,Xuint8*);
void flash_access_showBitfileInformation();
Xuint8 flash_access_readByteFromFlashram(Xuint32 address);
Xuint8 flash_access_checkIfChipErased(Xuint32 nChip);
Xuint8 flash_access_writeByteToFlashram(Xuint32 address, Xuint8 data);
void flash_access_chipErase(Xuint8 nChip);
Xuint8 flash_access_flashBitfile(Xuint8* bitfile, Xuint8 positionNumber);
void  writeToFlashAccessModule(Xuint8 address, Xuint32 value);
void writeDataToActel(Xuint32 address, Xuint32 value);
Xuint32 readFromFlashAccessModule(Xuint8 address);
void flash_access_copyBitfile();
void flash_access_compareBitfiles();
Xuint8 flash_access_calcXor();
Xuint8 flash_access_calcKIBXor(int nChip);

#endif
