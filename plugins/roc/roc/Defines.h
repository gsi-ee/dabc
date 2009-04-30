/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/

#ifndef ROC_Defines
#define ROC_Defines

#define KNUT_VERSION                     0x01070001   //All parts share the first 4 digits


#define ROC_PASSWORD               832226211

#define ROC_SUCCESS                      0    //Success
#define ROC_READBACK_ERROR               1    //Readback failed (different value)
#define ROC_ADDRESS_ERROR                2    //Wrong / unexisting address
#define ROC_VALUE_ERROR                  3    //Wrong value
#define ROC_PERMISSION_ERROR             4    //Permission denied (to low level addresses)
#define ROC_ANSWER_DELAYED               5    //The called function needs longer to run. //DEPRECATED
#define ROC_NETWORK_TRANSMISSION_ERROR   6    //Some slow control udp-data packets got lost.
#define ROC_I2C_ERROR                    7    //No Response (No Ack) Error on I2C bus.






/************************************
 * address space
 ***********************************/

//myOPB (ROC hardware)
#define myOPB_LOW                          0x000000   // LOW-Marker of myOPB-Range
#define myOPB_HIGH                         0x0fffff   // HIGH-Marker of myOPB-Range

#define ROC_HARDWARE_VERSION               0x00600      // r    Hardware-Version
#define ROC_SYSTEM_RESET                   0x00604      // w    reboots the system completely
#define ROC_FIFO_RESET                     0x00018       // w    reset all FIFOs
#define ROC_NUMBER                         0x00040

#define ROC_TESTPULSE_RESET_DELAY          0x00050       // w
#define ROC_TESTPULSE_LENGTH               0x00054       // w
#define ROC_TESTPULSE_NUMBER               0x00058       // w
#define ROC_TESTPULSE_START                0x0005C       // w

#define ROC_NX_ACTIVE                      0x00404      // r/w
#define ROC_FEB4NX                         0x0040C      // r/w If it is set to 1, the use of the inputs is changed in a way, that the FEB4nx can be utilized.
#define ROC_PARITY_CHECK                   0x00408      // r/w

#define ROC_SYNC_M_SCALEDOWN               0x00500      // w
#define ROC_SYNC_BAUD_START                0x00504      // w
#define ROC_SYNC_BAUD1                     0x00508      // w
#define ROC_SYNC_BAUD2                     0x0050C      // w

#define ROC_AUX_ACTIVE                     0x00510      // r/w

#define ROC_I2C1_DATA                      0x10000    // r/w
#define ROC_I2C1_RESET                     0x10004    // w    //ACTIVE LOW!
#define ROC_I2C1_REGRESET                  0x10008    // w    //ACTIVE LOW!
#define ROC_I2C1_SLAVEADDR                 0x1000C    // r/w
#define ROC_I2C1_REGISTER                  0x10010    // w
#define ROC_I2C1_ERROR                     0x10020    // r

#define ROC_I2C2_DATA                      0x20000    // r/w
#define ROC_I2C2_RESET                     0x20004    // w    //ACTIVE LOW!
#define ROC_I2C2_REGRESET                  0x20008    // w    //ACTIVE LOW!
#define ROC_I2C2_SLAVEADDR                 0x2000C    // r/w
#define ROC_I2C2_REGISTER                  0x20010    // w
#define ROC_I2C2_ERROR                     0x20020    // r

#define ROC_NX_FIFO_EMPTY                  0x00004       // r    Ist eine der 3 FIFOs (NX, ADC, LTS) empty?
#define ROC_NX_FIFO_FULL                   0x00008       // r    Ist eine der 3 FIFOs full?

#define ROC_NX_DATA                        0x00000       // r
#define ROC_NX_INIT                        0x00010       // w    ACTIVE HIGH! - Starts nXYTER-Receiver-Init
#define ROC_NX_RESET                       0x00014       // w    ACTIVE LOW! - Resets the nXYTER
#define ROC_LT_LOW                         0x00020       // r    Die unteren 32 bit des LTS (muessen zuerst gelesen werden!)
#define ROC_LT_HIGH                        0x00024       // r    Die oberen 32 bit des LTS (muessen nach LT_LOW gelesen werden!)

#define ROC_NX_MISS                        0x00030       // r    Wieviele Ereignisse konnten nicht in der FIFO gespeichert werden?
#define ROC_NX_MISS_RESET                  0x00034       // w    Den Ereigniscounter reseten. (1 schreiben erzeugt Single Pulse)

#define ROC_ADC_DATA                       0x00100      // r

#define ROC_ADC_REG                        0x00100      // w
#define ROC_ADC_ADDR                       0x00110      // w
#define ROC_ADC_ANSWER                     0x00114      // r

#define ROC_ADC_REG2                       0x00120      // w
#define ROC_ADC_ADDR2                      0x00130      // w
#define ROC_ADC_ANSWER2                    0x00134      // r

#define ROC_BURST1                         0x00200      // r
#define ROC_BURST2                         0x00204      // r
#define ROC_BURST3                         0x00208      // r

#define ROC_DEBUG_NX                       0x00300      // r
#define ROC_DEBUG_LTL                      0x00304      // r
#define ROC_DEBUG_LTH                      0x00308      // r
#define ROC_DEBUG_ADC                      0x0030C      // r
#define ROC_DEBUG_NX2                      0x00310      // r
#define ROC_DEBUG_LTL2                     0x00314      // r
#define ROC_DEBUG_LTH2                     0x00318      // r
#define ROC_DEBUG_ADC2                     0x0031C      // r


#define ROC_SR_INIT                        0x00104      // r/w
#define ROC_BUFG_SELECT                    0x00108      // r/w
#define ROC_SR_INIT2                       0x00124      // r/w
#define ROC_BUFG_SELECT2                   0x00128      // r/w

