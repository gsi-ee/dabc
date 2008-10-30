/* This file provides the functions the uIP stack
   needs to communicate with the driver */

#include "tapdev.h"

#define TEMAC_DEVICE_ID   XPAR_PLB_TEMAC_0_DEVICE_ID

/* Define the MAC address of the local device */
char TemacMAC[6] = { 0x00, 0xac, 0x3b, 0x33, 0x05, 0x71 }; /* Local MAC address */
/* Speed selection - should be 10, 100 or 1000 */
int TemacLinkSpeed = 100;   
/* If set, TemacConfigurePhy will be called for PHY Reg0 */
Xuint16 TemacInitReg0 = 0; // was a 0x3300 ?

unsigned TemacLostFrames = 0;

/* ----------------------------------------------------------------------- */


/* Initialize the XTemac */
int tapdev_init() 
{
   
    XStatus Status;


    /*************************************/
   /* Setup device for first-time usage */
    /*************************************/

   /*
    * Initialize the TEMAC instance
    */
   Status = XTemac_Initialize(&TemacInstance, TEMAC_DEVICE_ID);
   if (Status != XST_SUCCESS) {
      xil_printf("Error in initialize\r\n");
      return XST_FAILURE;
   }

    XTemac_Reset(&TemacInstance, XTE_NORESET_HARD);

    // Run a self=test on the TEMAC
//    Status = XTemac_SelfTest(&TemacInstance);
//    if(Status != XST_SUCCESS) {
//        xil_printf("Error in reset\r\n");
//        return XST_FAILURE;
//    }


   /*
    * Check whether the IPIF interface is correct for this example
    */
   if (!XTemac_mIsFifo(&TemacInstance)) {
      xil_printf
         ("Device HW not configured for FIFO direct mode\r\n");
      return XST_FAILURE;
   }
   


   /*
    * Set the MAC  address
    */
   Status = XTemac_SetMacAddress(&TemacInstance, (Xuint8*) TemacMAC);
   if (Status != XST_SUCCESS) {
      xil_printf("Error setting MAC address\r\n");
      return XST_FAILURE;
   }

   /*
    * Enable options needed 
    */
   Status = XTemac_SetOptions(&TemacInstance, XTE_POLLED_OPTION);
   if (Status != XST_SUCCESS) {
      xil_printf("Error setting options\r\n");
      return XST_FAILURE;
   }


   //Set PHY to correct mode
    
    if (TemacInitReg0) { 
     
        usleep(10000);      // Delay after setting the MAC address 
        Status = TemacConfigurePhy(&TemacInstance, TemacInitReg0);
        if (Status != XST_SUCCESS) {
           xil_printf("Error setting the PHY loopback\r\n");
           return XST_FAILURE;
        }
    }

   /*
    * Set PHY<-->MAC data clock
    */
   XTemac_SetOperatingSpeed(&TemacInstance, TemacLinkSpeed);

   usleep(10000);      // Delay after setting the MAC address 

   /*
    * Start the TEMAC device
    */
   Status = XTemac_Start(&TemacInstance);
   if (Status != XST_SUCCESS) {
      xil_printf("Error starting devicet");
      return XST_FAILURE;
   }

   return XST_SUCCESS;
}


/******************************************************************************/
/**
*
* This function detects the PHY address by looking for successful MII status
* register contents (PHY register 1). It looks for a PHY that supports
* auto-negotiation and 10Mbps full-duplex and half-duplex.  So, this code
* won't work for PHYs that don't support those features, but it's a bit more
* general purpose than matching a specific PHY manufacturer ID.
*
* Note also that on some (older) Xilinx ML4xx boards, PHY address 0 does not
* properly respond to this query.  But, since the default is 0 and asssuming
* no other address responds, then it seems to work OK.
*
* @param    The TEMAC driver instance
*
* @return   The address of the PHY (defaults to 0 if none detected)
*
* @note     None.
*
******************************************************************************/
/* Use MII register 1 (MII status register) to detect PHY */
#define PHY_DETECT_REG  1

