#ifndef SYSCORESHELL_H
#define SYSCORESHELL_H
/****************************************************
 * 
 * SysCoreShell
 * 
 * implements a credit protokoll for data transmission
 * based upon udp transmission for use on SysCore
 * costum Xilinx Virtex4FX board made at
 * KIP University Heidelberg
 * 
 * 
 * 05/08/2008
 * version 1.6.6
 * Norbert Abel
 * abel@kip.uni-heidelberg.de
 * Stefan Mueller-Klieser
 * stefanmk@kip.uni-heidelberg.de
 * 
 * implements
 * - ethernet -> arp
 *          used for local address resolution
 * - ethernet -> ip -> udp
 *          used for data and control transfer
 * 
 *    
 * Changelog:
 * since Version 1.6.7.4 (by S.Linev)
 *  - statistic block can be automatically send when 
 *     poke(ROC_CONSOLE_OUTPUT, 2) was done. This enables debug 
 *     output over UDP port to master controller. Now with debug output 
 *     statistic will be delivered to master terminal
 *  - statistic block now contains how many time (in relative unit) mainLoop
 *    spends in data taking, ethernet packets dispatching and data sending
 *  - maximal optimisation of readout loop - only doing readout (send blocked)
 *    one achieves 36.5 MB/s by reading OCM registers and 
 *    12.5 MB/s by reading BURST registers
 *
 * since Version 1.6.7.3 (by S.Linev)
 *  - flush timer - either flush data from partially filled buffer or
 *    resend last buffer to PC
 *  - small bug-fix in mainLoop - when there was no place in net fifo,
 *    mainLoop was terminated and reentered again (results in send 
 *    datarate reduction). Now up to 12.3 MB/s outgoing rate is measured
 *  - statistic block with several datarates, measured together 
 *
 * since Version 1.6.7.2 (by S.Linev)
 *  - solved (hopefully) problem with half/full diplex,
 *     function Temac_Reset() seems to be correctly resets TEMAC driver
 *  - optionally (via config file entry TEMAC_PHYREG0) one can enable 
 *    TEMAC register initialisation, used in previos tapdev_init() version 
 *  - cleanup peek/poke code, poke command can now transoprt 
 *    portion of raw data (for instance, part of bit file or file name)
 *  - save_config and load_config can store files with different names, 
 *    files should already exists on the SD card
 *  - upload and overwrite any file on SD-card; 
 *     used for software update, one also can overwrite config files 
 *  - fully configurable port numbers for ROC and host PC 
 *      (to exclude in future potential firewall problems)
 * 
 * since Version 1.6.7.1 (by S.Linev)
 *  - complete redesign of mainLoop
 *  - data sending do not block data taking and vise versa
 *  - as fast as possible goes back to data tacking
 *  - more than 100MB can be used for storage of messages before they send to PC
 *  - redesign of protocol - absolute window instead of relative credit counter
 *  - optimised write to Temac FIFO - operation always 4 bytes alligned
 *  - separate consoleMainLoop for console output, move most of cmd_xxxx methods to sc_console.c file
 *  - safe and fast method to upload bitfile (extended peek/poke logic)
 *  - sc_printf can display data on console, UART or on PC terminal
 *
 * since Version 1.6.5.0
 *  - redo of ethernet phy startup code, there was no proper phy init code
 *    in the xilinx device driver for the intel lxt971a, it is done via MII
 *    commands
 * 
 * since Version 1.6.0.0
 *  - Hexadecimal readout of MAC address from SD-Card
 *  - add define ROC_CONSOLE_OUTPUT to be able to mute all rs232 output
 *  - you can now PING the ROC
 *  - Arp cache is done in a different way. ROC uses the Info found in the
 *    ROC_SET_TARGET_IP packet, which will be send during addBoard()
 *
 * since Version 1.0.0.0
 *    - rewrote some of the retransmit code.
 *  - credits are set to max when DAQ start
 * 
 *****************************************************/
 
#define SYSCORE_VERSION 0x01060704

//debuglevel 0 silences the output to nearly zero, 3 makes it loud
#define DEBUGLEVEL 0

#include "defines.h"


#include <ctype.h> //compiliert nicht mit eldk!!!
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "xbasic_types.h"
#include "xio.h"
#include "xexception_l.h"
#include "xutil.h"
#include "xtime_l.h"
#include "xcache_l.h"
#include "xparameters.h"
#include "xuartlite_l.h"
#include <xuartlite.h>
#include "ddr_header.h"

