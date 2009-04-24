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

#ifndef ROC_UdpBoard
#define ROC_UdpBoard

#ifndef ROC_Board
#include "roc/Board.h"
#endif

namespace roc {


   #define UDP_PAYLOAD_OFFSET 42
   #define MAX_UDP_PAYLOAD 1472
   #define MESSAGES_PER_PACKET 243

#pragma pack(push, 1)

   extern const char* xmlTransferWindow;


   struct UdpMessage
   {
      uint8_t  tag;
      uint8_t  pad[3];
      uint32_t password;
      uint32_t id;
      uint32_t address;
      uint32_t value;
   };

   struct UdpMessageFull : public UdpMessage
   {
      uint8_t rawdata[MAX_UDP_PAYLOAD - sizeof(UdpMessage)];
   };

   struct UdpDataRequest
   {
      uint32_t password;
      uint32_t reqpktid;
      uint32_t frontpktid;
      uint32_t tailpktid;
      uint32_t numresend;
   };

   struct UdpDataRequestFull : public UdpDataRequest
   {
      uint32_t resend[(MAX_UDP_PAYLOAD - sizeof(struct UdpDataRequest)) / 4];
   };

   struct UdpDataPacket
   {
      uint32_t pktid;
      uint32_t lastreqid;
      uint32_t nummsg;
   // here all messages should follow
   };

   struct UdpDataPacketFull : public UdpDataPacket
   {
      uint8_t msgs[MAX_UDP_PAYLOAD - sizeof(struct UdpDataPacket)];
   };

   struct UdpStatistic {
      uint32_t   dataRate;   // data taking rate in  B/s
      uint32_t   sendRate;   // network send rate in B/s
      uint32_t   recvRate;   // network recv rate in B/s
      uint32_t   nopRate;    // double-NOP messages 1/s
      uint32_t   frameRate;  // unrecognized frames 1/s
      uint32_t   takePerf;   // relative use of time for data taking (*100000)
      uint32_t   dispPerf;   // relative use of time for packets dispatching (*100000)
      uint32_t   sendPerf;   // relative use of time for data sending (*100000)
   };


   /** ISE Binfile header
    * A bitfile from Xilinx ISE consists of a binfile and a fileheader.
    * We need only a bin file for reprogramming the virtex. So
    * we use this header in front of the bin file
    * to store it on the Actel Flash. Its size is 512 bytes.
    * Behind the Header, the binfile will be written to the flash */

   union ISEBinfileHeader {
      struct {
         uint8_t ident[4];
         uint32_t headerSize;
         uint32_t binfileSize;
         uint8_t XORCheckSum;
         uint8_t bitfileName[65];
         uint32_t timestamp;
      };
      uint8_t bytes[512];
   };

#pragma pack(pop)

   class UdpBoard : public roc::Board {

      protected:

         UdpBoard();
         virtual ~UdpBoard();

      public:

         /** Send console command
          * Via this function you can send remote console commands.
          * It works like telnet.
          * It is developed mainly for debugging reasons. */
         virtual bool sendConsoleCommand(const char* cmd) = 0;

         /** Upload bit file to ROC
          * Uploads bitfile to specified position (0 or 1)
          * Returns true if upload was successful. */
         virtual bool uploadBitfile(const char* filename, int position) = 0;

         virtual bool uploadSDfile(const char* filename, const char* sdfilename = 0) = 0;

         virtual bool saveConfig(const char* filename = 0) = 0;
         virtual bool loadConfig(const char* filename = 0) = 0;


   };

}

#endif