/* Mask used to verify certain PHY features (or register contents)
 * in the register above:
 *  0x1000: 10Mbps full duplex support
 *  0x0800: 10Mbps half duplex support
 *  0x0008: Auto-negotiation support
 */
#define PHY_DETECT_MASK 0x1808

Xuint32 TemacDetectPHY(XTemac *TemacInstancePtr)
{
   //xil_printf("starting phy detect\r\n");
   
   int Status;
   Xuint16 PhyReg;
   Xuint32 PhyAddr;

   for (PhyAddr = 0; PhyAddr <= 31; PhyAddr++) {
      Status = XTemac_PhyRead(TemacInstancePtr, PhyAddr,
               PHY_DETECT_REG, &PhyReg);

      if ((Status == XST_SUCCESS) && (PhyReg != 0xFFFF) &&
          ((PhyReg & PHY_DETECT_MASK) == PHY_DETECT_MASK)) {
         /* Found a valid PHY address */
         return PhyAddr;
      }
   }

   return 0;      /* default to zero */
}


/******************************************************************************/
/**
 * Set the Intel LXT971A PHY to correct mode. It is adapted by the 2.6.20 startup
 * code.
 *
 ******************************************************************************/
/* register definitions for the 971 */
#define MII_LXT971_IER      18  /* Interrupt Enable Register */
#define MII_LXT971_IER_IEN   0x00f2

#define MII_LXT971_ISR      19  /* Interrupt Status Register */



/******************************************************************************/
/**
*
* This function sets up the PHY.
*
* @param    Speed is the loopback speed 10, 100, or 1000 Mbit.
*
* @return   XST_SUCCESS if successful, else XST_FAILURE.
*
* @note     None.
*
******************************************************************************/
int TemacConfigurePhy(XTemac *TemacInstancePtr, Xuint16 reg0set)
{
   xil_printf("Setting ethernet phy registers ...\r\n");
   
   int Status;
   Xuint16 PhyReg0;//ctrl
   /*
   Xuint16 PhyReg1;//status
   Xuint16 PhyReg17;//status 2
   Xuint16 PhyReg4;//auto negotioation
   Xuint16 PhyReg5;//auto negotioation partner ability
   Xuint16 PhyReg6;//auto negotioation errors
   Xuint16 PhyReg16;//configuration
   Xuint16 PhyReg18;//interuppt en
   Xuint16 PhyReg19;//interuppt en
   */

   /* Detec the PHY address */
   Xuint32 PhyAddr = TemacDetectPHY(TemacInstancePtr);
    
   xil_printf("Detect PhyAddr = 0x%x\r\n",PhyAddr);
   
   Status = XTemac_PhyRead(TemacInstancePtr, PhyAddr, 0, &PhyReg0);
   
   // There seem to be some sleeps needing between PhyReads/PhyWrites 
   usleep(100000);
   
   //100mbit, full-duplex, auto-negotiation, reset
   // PhyReg0 = 0x3300;
   PhyReg0 = reg0set;

   //xil_printf("starting phy write\r\n");      
   Status = XTemac_PhyWrite(TemacInstancePtr, PhyAddr, 0, PhyReg0);

   usleep(100000);

    /*
   Status |= XTemac_PhyWrite(TemacInstancePtr, PhyAddr, 0,
              PhyReg0 | PHY_R0_RESET);
   usleep(1000000 * TEMAC_PHY_DELAY_SEC);
   Status |= XTemac_PhyWrite(TemacInstancePtr, PhyAddr, 0,
              PhyReg0 | PHY_R0_LOOPBACK);
   usleep(1000000);
   */

   //xil_printf("phy write end\r\n");   
   if (Status != XST_SUCCESS) {
      xil_printf("Error writing phy");
      return XST_FAILURE;
   }

   return XST_SUCCESS;
}