#define ROC_DELAY_LTS                      0x0002C       // r/w
#define ROC_DELAY_NX0                      0x00060       // r/w
#define ROC_DELAY_NX1                      0x00064       // r/w
#define ROC_DELAY_NX2                      0x00068       // r/w
#define ROC_DELAY_NX3                      0x0006C       // r/w

#define ROC_ADC_LATENCY1                   0x00140      // r/w
#define ROC_ADC_LATENCY2                   0x00144      // r/w
#define ROC_ADC_LATENCY3                   0x00148      // r/w
#define ROC_ADC_LATENCY4                   0x0014C      // r/w

#define ROC_AUX_DATA_LOW                   0x00150      // r
#define ROC_AUX_DATA_HIGH                  0x00154      // r

#define ROC_ADC_PORT_SELECT1               0x00160      // r/w
#define ROC_ADC_PORT_SELECT2               0x00164      // r/w
#define ROC_ADC_PORT_SELECT3               0x00168      // r/w
#define ROC_ADC_PORT_SELECT4               0x0016C      // r/w




//ROC software
#define ROC_SOFTWARE_VERSION        0x100000   // r    Software-Version
#define ROC_BURST                   0x100001   // r/w  activate OPB-2-OCM-Prefetcher
#define ROC_OCM_REG1                0x100100   // r    Die fuer den Burstzugriff zurechtgemachten Daten - Byte 1
#define ROC_OCM_REG2                0x100101   // r    Die fuer den Burstzugriff zurechtgemachten Daten - Byte 2
#define ROC_OCM_REG3                0x100102   // r    Die fuer den Burstzugriff zurechtgemachten Daten - Byte 3


//sdcard
#define ROC_CFG_READ                0x100200   // w    Beim Schreiben einer 1 werden die Konfigurationsdaten (IP, MAC, Delays) von der SD-Karte gelesen
#define ROC_CFG_WRITE               0x100201   // w    Beim Schreiben einer 1 werden die Konfigurationsdaten (IP, MAC, Delays) auf der SD-Karte gespeichert


//network
#define ROC_FLASH_KIBFILE_FROM_DDR  0x100302 // w position value is parsed right now only 0 is supported
#define ROC_SWITCHCONSOLE           0x100303 // r/w
#define ROC_START_DAQ               0x100304 // w
#define ROC_STOP_DAQ                0x100305 // w
#define ROC_IP_ADDRESS              0x100306 // r/w
#define ROC_NETMASK                 0x100307 // r/w
#define ROC_MAC_ADDRESS_UPPER       0x100308 // r/w upper two bytes
#define ROC_MAC_ADDRESS_LOWER       0x100309 // r/w lower four bytes
#define ROC_MASTER_LOGIN            0x10030A // r/w
#define ROC_MASTER_DATAPORT         0x10030B // r/w  data port of master node
#define ROC_MASTER_CTRLPORT         0x10030C // r/w
#define ROC_BUFFER_FLUSH_TIMER      0x10030E // r/w
#define ROC_MASTER_LOGOUT           0x100310 // w
#define ROC_MASTER_IP               0x100311 // r
#define ROC_RESTART_NETWORK         0x100312 // w
#define ROC_CONSOLE_OUTPUT          0x100313 // w
#define ROC_LOST_ETHER_FRAMES       0x100315 // r ethernet frames lost
#define ROC_UKNOWN_ETHER_FRAMES     0x100316 // r number of ethernet frames with tag other than IP and ARP
#define ROC_SUSPEND_DAQ             0x100319 // w go into suspend state
#define ROC_CLEAR_FILEBUFFER        0x10031A // w
#define ROC_CHECK_BITFILEBUFFER     0x10031B // r
#define ROC_CHECK_BITFILEFLASH0     0x10031C // r
#define ROC_CHECK_BITFILEFLASH1     0x10031D // r
#define ROC_CHECK_FILEBUFFER        0x10031E // r
#define ROC_TEMAC_PRINT             0x100320 // w - just print some Temac registers and config
#define ROC_TEMAC_REG0              0x100321 // r/w - set or read Remac config register
#define ROC_OVERWRITE_SD_FILE       0x100322 // w
#define ROC_CONSOLE_CMD             0x100323 // w - executes provided console command
#define ROC_CTRLPORT                0x100324 // r/w - control port number
#define ROC_DATAPORT                0x100325 // r/w - data port number
#define ROC_BURST_LOOPCNT           0x100326 // r/w - number of readout in burst loops
#define ROC_OCM_LOOPCNT             0x100327 // r/w - number of readout in ocm loops
#define ROC_STATBLOCK               0x100328 // r - block with sc_statistic
#define ROC_DEBUGMSG                0x100329 // r - last output of sc_printf command
#define ROC_HIGHWATER               0x10032A // r/w - value of high water marker (in number of UDP buffers, maximum is about 78000 buffers or 112 MB)
#define ROC_LOWWATER                0x10032B // r/w - value of low water marker (in number of UDP buffers, default is 10% of highwater)
#define ROC_NUMBUFALLOC             0x10032C // r - number of allocated UDP buffers

//Flash
#define ROC_FBUF_START              0x200000   //       first address of file buffer
#define ROC_FBUF_SIZE               0x400000  //       size of file buffer (4MB)

/************************************
 * address space finished
 ***********************************/





// default ports number of ROC itself
#define ROC_DEFAULT_CTRLPORT        13132
#define ROC_DEFAULT_DATAPORT        13131

//ROC TAGS
#define ROC_PEEK                    111
#define ROC_POKE                    112
#define ROC_CONSOLE                 113

//ROC/FEB Parameters
#define CON19                       0
#define CON20                       1

#endif
