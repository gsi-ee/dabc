#ifndef DEFINES_H
#define DEFINES_H

#define ROC_PASSWORD               832226211

#define ROC_SUCCESS                  0    //Success 
#define ROC_READBACK_ERROR            1    //Readback failed (different value) 
#define ROC_ADDRESS_ERROR            2    //Wrong / unexisting address 
#define ROC_VALUE_ERROR               3    //Wrong value 
#define ROC_PERMISSION_ERROR         4    //Permission denied (to low level addresses) 
#define ROC_ANSWER_DELAYED            5    //The called function needs longer to run. 
#define ROC_NETWORK_TRANSMISSION_ERROR   6    //Some slow control udp-data packets got lost.
#define ROC_I2C_ERROR               7    //No Response (No Ack) Error on I2C bus. 


/************************************
 * address space
 ***********************************/

//ROC high level
#define ROC_SOFTWARE_VERSION      0x0000   // r    Software-Version
#define ROC_HARDWARE_VERSION      0x0001   // r    Hardware-Version 
#define ROC_SYSTEM_RESET         0x0001   // w    reboots the system completely
#define ROC_FIFO_RESET            0x0002   // w    reset all FIFOs
#define ROC_NUMBER               0x0010   // r/w    ROC number
#define ROC_BURST               0x0020   // r/w    activate OPB-2-OCM-Prefetcher  ==> PREFETCH 
#define ROC_TESTPULSE_RESET_DELAY   0x0030
#define ROC_TESTPULSE_LENGTH      0x0031
#define ROC_TESTPULSE_NUMBER      0x0032
#define ROC_TESTPULSE_START         0x0033
#define ROC_TESTPULSE_RESET         0x0034
#define ROC_NX_SELECT            0x0040
#define ROC_NX_ACTIVE            0x0041
#define ROC_NX_CONNECTOR         0x0042
#define ROC_NX_SLAVEADDR         0x0043
#define ROC_FEB4NX               0x0045 // r/w If it is set to 1, the use of the inputs is changed in a way, that the FEB4nx can be utilized. 
#define ROC_PARITY_CHECK         0x0050
#define ROC_SYNC_M_SCALEDOWN        0x0060
#define ROC_SYNC_BAUD_START         0x0061
#define ROC_SYNC_BAUD1             0x0062
#define ROC_SYNC_BAUD2               0x0063
#define ROC_AUX_ACTIVE              0x0064
#define ROC_NX_REGISTER_BASE      0x0100   //- 0x012D    r/w    Reads or writes to the I2C register 0. This command communicates with the nXYTER selected by NX_SELECT. 
#define ROC_NX_FIFO_EMPTY         0x0200   // r    Ist eine der 3 FIFOs (NX, ADC, LTS) empty? 
#define ROC_NX_FIFO_FULL         0x0201   // r    Ist eine der 3 FIFOs full?

//sdcard
#define ROC_CFG_READ            0x0300   // w    Beim Schreiben einer 1 werden die Konfigurationsdaten (IP, MAC, Delays) von der SD-Karte gelesen 
#define ROC_CFG_WRITE            0x0301   // w    Beim Schreiben einer 1 werden die Konfigurationsdaten (IP, MAC, Delays) auf der SD-Karte gespeichert