//console
//#include "tty.h"

//network
#include "tapdev.h"
#include "ether.h"
#include "ip.h"
#include "arp.h"
#include "udp.h"
#include "crc32.h"

//utils
#include "sd_reader.h"
#include "flash_access.h"


/*************************************
 * Which address ranges are cacheable? 
 ************************************/
 
#define CACHEABLE_AREA 0x80000001 

/*********************
 * N-XITER Control and
 * OCM Data Prefetcher
 ********************/
 
#define NX_DATA (XPAR_NX_DATA_0_BASEADDR)
#define NX_FIFO_EMPTY (XPAR_NX_DATA_0_BASEADDR + 0x04)
#define NX_FIFO_FULL (XPAR_NX_DATA_0_BASEADDR + 0x08)
#define NX_INIT (XPAR_NX_DATA_0_BASEADDR + 0x10)
#define NX_RESET (XPAR_NX_DATA_0_BASEADDR + 0x14)
#define FIFO_RESET (XPAR_NX_DATA_0_BASEADDR + 0x18)

#define LT_LOW (XPAR_NX_DATA_0_BASEADDR + 0x20)
#define LT_HIGH (XPAR_NX_DATA_0_BASEADDR + 0x24)
#define NX1_DELAY (XPAR_NX_DATA_0_BASEADDR + 0x60)
#define NX2_DELAY (XPAR_NX_DATA_0_BASEADDR + 0x64)
#define NX3_DELAY (XPAR_NX_DATA_0_BASEADDR + 0x68)
#define NX4_DELAY (XPAR_NX_DATA_0_BASEADDR + 0x6C)
#define LTS_DELAY (XPAR_NX_DATA_0_BASEADDR + 0x2C)

#define NX_MISS (XPAR_NX_DATA_0_BASEADDR + 0x30)
#define NX_MISS_RESET (XPAR_NX_DATA_0_BASEADDR + 0x34)

#define I2C1_DATA (XPAR_I2C_0_BASEADDR)
#define I2C1_RESET (XPAR_I2C_0_BASEADDR+4)
#define I2C1_REGRESET (XPAR_I2C_0_BASEADDR+8)
#define I2C1_SLAVEADDR (XPAR_I2C_0_BASEADDR+12)
#define I2C1_REGISTER (XPAR_I2C_0_BASEADDR+16)
#define I2C1_ERROR (XPAR_I2C_0_BASEADDR+0x20)

#define I2C2_DATA (XPAR_I2C_1_BASEADDR)
#define I2C2_RESET (XPAR_I2C_1_BASEADDR+4)
#define I2C2_REGRESET (XPAR_I2C_1_BASEADDR+8)
#define I2C2_SLAVEADDR (XPAR_I2C_1_BASEADDR+12)
#define I2C2_REGISTER (XPAR_I2C_1_BASEADDR+16)
#define I2C2_ERROR (XPAR_I2C_1_BASEADDR+0x20)

#define HW_ROC_NUMBER (XPAR_NX_DATA_0_BASEADDR + 0x40)

#define TP_RESET_DELAY (XPAR_NX_DATA_0_BASEADDR + 0x50)
#define TP_TP_DELAY (XPAR_NX_DATA_0_BASEADDR + 0x54)
#define TP_COUNT (XPAR_NX_DATA_0_BASEADDR + 0x58)
#define TP_START (XPAR_NX_DATA_0_BASEADDR + 0x5C)

#define ADC_DATA (XPAR_NX_DATA_0_BASEADDR + 0x100)
#define ADC_REG (XPAR_NX_DATA_0_BASEADDR + 0x100)
#define SR_INIT (XPAR_NX_DATA_0_BASEADDR + 0x104)
#define BUFG_SELECT (XPAR_NX_DATA_0_BASEADDR + 0x108)
#define ADC_ADDR (XPAR_NX_DATA_0_BASEADDR + 0x110)
#define ADC_READ_EN (XPAR_NX_DATA_0_BASEADDR + 0x114)
#define ADC_ANSWER (XPAR_NX_DATA_0_BASEADDR + 0x114)
#define ADC_REG2 (XPAR_NX_DATA_0_BASEADDR + 0x120)
#define SR_INIT2 (XPAR_NX_DATA_0_BASEADDR + 0x124)
#define BUFG_SELECT2 (XPAR_NX_DATA_0_BASEADDR + 0x128)
#define ADC_ADDR2 (XPAR_NX_DATA_0_BASEADDR + 0x130)
#define ADC_READ_EN2 (XPAR_NX_DATA_0_BASEADDR + 0x134)
#define ADC_ANSWER2 (XPAR_NX_DATA_0_BASEADDR + 0x134)