/******************************************************************************/
/**
* This functions get the Tx status.
* If an error is reported, it handles all the possible  error conditions.
*
* @param    None.
*
* @return   Status is the status of the last call to the
*           XTemac_FifoQuerySendStatus() function.
*
* @note     None.
*
******************************************************************************/
XStatus TemacGetTxStatus(void)
{
    Xuint32 SendStatus;
    XStatus Status;

    /*
     * Try to get the status
     */
    Status = XTemac_FifoQuerySendStatus(&TemacInstance, &SendStatus);

    switch(Status)
    {
        case XST_SUCCESS:  /* Frame sent without error */
        case XST_NO_DATA:  /* Timeout */
            break;

        case XST_PFIFO_ERROR:
            xil_printf("Packet FIFO error\r\n");
            TemacResetDevice();
            break;

        case XST_IPIF_ERROR:
            print("IPIF error\r\n");
            TemacResetDevice();
            break;

        case XST_FIFO_ERROR:
            print("Status/Length FIFO error\r\n");
            TemacResetDevice();
            break;

        case XST_NOT_POLLED:
            print("Driver was not in polled mode\r\n");
            break;

        case XST_DEVICE_IS_STOPPED:
            print("Device was stopped prior to polling operation\r\n");
            break;

        default:
            print("Driver returned unknown transmit status\r\n");
            break;
    }

    return(Status);
}


/******************************************************************************/
/**
* This functions gets the Rx status. If an error is reported,
* handle all the possible  error conditions.
*
* @param    None.
*
* @return   Status is the status of the last call to the
*           XTemac_FifoQueryRecvStatus() function.
*
* @note     None.
*
******************************************************************************/
XStatus TemacGetRxStatus(void)
{
    XStatus Status;

    /*
     * There are two ways to poll for a received frame:
     *
     * XTemac_Recv() can be used and repeatedly called until it returns a
     * length,  but this method does not provide any error detection.
     *
     * XTemac_FifoQueryRecvStatus() can be used and this function provides
     * more information to handle error conditions.
     */

    /*
     * Try to get status
     */
    Status = XTemac_FifoQueryRecvStatus(&TemacInstance);

    switch(Status)
    {
        case XST_SUCCESS:  /* Frame has arrived */
        case XST_NO_DATA:  /* Timeout */
            break;

        case XST_DATA_LOST:
           TemacLostFrames++;        
            //print("Inbound frame was dropped\r\n");
            break;

        case XST_PFIFO_ERROR:
            print("Packet FIFO error\r\n");
            TemacResetDevice();
            break;

        case XST_IPIF_ERROR:
            print("IPIF error\r\n");
            TemacResetDevice();
            break;

        case XST_FIFO_ERROR:
            print("Length/status FIFO error\r\n");
            TemacResetDevice();
            break;

        case XST_DEVICE_IS_STOPPED:
            print("Device was stopped prior to polling operation\r\n");
            break;

        default:
            print("Driver returned invalid transmit status\r\n");
            break;
    }

    return(Status);
}