#define ROC_FLASH_KIBFILE_FROM_DDR    0x0302 // w position value is parsed right now only 0 is supported
#define ROC_SWITCHCONSOLE         0x0303 // r/w
#define ROC_START_DAQ            0x0304 // w
#define ROC_STOP_DAQ            0x0305 // w
#define ROC_IP_ADDRESS            0x0306 // r/w
#define ROC_NETMASK               0x0307 // r/w
#define ROC_MAC_ADDRESS_UPPER      0x0308 // r/w upper two bytes
#define ROC_MAC_ADDRESS_LOWER      0x0309 // r/w lower four bytes
#define ROC_MASTER_LOGIN          0x030A // w
#define ROC_MASTER_DATAPORT         0x030B // r/w
#define ROC_MASTER_CTRLPORT      0x030C // r/w
#define ROC_BUFFER_FLUSH_TIMER      0x030E // r/w
#define ROC_MASTER_LOGOUT          0x0310 // w
#define ROC_MASTER_IP               0x0311 // r
#define ROC_RESTART_NETWORK         0x0312 // w
#define ROC_CONSOLE_OUTPUT         0x0313 // w
#define ROC_TESTPATTERN_GEN         0x0314 // r/w Turns on/off test pattern generator for throuput test
#define ROC_LOST_ETHER_FRAMES      0x0315 // r ethernet frames lost
#define ROC_UKNOWN_ETHER_FRAMES      0x0316 // r number of ethernet frames with tag other than IP and ARP
#define ROC_SUSPEND_DAQ             0x0319 // w go into suspend state
#define ROC_CLEAR_FILEBUFFER      0x031A // w
#define ROC_CHECK_BITFILEBUFFER      0x031B // r
#define ROC_CHECK_BITFILEFLASH0      0x031C // r
#define ROC_CHECK_BITFILEFLASH1      0x031D // r
#define ROC_CHECK_FILEBUFFER      0x031E // r
#define ROC_TEMAC_PRINT           0x0320 // w - just print some Temac registers and config
#define ROC_TEMAC_REG0             0x0321 // r/w - set or read Remac config register
#define ROC_OVERWRITE_SD_FILE      0x0322 // w
#define ROC_CONSOLE_CMD            0x0323 // w - executes provided console command   
#define ROC_CTRLPORT                  0x0324 // r/w - control port number
#define ROC_DATAPORT                  0x0325 // r/w - data port number

#define ROC_ACTIVATE_LOW_LEVEL      0x0fff   // r/w    Darf man auf Low-Level-Adressen (ab 0x1000) zugreifen? 

//ROC low Level 
#define ROC_LOW_LEVEL_BEGIN         0x1000   //        first address of low-level access

#define ROC_NX_DATA               0x1000   // r    Die Rohdaten aus der nXYTER-FIFO    ACHTUNG: Liest nur eine der 3 FIFOs! 
#define ROC_NX_INIT               0x1010   // w    Soll die Byte-Synchronisation fuer das NX-Wort laufen? 
#define ROC_NX_RESET            0x1014   // w    NX- und LTS-Reset 
#define ROC_LT_LOW               0x1020   // r    Die unteren 32 bit des LTS (muessen zuerst gelesen werden!) 
                                  // ACHTUNG: Liest nur eine der 3 FIFOs! 
#define ROC_LT_HIGH               0x1024   // r    Die oberen 32 bit des LTS (muessen nach LT_LOW gelesen werden!) 
#define ROC_NX_MISS               0x1030   // r    Wieviele Ereignisse konnten nicht in der FIFO gespeichert werden? 
#define ROC_NX_MISS_RESET         0x1034   // w    Den Ereigniscounter reseten. (1 schreiben erzeugt Single Pulse) 
#define ROC_ADC_DATA            0x1100   // r    Die Daten aus der ADC-FIFO 
#define ROC_ADC_ADDR            0x1101   // r    Die Daten aus der ADC-FIFO 
#define ROC_ADC_REG               0x1102   // w    Das ADC-Control-Registers 
                                  // ACHTUNG: Liest nur eine der 3 FIFOs! 