#define ADC_LATENCY1 (XPAR_NX_DATA_0_BASEADDR + 0x140)
#define ADC_LATENCY2 (XPAR_NX_DATA_0_BASEADDR + 0x144)
#define ADC_LATENCY3 (XPAR_NX_DATA_0_BASEADDR + 0x148)
#define ADC_LATENCY4 (XPAR_NX_DATA_0_BASEADDR + 0x14C)

#define AUX_DATA_LOW (XPAR_NX_DATA_0_BASEADDR + 0x150)
#define AUX_DATA_HIGH (XPAR_NX_DATA_0_BASEADDR + 0x154)

#define BURST1 (XPAR_NX_DATA_0_BASEADDR + 0x200)
#define BURST2 (XPAR_NX_DATA_0_BASEADDR + 0x204)
#define BURST3 (XPAR_NX_DATA_0_BASEADDR + 0x208)

#define DEBUG_NX (XPAR_NX_DATA_0_BASEADDR + 0x300)
#define DEBUG_LTL (XPAR_NX_DATA_0_BASEADDR + 0x304)
#define DEBUG_LTH (XPAR_NX_DATA_0_BASEADDR + 0x308)
#define DEBUG_ADC (XPAR_NX_DATA_0_BASEADDR + 0x30C)
#define DEBUG_NX2 (XPAR_NX_DATA_0_BASEADDR + 0x310)
#define DEBUG_LTL2 (XPAR_NX_DATA_0_BASEADDR + 0x314)
#define DEBUG_LTH2 (XPAR_NX_DATA_0_BASEADDR + 0x318)
#define DEBUG_ADC2 (XPAR_NX_DATA_0_BASEADDR + 0x31C)

#define OCM_REG1 (0xe8800000)
#define OCM_REG2 (0xe8800000 + 0x4)
#define OCM_REG3 (0xe8800000 + 0x8)

// #define OCM_REG1 (XPAR_NX_OCM_0_BASEADDR)
// #define OCM_REG2 (XPAR_NX_OCM_0_BASEADDR + 0x4)
// #define OCM_REG3 (XPAR_NX_OCM_0_BASEADDR + 0x8)

#define PREFETCH (XPAR_NX_DATA_0_BASEADDR + 0x400)
#define ACTIVATE_NX (XPAR_NX_DATA_0_BASEADDR + 0x404)
#define CHECK_PARITY (XPAR_NX_DATA_0_BASEADDR + 0x408)
#define FEB4nx (XPAR_NX_DATA_0_BASEADDR + 0x40C)

#define SYNC_M_DELAY (XPAR_NX_DATA_0_BASEADDR + 0x500)
#define SYNC_BAUD_START (XPAR_NX_DATA_0_BASEADDR + 0x504)
#define SYNC_BAUD1 (XPAR_NX_DATA_0_BASEADDR + 0x508)
#define SYNC_BAUD2 (XPAR_NX_DATA_0_BASEADDR + 0x50C)
#define AUX_ACTIVATE (XPAR_NX_DATA_0_BASEADDR + 0x510)

#define HW_VERSION (XPAR_NX_DATA_0_BASEADDR + 0x600)

#define WATCHDOG_OFF (XPAR_VIRTEX_OK_DISABLE_0_BASEADDR)

//Bitfile upload
#define MAX_NUMBER_OF_KIBFILES 2


int read_I2C(int i2cn, int reg);
void NX(int reg, int val);
void set_nx(int nxn, int mode);
unsigned long ungray(int x, int n);
void shiftit(Xuint32 shift);
int nx_autocalibration();
void nx_autodelay();
void nx_reset();
void decode(int mode);
void testsetup();
Xuint16 peek (Xuint32 addr, Xuint32 *retVal, Xuint8* rawdata, Xuint32* rawdatasize);
Xuint16 poke (Xuint32 addr, Xuint32 val, Xuint8* rawdata);
void load_config(char* filename);
void save_config(char* filename);
void activate_nx(int nx, int act, int con, int sa);
void prefetch (int pref);
Xuint8 calcBitfileBufferChecksum();
Xuint8 calcFileBufferChecksum();