/******************************************************************************/
/**
* This function resets the device but preserves the options set by the user.
*
* @param    None.
*
* @return   XST_SUCCESS if successful, else XST_FAILURE.
*
* @note     None.
*
******************************************************************************/
XStatus TemacResetDevice(void)
{
    XStatus Status;
    Xuint8 MacSave[6];
    Xuint32 Options;

    /*
     * Stop device
     */
    XTemac_Stop(&TemacInstance);

    /*
     * Save the device state
     */
    XTemac_GetMacAddress(&TemacInstance, MacSave);
    Options = XTemac_GetOptions(&TemacInstance);

    /*
     * Stop and reset the device
     */
    XTemac_Reset(&TemacInstance, XTE_NORESET_HARD);

    /*
     * Restore the state
     */
    Status  = XTemac_SetMacAddress(&TemacInstance, MacSave);
    Status |= XTemac_SetOptions(&TemacInstance, Options);
    Status |= XTemac_ClearOptions(&TemacInstance, ~Options);
    if (Status != XST_SUCCESS)
    {
        print("Error restoring state after reset\r\n");
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}



Xuint32 tapdev_freebytes()
{
   return XTemac_FifoGetFreeBytes(&TemacInstance, XTE_SEND);
}

void tapdev_write_to_fifo_partial(Xuint8 *buf, Xuint16 len) {
   XStatus Status;
   Xuint32 FifoFreeBytes;
   /* Wait for enough room in FIFO to become available */
   /* will hang the program if fifo is stucked*/
   do {
      FifoFreeBytes = XTemac_FifoGetFreeBytes(&TemacInstance, XTE_SEND);
   } while (FifoFreeBytes < len);
      Status = XTemac_FifoWrite(&TemacInstance, buf, len, XTE_PARTIAL_PACKET);
      if (Status != XST_SUCCESS) {
        print("Error writing to FIFO ... reseting device\r\n");
        TemacResetDevice();
        tapdev_init();
        return;
      }
}

void tapdev_write_to_fifo_end_of_packet(Xuint8 *buf, Xuint16 len) {
   XStatus Status;
   Xuint32 FifoFreeBytes;
   /* Wait for enough room in FIFO to become available */
   /* will hang the program if fifo is stucked*/
   do {
      FifoFreeBytes = XTemac_FifoGetFreeBytes(&TemacInstance, XTE_SEND);
   } while (FifoFreeBytes < len);
      Status = XTemac_FifoWrite(&TemacInstance, buf, len, XTE_END_OF_PACKET);
      if (Status != XST_SUCCESS) {
        print("Error writing to FIFO ... reseting device\r\n");
        TemacResetDevice();
        tapdev_init();
        return;
      }
}

void tapdev_send(Xuint16 len) {

   XStatus Status;

   /* Initiate transmit */
   Status = XTemac_FifoSend(&TemacInstance, len);
   if (Status != XST_SUCCESS) {
       print("Error on transmit ... reseting device\r\n");
       TemacResetDevice();
       tapdev_init();
       return;
   }


   /* Wait for status of the transmitted packet */
   switch(TemacGetTxStatus())
   {
      case XST_SUCCESS: /* Got a sucessfull transmit status */
      case XST_NO_DATA: /* Timed out */
         break;
      default:
         /* Some other error */
         break;
   }

   return;
}

void tapdev_read_from_fifo_new_packet(Xuint32 *RxFrameLength)
{
   if (XTemac_FifoRecv(&TemacInstance, RxFrameLength) != XST_SUCCESS)
      print("Error reading new Packet!\r\n");
}

void tapdev_read_from_fifo_partial(Xuint8 *buf, Xuint32 len)
{
   if(XTemac_FifoRead(&TemacInstance, buf, len, XTE_PARTIAL_PACKET) != XST_SUCCESS);
      print("Error in FifoRead\r\n");
}
   
void tapdev_read_from_fifo_flush_end_of_packet()
{
   Xuint8 *buf=0;
   if (XTemac_FifoRead(&TemacInstance, buf, 0, XTE_END_OF_PACKET) != XST_SUCCESS);
      //print("Error in FifoFlush\r\n");
}

/* Try to read out a full packet from the FIFO and write it into buf, including all headers */
int tapdev_tryread_full_packet(Xuint8 *buf, Xuint32 len) {
   
   XStatus Status;
   Xuint32 RxFrameLength;
   Xuint32 readlen=0;

   /* Check if frame has arrived */
   switch(TemacGetRxStatus())
   {
      case XST_SUCCESS:  /* Got a successfull receive status */
         break;

      case XST_NO_DATA:  /* Timed out */
      case XST_DATA_LOST:
         return 0;
         break;
      default:           /* Some other error */
         TemacResetDevice();
         tapdev_init();
         return 0;
   }
   
   /* Check if packet has arrived, if so read it out */
   if (XTemac_FifoRecv(&TemacInstance, &RxFrameLength) == XST_SUCCESS) {
       if (RxFrameLength > 14) {
         if (RxFrameLength > len) {
            print("Not enough space in pbuf\r\n");
            readlen = len;
         }
         else
            readlen = RxFrameLength;
          Status = XTemac_FifoRead(&TemacInstance, buf, readlen,
                                    XTE_END_OF_PACKET);

          if (Status != XST_SUCCESS) {
             print("Error in FifoRead\r\n");
             TemacResetDevice();
             tapdev_init();
             return 0;
          }
       }
       return readlen;
   } else {
       return 0;
   }
}


void tapdev_send_fullpacket(Xuint8 *buf, Xuint16 len) {

   XStatus Status;
   Xuint32 FifoFreeBytes;
      /* Wait for enough room in FIFO to become available */
      do {
         FifoFreeBytes = XTemac_FifoGetFreeBytes(&TemacInstance, XTE_SEND);
         /*if (FifoFreeBytes < len) {
            print("FifoFreeBytes < len\r\n");
         }*/
         //if (FifoFreeBytes < len)
         //   print("FifoFreeBytes < len\r\n");
      } while (FifoFreeBytes < len);
      /* Write the frame header to FIFO */
      //Status = XTemac_FifoWrite(&TemacInstance, buf, len, XTE_PARTIAL_PACKET);
      Status = XTemac_FifoWrite(&TemacInstance, buf, len, XTE_END_OF_PACKET);
      if (Status != XST_SUCCESS) {
        print("Error writing to FIFO\r\n");
        TemacResetDevice();
        tapdev_init();
        return;
      }

   /* Initiate transmit */
   Status = XTemac_FifoSend(&TemacInstance, len);
   if (Status != XST_SUCCESS) {
       print("Error on transmit\r\n");
       TemacResetDevice();
       tapdev_init();
       return;
   }

        
   /* Wait for status of the transmitted packet */
   switch(TemacGetTxStatus())
   {
      case XST_SUCCESS: /* Got a sucessfull transmit status */
      case XST_NO_DATA: /* Timed out */
         break;
      default:
         /* Some other error */
         break;
   }
   return;
}


int TemacReadoutPhyRegisters(XTemac *TemacInstancePtr)
{
    XStatus Status;
    Xuint16 PhyReg;
    Xuint32 PhyAddr;

    /* Detec the PHY address */
    PhyAddr = TemacDetectPHY(TemacInstancePtr);

    sleep(1);
    Status = XTemac_PhyRead(TemacInstancePtr, PhyAddr, 0, &PhyReg);
    Status |= XTemac_PhyRead(TemacInstancePtr, PhyAddr, 1, &PhyReg);
    Status |= XTemac_PhyRead(TemacInstancePtr, PhyAddr, 4, &PhyReg);
    Status |= XTemac_PhyRead(TemacInstancePtr, PhyAddr, 5, &PhyReg);
    Status |= XTemac_PhyRead(TemacInstancePtr, PhyAddr, 6, &PhyReg);
    Status |= XTemac_PhyRead(TemacInstancePtr, PhyAddr, 16, &PhyReg);
    Status |= XTemac_PhyRead(TemacInstancePtr, PhyAddr, 17, &PhyReg);
    Status |= XTemac_PhyRead(TemacInstancePtr, PhyAddr, 18, &PhyReg);
    Status |= XTemac_PhyRead(TemacInstancePtr, PhyAddr, 19, &PhyReg);
    sleep(1);

    if (Status != XST_SUCCESS) {
       xil_printf("Error reading phy");
       return XST_FAILURE;
    }
    return XST_SUCCESS;
}


Xuint16 TemacReadCtrlReg(unsigned reg, Xuint32* retVal)
{
   Xuint32 PhyAddr = TemacDetectPHY(&TemacInstance);

   Xuint16 PhyReg;

   XStatus Status = XTemac_PhyRead(&TemacInstance, PhyAddr, reg, &PhyReg);
   
   *retVal = PhyReg;

   return (Status == XST_SUCCESS) ? 0 : 2;
}

Xuint16 TemacWriteCtrlReg(unsigned regnum, Xuint32 val)
{
   Xuint32 PhyAddr = TemacDetectPHY(&TemacInstance);

   Xuint16 PhyReg = val;
   
   XStatus Status = XTemac_PhyWrite(&TemacInstance, PhyAddr, regnum, PhyReg);

   return (Status == XST_SUCCESS) ? 0 : 2;
}

int TemacPrintPhyRegisters()
{
   int Status;
   Xuint16 PhyReg0;//ctrl
   Xuint16 PhyReg1;//status
   Xuint16 PhyReg17;//status 2
   Xuint16 PhyReg4;//auto negotioation
   Xuint16 PhyReg5;//auto negotioation partner ability
   Xuint16 PhyReg6;//auto negotioation errors
   Xuint16 PhyReg16;//configuration
   Xuint16 PhyReg18;//interuppt en
   Xuint16 PhyReg19;//interuppt status
   
   Xuint32 PhyAddr;

   /* Detec the PHY address */
   PhyAddr = TemacDetectPHY(&TemacInstance);
   //xil_printf("phy detect end address = 0x%x\r\n", PhyAddr);

   Status = XTemac_PhyRead(&TemacInstance, PhyAddr, 0, &PhyReg0);
   Status |= XTemac_PhyRead(&TemacInstance, PhyAddr, 1, &PhyReg1);
   Status |= XTemac_PhyRead(&TemacInstance, PhyAddr, 4, &PhyReg4);
   Status |= XTemac_PhyRead(&TemacInstance, PhyAddr, 5, &PhyReg5);
   Status |= XTemac_PhyRead(&TemacInstance, PhyAddr, 6, &PhyReg6);
   Status |= XTemac_PhyRead(&TemacInstance, PhyAddr, 16, &PhyReg16);
   Status |= XTemac_PhyRead(&TemacInstance, PhyAddr, 17, &PhyReg17);
   Status |= XTemac_PhyRead(&TemacInstance, PhyAddr, 18, &PhyReg18);
   Status |= XTemac_PhyRead(&TemacInstance, PhyAddr, 19, &PhyReg19);
                        
   if (Status != XST_SUCCESS) {
      sc_printf("Error reading phy");
      return XST_FAILURE;
   }

   sc_printf("Printing temac registers, PhyAddr = 0x%x\r\n", PhyAddr);
   sc_printf("   PhyReg0  = 0x%x ctrl\r\n", PhyReg0);
   sc_printf("   PhyReg1  = 0x%x status\r\n", PhyReg1);
   sc_printf("   PhyReg17 = 0x%x status2\r\n", PhyReg17);
   sc_printf("   PhyReg4  = 0x%x auto negatition\r\n", PhyReg4);
   sc_printf("   PhyReg5  = 0x%x auto negotioation partner ability \r\n", PhyReg5);
   sc_printf("   PhyReg6  = 0x%x auto negotioation errors\r\n", PhyReg6);
   sc_printf("   PhyReg16 = 0x%x configuration\r\n", PhyReg16);
   sc_printf("   PhyReg18 = 0x%x interuppt en\r\n", PhyReg18);
   sc_printf("   PhyReg19 = 0x%x interuppt status\r\n", PhyReg19);
   
   return XST_SUCCESS;
}


void TemacPrintConfig()
{
   sc_printf("Printout of current Temac config\r\n");
   sc_printf("Unique ID of device = %u\r\n", TemacInstance.Config.DeviceId);
   sc_printf("Physical base address of IPIF registers = 0x%x\r\n", TemacInstance.Config.BaseAddress);
   sc_printf("Depth of receive packet FIFO in bits = %u\r\n", TemacInstance.Config.RxPktFifoDepth);
   sc_printf("Depth of transmit packet FIFO in bits = %u\r\n", TemacInstance.Config.TxPktFifoDepth);
   sc_printf("Depth of the status/length FIFOs in entries = %u\r\n", TemacInstance.Config.MacFifoDepth);
   sc_printf("IPIF/DMA hardware configuration = 0x%x\r\n", TemacInstance.Config.IpIfDmaConfig);
   sc_printf("Has data realignment engine on Tx channel = %u\r\n", TemacInstance.Config.TxDre);
   sc_printf("Has data realignment engine on Rx channel = %u\r\n", TemacInstance.Config.RxDre);
   sc_printf("Has checksum offload on Tx channel = %u\r\n", TemacInstance.Config.TxCsum);
   sc_printf("Has checksum offload on Tx channel = %u\r\n", TemacInstance.Config.RxCsum);
   sc_printf("Which type of PHY interface is used = %u\r\n", TemacInstance.Config.PhyType);
}