#define ROC_BURST1               0x1200   // r    Die fuer den Burstzugriff zurechtgemachten Daten - Byte 1 
#define ROC_BURST2               0x1204   // r    Die fuer den Burstzugriff zurechtgemachten Daten - Byte 2 
#define ROC_BURST3               0x1208   // r    Die fuer den Burstzugriff zurechtgemachten Daten - Byte 3 
#define ROC_DEBUG_NX            0x1300   // r    Die dem Burstzugriff zugehoerigen Rohdaten aus der NX-FIFO - 48-Bit-Wort 1 
#define ROC_DEBUG_LTL            0x1304   // r    Die dem Burstzugriff zugehoerigen Low-Rohdaten aus der LTS-FIFO - 48-Bit-Wort 1 
#define ROC_DEBUG_LTH            0x1308   // r    Die dem Burstzugriff zugehoerigen High-Rohdaten aus der LTS-FIFO - 48-Bit-Wort 1 
#define ROC_DEBUG_ADC            0x130C   // r    Die dem Burstzugriff zugehoerigen Rohdaten aus der ADC-FIFO - 48-Bit-Wort 1 
#define ROC_DEBUG_NX2            0x1310   // r    Die dem Burstzugriff zugehoerigen Rohdaten aus der NX-FIFO - 48-Bit-Wort 2 
#define ROC_DEBUG_LTL2            0x1314   // r    Die dem Burstzugriff zugehoerigen Low-Rohdaten aus der LTS-FIFO - 48-Bit-Wort 2 
#define ROC_DEBUG_LTH2            0x1318   // r    Die dem Burstzugriff zugehoerigen High-Rohdaten aus der LTS-FIFO - 48-Bit-Wort 2 
#define ROC_DEBUG_ADC2            0x131C   // r    Die dem Burstzugriff zugehoerigen Rohdaten aus der ADC-FIFO - 48-Bit-Wort 2 
#define ROC_I2C_DATA            0x2000   // r/w    Das I2C-Datenwort 
#define ROC_I2C_RESET            0x2004   // w    Der Reset der I2C-FSM 
#define ROC_I2C_REGRESET         0x2008   // w    Der Reset der I2C-Register 
#define ROC_I2C_REGISTER         0x2010   // w    Das zu lesende oder zu schreibende I2C-Register 
#define ROC_OCM_REG1            0x3011   // r    Die fuer den Burstzugriff zurechtgemachten Daten - Byte 1 
#define ROC_OCM_REG2            0x3012   // r    Die fuer den Burstzugriff zurechtgemachten Daten - Byte 2 
#define ROC_OCM_REG3            0x3013   // r    Die fuer den Burstzugriff zurechtgemachten Daten - Byte 3 
#define ROC_SHIFT               0x4000   // w    Setzt den Init-Wert des shift-Registers und den BUFMUX so, dass ihr Inhalt einem verschieben um den Ã¼bergebenen Wert entspricht. 
                                  // ACHTUNG: Damit der neue Delay-Wert genutzt wird, muss ein System-Reset gezogen werden. 
#define ROC_SR_INIT               0x4002   // r/w    Der Init-Wert des Shift-Registers fuer die ADC-Clock 
#define ROC_BUFG_SELECT            0x4003   // r/w    Der Wert des Selectbits fuer den BUFGMUX 
#define ROC_ADC_LATENCY            0x4004   // r/w    Die Zahl der Takte, die die ADC-Daten den NX-Daten hinterherhinken 
#define ROC_DO_AUTO_LATENCY         0x4010   // w    Bestimmt beim Schreiben einer 1 automatisch die Latency und die richtigen Werte fÃ¼r den BUFMUX und das Shift-Register. 
                                  // ACHTUNG: Dies verÃ¤ndert die I2C-Register! 
#define ROC_DO_AUTO_DELAY         0x4011   // w    Bestimmt beim Schreiben einer 1 die Verzoegerung zwischen ROC und NX und setzt den delay so, dass die TS synchron sind. 
                                  // ACHTUNG: Diese Funktion veraendert die I2C-Register! 
#define ROC_DO_TESTSETUP         0x4012   // w    Setzt die I2C-Register so, dass ein Testpuls moeglich ist.
   
#define ROC_DELAY_LTS            0x4100   // r/w
#define ROC_DELAY_NX0            0x4101   // r/w
#define ROC_DELAY_NX1            0x4102   // r/w
#define ROC_DELAY_NX2            0x4103   // r/w
#define ROC_DELAY_NX3            0x4104   // r/w

#define ROC_LOW_LEVEL_END         0x5000   //        last address of low-level access

#define ROC_FBUF_START            0x10000   //       first address of file buffer
#define ROC_FBUF_SIZE              0x400000  //       size of file buffer (4MB)


/************************************
 * address space finished
 ***********************************/

// ports number of ROC itself
#define ROC_DEFAULT_CTRLPORT                        13132
#define ROC_DEFAULT_DATAPORT                        13131

//ROC TAGS
#define ROC_PEEK                     111
#define ROC_POKE                     112
#define ROC_CONSOLE                     113

//STATUS FLAGS
#define SCP_STATUS_ERROR               0
#define SCP_STATUS_SUCCESS               1
#define SCP_STATUS_MESSAGE               2
#define SCP_STATUS_ARP_PACKET            3

//ROCRETURN addresses
#define ROC_RETURN_ACKNOWLEDGMENT         0
#define ROC_RETURN_PEEK_RETURN            1


#endif /*DEFINES_H_*/