/***************************************
 * translate commands to debug console
 ***************************************/

extern struct sc_config gC;


/*****************************************************
 * SysCore Configuration Structure
 *****************************************************/  
struct sc_config
{
   Xuint16    syscoreId;
   Xuint8     verbose; // mask 1 - console, 2 - network, 4 - UART  7 - all together
   Xuint8     etherAddrSc[ETHER_ADDR_LEN];
   Xuint8     ipAddrSc[IP_ADDR_LEN];
   Xuint8     ipNetmask[IP_ADDR_LEN];
   Xuint16    ctrlPort;
   Xuint16    dataPort;

   int        flushTimeout;
   int        burstLoopCnt;   // number of burst readouts in one loop
   int        ocmLoopCnt;     // number of com readouts in one loop

   int        rocNumber;
   
   int        prefetchActive;
   int        activateLowLevel;
   int        rocLTSDelay;
   int        rocNX1Delay;
   int        rocNX2Delay;
   int        rocNX3Delay;
   int        rocNX4Delay;
   int        rocSrInit1;
   int        rocBufgSelect1;
   int        rocSrInit2;
   int        rocBufgSelect2;
   int        rocAdcLatency1;
   int        rocAdcLatency2;
   int        rocAdcLatency3;
   int        rocAdcLatency4;
   int        NX_Number;
   int        rocFEB4nx;
   int        global_consolemode;
   
   int        testpatternGenerator;
};


/*****************************************************
 * Multiple nXYTER Configuration Structure
 *****************************************************/  

typedef struct {
  int active;
  int Connector;
  int I2C_SlaveAddr;
} NX_type;

extern NX_type NX_attribute[4];

extern char defConfigFileName[];

/*****************************************************
 * Receive Buffer
 * needed for decoding ethernet packets
 ****************************************************/
Xuint8 global_file_buffer[ROC_FBUF_SIZE];//4MB buffer for uploaded bitfile

/*****************************************************
 * SysCore Data Structure
 *****************************************************/ 

/*****************************************************
 * SysCore Protokoll Message Packet Structure
 *****************************************************/ 


   
struct SysCore_Message_Packet
{
   Xuint8  tag;
   Xuint8  pad[3];
   Xuint32 password;
   Xuint32 id;
   Xuint32 address;
   Xuint32 value;
};


#define SCP_HEADER_OFFSET UDP_PAYLOAD_OFFSET
#define SCP_PAYLOAD_OFFSET (SCP_HEADER_OFFSET + sizeof(struct SysCore_Message_Packet))
#define SCP_MESSAGE_ETHER_FRAME_SIZE (SCP_PAYLOAD_OFFSET + 80)
#define MAX_SCP_PAYLOAD MAX_UDP_PAYLOAD - sizeof(struct SysCore_Message_Packet)


/*****************************************************
 * public function declaration
 *****************************************************/

/****************************************
 * networkInit()
 * 
 *   -    initialize networkadapter using
 *      global variables in scp.c
 *   -   set MAC address
 ***************************************/
void networkInit();

/****************************************
 * int ethernetPacketDispatcher()
 * 
 * dispatches only one packet in FIFO
 * autoresponse to arp request in 
 * 
 * returns 1 if packet was dispatched
 ***************************************/
int ethernetPacketDispatcher();

/***************************************
 * Helper
 ***************************************/
void timer_set(XTime *timer, XTime timeoutMS);
Xuint8 timer_expired(XTime *timer);
void swap_byteorder(Xuint8* target, Xuint16 bytes);

/*---------------------------------------------------*/
/* The purpose of this routine is to output data the */
/* same as the standard printf function without the  */
/* overhead most run-time libraries involve. Usually */
/* the printf brings in many kilobytes of code and   */
/* that is unacceptable in most embedded systems.    */
/*---------------------------------------------------*/

void sc_printf(const char* fmt, ...);


// _____________________ NEW DEFINITIONS _____________________

struct SysCoreProfile
{
   XTime tm1;
   XTime tm2;
   Xuint32 cnt;
   Xuint32 sum;
};

#define StartProfile(name) \
   static struct SysCoreProfile name = { 0, 0, 0, 0 }; \
   XTime_GetTime(&(name.tm1));

