/* This file provides the header for the functions
   the uIP stack needs to communicate with the driver */

#ifndef TAPDEV_H
#define TAPDEV_H

#include "xexception_l.h"
#include "xbasic_types.h"
#include "xpseudo_asm.h"

typedef Xuint8 u8_t;
typedef Xuint16 u16_t;
typedef Xuint32 u32_t;
typedef Xint8 s8_t;
typedef Xint16 s16_t;
typedef Xint32 s32_t;

#include "xparameters.h"     /* defines XPAR values */
#include "xtemac.h"          /* defines XTemac API */
#include "stdio.h"           /* stdio */

/* From xparameters.h */
#define TEMAC_DEVICE_ID   XPAR_PLB_TEMAC_0_DEVICE_ID

#define TEMAC_PHY_DELAY_SEC  4   /* Amount of time to delay waiting on PHY */

/* Define an aligned data type for an ethernet frame. This declaration is
   specific to the GNU compiler */
//typedef char EthernetFrame[XTE_MAX_FRAME_SIZE] __attribute__ ((aligned (8)));

XTemac TemacInstance;   /* Device instance used throughout examples */
extern char TemacMAC[]; /* MAC address, defined in tapdev.c */
extern int TemacLinkSpeed;   /* Speed selection - should be 10, 100 or 1000 */
extern Xuint16 TemacInitReg0; /* If set, TemacConfigurePhy will be called for PHY Reg0 */
extern unsigned TemacLostFrames; /* Conted number of lost ethernet frames */ 

//EthernetFrame TxFrame;    /* Transmit frame buffer */
//EthernetFrame RxFrame;    /* Receive frame buffer */

/* Functions implemented in tapdev.c */

// initializes the TEMAC
int tapdev_init();
int TemacConfigurePhy(XTemac *TemacInstancePtr, Xuint16 reg0set);

// writes data to the outgoing FIFO
void tapdev_write_to_fifo_partial(Xuint8 *buf, Xuint16 len);
// writes an end of frame to the outgoing FIFO
void tapdev_write_to_fifo_end_of_packet(Xuint8 *buf, Xuint16 len);

// send the outgoing FIFO
void tapdev_send(Xuint16 len);
void tapdev_send_fullpacket(Xuint8 *buf, Xuint16 len);

// tries to read a frame from the ingoing FIFO
int tapdev_tryread_full_packet(Xuint8 *pbuf, Xuint32 len);

void tapdev_read_from_fifo_new_packet(Xuint32 *RxFrameLenght);
void tapdev_read_from_fifo_partial(Xuint8 *buf, Xuint32 len);
void tapdev_read_from_fifo_flush_end_of_packet();

// returns the number of free bytes in the outgoing FIFO
Xuint32 tapdev_freebytes();

XStatus TemacGetTxStatus();
XStatus TemacGetRxStatus();
XStatus TemacResetDevice();
Xuint32 TemacDetectPHY(XTemac *TemacInstancePtr);
int TemacReadoutPhyRegisters(XTemac *TemacInstancePtr);
Xuint16 TemacReadCtrlReg(unsigned reg, Xuint32* retVal);
Xuint16 TemacWriteCtrlReg(unsigned reg, Xuint32 val);
int TemacPrintPhyRegisters();
void TemacPrintConfig();


#endif
