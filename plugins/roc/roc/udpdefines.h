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

#ifndef ROC_UDPDEFINES
#define ROC_UDPDEFINES

#define ROC_PASSWORD                    832226211

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


#endif