#define StopProfile(name, limit, label) \
   XTime_GetTime(&(name.tm2)); \
   name.sum += (name.tm2 - name.tm1); \
   if (++name.cnt >= limit) { \
      int irate = (name.sum / name.cnt) * 1000; \
      int drate = irate % XPAR_CPU_PPC405_CORE_CLOCK_FREQ_HZ; \
      irate = irate / XPAR_CPU_PPC405_CORE_CLOCK_FREQ_HZ; \
      drate = (drate * 1000) / XPAR_CPU_PPC405_CORE_CLOCK_FREQ_HZ; \
      xil_printf(label" tm = %d us\r\n", irate*1000 + drate); \
      name.sum = 0; \
      name.cnt = 0; \
   }

struct SysCore_Data_Packet
{
   Xuint32 pktid;
   Xuint32 lastreqid;
   Xuint32 nummsg;
};

struct SysCore_Data_Request
{
   Xuint32 password;
   Xuint32 reqpktid;
   Xuint32 frontpktid; 
   Xuint32 tailpktid;
   Xuint32 numresend;
};

struct sc_statistic {
   Xuint32   dataRate;   // data taking rate in  B/s 
   Xuint32   sendRate;   // network send rate in B/s 
   Xuint32   recvRate;   // network recv rate in B/s 
   Xuint32   nopRate;    // double-NOP messages 1/s
   Xuint32   frameRate;  // unrecognised frames 1/s
   Xuint32   takePerf;   // relative use of time for data taing (*100000)
   Xuint32   dispPerf;   // relative use of time for packets dispatching (*100000)
   Xuint32   sendPerf;   // relative use of time for data sending (*100000)
};


#define RESEND_MAX_SIZE 128

// in 5 seconds master recognised as disconnected
#define MASTER_DISCONNECT_TIME 500

struct sc_data 
{
   Xuint8  **buf;      /* main buffer with all packages inside, only payload without headers */
   Xuint32   numalloc; /* number of allocated items in buffer */

//   Xuint8    udpheader[UDP_PAYLOAD_OFFSET];
   
   Xuint32   head;    /* buffer which is now filled */
   Xuint32   numbuf;  /* number of filled buffers*/
   Xuint32   headid;  /* id of the buffer in the head */
   
   Xuint32   high_water;      /* Maximum number of buffers to be used in normal situation */
   Xuint32   low_water;       /* Number of buffers to enable data taking after is was stopped */
   unsigned char data_taking_on;  /* indicate if we can take new buffer */
   
   Xuint32   send_head;   /* buffer which must be send next */
   Xuint32   send_limit;    /* limit for sending - can be in front of existing header */
   
   Xuint32   resend_indx[RESEND_MAX_SIZE]; /* indexes of buffers which should be resended */
   Xuint32   resend_size;                  /* number of indexes to be resended */
   
   Xuint32   last_req_pkt_id; /* id of last request packet, should be always send back */
   
   struct SysCore_Data_Packet*  curr_pkt;
   Xuint8*                      curr_target;
   
   int        masterConnected; // 1 if master controller is connected
   int        masterCounter;   // reverse counter for master connection

   Xuint16    masterDataPort;
   Xuint16    masterControlPort;
   Xuint8     masterIp[IP_ADDR_LEN];
   Xuint8     masterEther[ETHER_ADDR_LEN];
   
   Xuint32    lastMasterPacketId;
   Xuint8     recv_buf[MAX_ETHER_FRAME_SIZE];
   Xuint8     send_buf[MAX_ETHER_FRAME_SIZE];
   Xuint8     retry_buf[MAX_ETHER_FRAME_SIZE];
   Xuint32    retry_buf_rawsize;
   
   unsigned   daqState;
   
   XTime      lastFlushTime;

   unsigned   etherFramesOtherIpOrArp;
   
   Xuint32    xor0buff;
   Xuint32    xor1buff;
   
   struct sc_statistic stat; // statistic block
   XTime      lastStatTime;
   
   char       debugOutput[1024]; // last message over sc_printf

};

extern struct sc_data gD;
extern struct sc_statistic gCnt; // counter to accumulate statistic



void resetBuffers();
void processDataRequest(struct SysCore_Data_Request* req);
void addSystemMessage(Xuint8 id, int flush);
void sendConsoleData(Xuint32 addr, void* data, unsigned payloadSize);
int overwriteSDFile();

#endif
