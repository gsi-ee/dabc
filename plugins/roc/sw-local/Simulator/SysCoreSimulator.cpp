#include "SysCoreSimulator.h"

#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/timex.h>

struct sc_config gC;

struct sc_data gD;

double get_clock()
{
   static double firsttm = 0.;

   ntptimeval val;
   ntp_gettime(&val);

   double tm = val.time.tv_sec + val.time.tv_usec * 1e-6;

   if (firsttm == 0.) firsttm = tm;

   return tm - firsttm;
}

/*****************************************************************************
 * Implementations
 ****************************************************************************/

/*****************************************************************************
 * Sockets
 *****************************************************************************/

ScSimuControlSocket::ScSimuControlSocket(ISocketHandler& h, SysCoreSimulator* controler) : UdpSocket(h)
{
   controler_ = controler;
}

void ScSimuControlSocket::OnRawData(const char *p,size_t l,struct sockaddr *sa_from,socklen_t sa_len)
{
   if (sa_len != sizeof(struct sockaddr_in)) return;// IPv4

   if (controler_->doRecvLost()) return;
      
   struct sockaddr_in sa;
   memcpy(&sa, sa_from, sa_len);
   ipaddr_t src_ip;
   memcpy(&src_ip, &sa.sin_addr, 4);
   port_t src_port = ntohs(sa.sin_port);
   
   SysCore_Message_Packet_Full pkt;

   memcpy(&pkt, p, l);
   
   if (ntohl(pkt.password) != ROC_PASSWORD) return;

   gD.lastMasterTime = get_clock();
   
   pkt.address = ntohl(pkt.address);
   pkt.value = ntohl(pkt.value);
   
   if ((pkt.tag == ROC_POKE) && 
       (pkt.address == ROC_MASTER_LOGIN)) {
      
      if (gD.masterConnected) {
         printf("Master already connected\n");
         return;
      }
      
      gD.masterIp = src_ip;
      gD.masterControlPort = src_port;
      
      gD.masterConnected = true;
      
      std::string ipA;
      Utility::l2ip(src_ip,ipA);
      printf("Master is %s : %u\n", ipA.c_str(), src_port);
   }
   
   gD.masterConnected = true;
                  
   switch(pkt.tag) {
      
      case ROC_PEEK:
         pkt.address = controler_->peek(pkt.address, &pkt.value);
         pkt.address = htonl(pkt.address);
         pkt.value = htonl(pkt.value);

         if (!controler_->doSendLost())
             controler_->cmdSocket_.SendToBuf(src_ip, src_port,
                       (char*)&pkt, sizeof(SysCore_Message_Packet));
         break;
         
      case ROC_POKE:
         pkt.value = controler_->poke(pkt.address, pkt.value, pkt.rawdata);
         pkt.value = htonl(pkt.value);
         pkt.address = htonl(pkt.address);
                           
         if (!controler_->doSendLost())
            controler_->cmdSocket_.SendToBuf(src_ip, src_port,
                      (char*)&pkt,  sizeof(SysCore_Message_Packet));
         break;
         
      default:
         break;
   }
}

ScSimuDataSocket::ScSimuDataSocket(ISocketHandler& h, SysCoreSimulator* controler)   :UdpSocket(h)                                       
{
   controler_ = controler;
}

void ScSimuDataSocket::OnRawData(const char *p,size_t l,struct sockaddr *sa_from,socklen_t sa_len)
{
   if (sa_len != sizeof(struct sockaddr_in)) return;// IPv4

   if (controler_->doRecvLost()) return;

   SysCore_Data_Request_Full pkt;
   
   memcpy(&pkt, p, l);
   
   if (ntohl(pkt.password) != ROC_PASSWORD) return;
   
   gD.lastMasterTime = get_clock();

   pkt.reqpktid = ntohl(pkt.reqpktid);
   pkt.frontpktid = ntohl(pkt.frontpktid);
   pkt.tailpktid = ntohl(pkt.tailpktid);
   pkt.numresend = ntohl(pkt.numresend);

   for (unsigned n=0; n<pkt.numresend; n++)
      pkt.resend[n] = ntohl(pkt.resend[n]);
      
   controler_->processDataRequest(&pkt);
}

/*****************************************************************************
 * SysCoreSimulator
 ****************************************************************************/

SysCoreSimulator::SysCoreSimulator(int rocid, int simtype) :
   socketHandler_(&stOutLog_),//socketHandlerMutex_, &stOutLog_),
   dataSocket_(socketHandler_, this),
   cmdSocket_(socketHandler_, this)
{
   
   simulationType_ = simtype;
   
   printf("Starting ROC Simulator...\n");
   memset(nxRegister,1,sizeof(nxRegister));

   syncMDelay_ = 8;
   simulationType_ = 1;
   
   resetSimulator();
   
   setLostRates(0., 0.);

   srand(3442143);
   
   gC.dataPort = ROC_DEFAULT_DATAPORT;
   gC.ctrlPort = ROC_DEFAULT_CTRLPORT;
   gC.rocNumber = rocid;
   gC.flushTimeout = 1000;
   
   gD.high_water = 2000;   /* Maximum number of buffers to be used in normal situation */
   gD.low_water = 50;      /* Number of buffers to enable data taking after is was stopped */
   
   gD.headid = 0;         // id of buffer which is now filled 
   gD.send_limit = 0;    /* limit for sending - can be in front of existing header */
   gD.resend_size = 0;   /* number of indexes to be resended */
   gD.last_req_pkt_id = 0; /* id of last request packet, should be always send back */
   
   gD.masterConnected = false; // 1 if master controller is connected
   gD.masterDataPort = 0;
   gD.masterControlPort = 0;
   gD.masterIp = 0;
   
   gD.daqState = 0;
   gD.data_taking_on = false;
   gD.lastMasterTime = get_clock();
   
}

SysCoreSimulator::~SysCoreSimulator()
{
   //readout network fifo
   int i = 10;
   while(i--) socketHandler_.Select(0,100);
   socketHandler_.RegStdLog(0);
   cmdSocket_.CloseAndDelete();
   dataSocket_.CloseAndDelete();
}

void SysCoreSimulator::resetSimulator()
{
   nxTimestamp = 0;
   nxEpoch = 0;
   makeEpochMarker = true;
   
   
   // start with epoch wich has 24 bit already filled
//   uint64_t first_stamp = (0x1000000ULL << 14); 
   uint64_t first_stamp = 0;

   nextEventNumber = 0;
   clockScrewUp = 1. + gC.rocNumber / 100.;
   nextTimeHit = first_stamp + 5674;         
   nextHitCounter = 1;                 // generate only one hit for first event
   nextTimeNoise = first_stamp + 3456; // indicate when time stamp message should be
   nextTimeSync = first_stamp + 0x345; // indicate when next time sync message should appear
   nextTimeAUX = first_stamp + 0x500;  // first AUX signal should be after first sync
   lastEpoch = (first_stamp >> 14) - 1;    // last epoch set in data stream
   nextSyncEvent = 0;
}


bool SysCoreSimulator::doRecvLost()
{
   totalRecvCounter++; 

   if ((recvLostRate <=0.) || (recvLostRate >=1)) return false; 
   
   if (recvLostCounter>0) {
      --recvLostCounter;
      if (recvLostCounter==0) totalRecvLosts++;
      return (recvLostCounter==0);
   }
   
   recvLostCounter = 1 + int (2. * rand() / RAND_MAX / recvLostRate);
   
   return false;
}


bool SysCoreSimulator::doSendLost()
{
   totalSendCounter++; 

   if ((sendLostRate <=0.) || (sendLostRate >=1)) return false; 
   
   if (sendLostCounter>0) {
      sendLostCounter--;
      if (sendLostCounter==0) totalSendLosts++;
      return sendLostCounter==0;
   }
   
   sendLostCounter = 1 + int (2. * rand() / RAND_MAX / sendLostRate);
   
   return false;
}

void SysCoreSimulator::setLostRates(double recv, double send)
{
   recvLostRate = recv; // relative loss rate of packets recieve 
   sendLostRate = send; // relative loss rate of packets send
    
   recvLostCounter = 0; // backward counter to next recv lost 
   sendLostCounter = 0; // backward counter to next send lost 

   totalSendCounter = 0; totalSendLosts = 0;
   totalRecvCounter = 0; totalRecvLosts = 0;
}

  
bool SysCoreSimulator::initSockets()
{
   //cmd Socket
   if (cmdSocket_.Bind(gC.ctrlPort, 1) != 0) {
      printf("Command Socket Bind failed for Port %d.\n", gC.ctrlPort);
      return false;
   }

   printf("Simulator control port is %d\n", gC.ctrlPort);
   socketHandler_.Add(&cmdSocket_);
   
   //data Socket
   if (dataSocket_.Bind(gC.dataPort, 1) != 0) {
      printf("Data Socket Bind failed for Port %d.\n", gC.dataPort);
      return false;
   }
   printf("Simulator data port is %d\n", gC.dataPort);
   socketHandler_.Add(&dataSocket_);
   return true;
}


uint16_t SysCoreSimulator::peek (uint32_t addr, uint32_t *retVal)
{  
//   printf("SysCoreSimulator peek at %d\n", addr);
    
   switch (addr) {
      case ROC_SOFTWARE_VERSION:
         *retVal = ROC_SIMULATOR_SW_VERSION;
         return 0;
      case ROC_HARDWARE_VERSION:
         *retVal = ROC_SIMULATOR_HW_VERSION;
         return 0;
      case ROC_NUMBER:
         *retVal = gC.rocNumber;
          return 0;
      case ROC_DATAPORT:
         *retVal = gC.dataPort;
          return 0;
      default:   
         if ((addr>=0x0100)&&(addr<=0x012D)) {
            *retVal =  nxRegister[addr-0x100];
            return 0;
         }
   } 
   
   return 2;
}

uint16_t SysCoreSimulator::poke(uint32_t addr, uint32_t val, uint8_t* rawdata)
{  
   printf("Roc:%u Poke 0x%04x = 0x%04x\n", gC.rocNumber, addr, val);
   
   switch (addr) {
      case ROC_START_DAQ:
         printf("Roc:%u Starting DAQ ...\n", gC.rocNumber);
         gD.buf.clear();
         gD.send_limit = 0;
         gD.headid = 0;
         gD.resend_size = 0;   
         gD.last_req_pkt_id = 0; 
         gD.next_send_id = 0;
         gD.data_taking_on = true;
         gD.lastFlushTime = get_clock();
         gD.droppkt_id = gD.headid; // drop start message
         //gD.droppkt_id = 0 - 1;
         makeSysMessage(1);
         gD.daqState = 1;
         return 0;
      case ROC_SUSPEND_DAQ:
         printf("Roc:%u DAQ suspended.\n", gC.rocNumber);
         gD.droppkt_id = gD.headid; // drop stop message
         // gD.droppkt_id = 0 - 1;
         printf("Try to drop packet 0x%x with stop message\n",gD.droppkt_id);
         makeSysMessage(2);
         gD.daqState = 2;
         gD.data_taking_on = false;
         return 0;
      case ROC_STOP_DAQ:
         printf("Roc:%u DAQ stopped.\n", gC.rocNumber);
         gD.daqState = 0;
         gD.buf.clear();
         gD.send_limit = 0;
         gD.headid = 0;
         gD.data_taking_on = false;
         return 0;
      case ROC_NUMBER:
         printf("Set ROC id to %d.\n",val);
         gC.rocNumber = val;
         return 0;
      case ROC_MASTER_LOGIN:
         printf("Master login & simulator reset.\n");
         resetSimulator();
         return 0;   

      case ROC_MASTER_LOGOUT:
         printf("Master is logout.\n");
         gD.masterConnected = false;
         totalSendLosts = 0;
         totalSendCounter = 0;
         totalRecvLosts = 0;
         totalRecvCounter = 0;
         return 0;   
         
      case ROC_MASTER_DATAPORT:
         gD.masterDataPort = val;     
         printf("Set Target data port to (%d).\n",val);
         return 0;
      case ROC_SYNC_M_SCALEDOWN:
         syncMDelay_ = 1 << val; 
         if (syncMDelay_==0) syncMDelay_ = 8;
         printf("Set sync delay = %u, step = %u.\n", val, syncMDelay_); 
         return 0;
      case ROC_FIFO_RESET:
         return 0;   
      case ROC_BUFFER_FLUSH_TIMER:
         gC.flushTimeout = val;
         return 0; 
      case ROC_CONSOLE_OUTPUT:
         return 0;   
      case ROC_CONSOLE_CMD:
         printf("Exectute console command %s.\n", rawdata);
         return 0;   
      case ROC_NX_SELECT:
      case ROC_NX_ACTIVE:
      case ROC_AUX_ACTIVE:
      case ROC_BURST:
         return 0;
         
      default:   
        if ((addr>=0x0100)&&(addr<=0x012D)) {
           nxRegister[addr-0x100] = val;
           return 0;
        }
   }

   return 2;
}

void SysCoreSimulator::fillRareSyncData(uint8_t* target, uint16_t numOfDatas)
{
   uint32_t nextepoch = 0;
   
   uint64_t min = 0;
   
   while (numOfDatas > 0) {
       
      min = nextTimeNoise; 
      if (min > nextTimeSync) min = nextTimeSync;
      if (min > nextTimeAUX) min = nextTimeAUX;
      if (min > nextTimeHit) min = nextTimeHit;

      nextepoch = min >> 14;
         
      if (nextepoch != lastEpoch) {
         target[0] = ((0x2 << 5) | (gC.rocNumber << 2) | 0)  & 0xFF;//msg type, roc#, unused
         target[1] = (nextepoch >> 24) & 0xFF;
         target[2] = (nextepoch >> 16) & 0xFF;
         target[3] = (nextepoch >> 8) & 0xFF;
         target[4] = (nextepoch >> 0) & 0xFF;
         target[5] = 0;
         lastEpoch = nextepoch;
      } else
      
      if (nextTimeHit == min) {
        // random channel, random spectrum:      
        uint16_t channel = rand() % 128;
        uint16_t nxyter =  rand() % 4;
        uint16_t adcvalue = rand() % 0x4000;

        uint16_t timestamp = nextTimeHit & 0x3fff;
        bool lastepoch = false;
       
         //pseudo hit data
        target[0] = ((0x1 << 5) | (gC.rocNumber << 2) | nxyter)  & 0xFF;//msg type, roc#, nx#0
        target[1] = (timestamp >> 9) & 0xFF;
        target[2] = (timestamp >> 1) & 0xFF;
        target[3] = ((timestamp << 7) | (channel & 0x7F)) & 0xFF;
        target[4] = (adcvalue >> 5) & 0xFF;
        target[5] = ((adcvalue << 3) & 0xFF) | (lastepoch ? 1 : 0);//last Epoch   
        
        nextHitCounter--;
        if (nextHitCounter == 0) {
           nextEventNumber++;
           nextHitCounter = 3 + (rand() % 6); // produce between 3 and 8 hits per event
           nextTimeHit = uint64_t(nextEventNumber * 73747 * clockScrewUp); // produce event every 73.747 microsec or about 13.5 KHz
        } 
        nextTimeHit += (1 + rand() % 7); // maximum event duration 8hits * 7 nanosec = 56 nanosec
      } else
      
      if (nextTimeNoise == min ) {
        // random channel, random spectrum:      
        uint16_t channel = rand() % 128;
        uint16_t nxyter =  rand() % 4;
        uint16_t adcvalue = rand() % 0x4000;     
        
        uint16_t timestamp = nextTimeNoise & 0x3fff;
        bool lastepoch = false;
       
         //pseudo hit data
        target[0] = ((0x1 << 5) | (gC.rocNumber << 2) | nxyter)  & 0xFF;//msg type, roc#, nx#0
        target[1] = (timestamp >> 9) & 0xFF;
        target[2] = (timestamp >> 1) & 0xFF;
        target[3] = ((timestamp << 7) | (channel & 0x7F)) & 0xFF;
        target[4] = (adcvalue >> 5) & 0xFF;
        target[5] = ((adcvalue << 3) & 0xFF) | (lastepoch ? 1 : 0);//last Epoch   
        
        nextTimeNoise += (rand() % 0x4000); // about 2 noise hits per epoch
      } else

      if (nextTimeSync == min) {
         uint16_t timestamp = nextTimeSync & 0x3fff;
         uint32_t evnt = nextSyncEvent & 0xffffffL;

         target[0] = ((0x3 << 5) | (gC.rocNumber << 2) | 0x0)  & 0xFF;//msg type, roc#, channel
//         target[1] = (timestamp >> 6);
//         target[2] = ((timestamp << 2) & 0xFC) | ((evnt >> 22) & 0x3);
         target[1] = ((nextepoch & 0x1) << 7) | ((timestamp >> 7) & 0x7F); // epoch low bit plus time stamp
         target[2] = ((timestamp << 1) & 0xFC) | ((evnt >> 22) & 0x3);  
         target[3] = (evnt >> 14) & 0xFF;
         target[4] = (evnt >> 6) & 0xFF;
         target[5] = ((evnt << 2) | 0x0) & 0xFF; 
         
         // every n epochs new time sync, period depends from id
         // as result, all other times must be suppressed  
         nextSyncEvent += syncMDelay_;
         nextTimeSync = uint64_t(0x345 + nextSyncEvent * 0x4000 * clockScrewUp);
      } else

      if (nextTimeAUX == min) {

         uint8_t naux = 0; // 7 bit
         uint16_t timestamp = nextTimeAUX & 0x3fff;
         uint8_t failing = 1;

         target[0] = ((0x4 << 5) | (gC.rocNumber << 2) | (naux >> 5))  & 0xFF;//msg type, roc#, naux

         target[1] = (naux << 3) | ((nextepoch & 0x1) << 2) | ((timestamp >> 12) & 0x3);  // naux, epoch low bit, timestamp 2 bit
         target[2] = (timestamp >> 4) & 0xFF;  // timestamp
         target[3] = ((timestamp << 4) & 0xE0) | (failing << 4);  // timestamp, last bit of time stamp fail out

//         target[1] = ((naux << 3) | (timestamp >> 11)) & 0xFF;  // naux and timestamp
//         target[2] = (timestamp >> 3) & 0xFF;  // timestamp
//         target[3] = ((timestamp << 5) | (failing<<4)) & 0xFF;  // timestamp

         target[4] = 0;
         target[5] = 0;

         nextTimeAUX += 20000; // every 20 miscroseconds or 1.22 epochs (without time correction)
      }
      
      numOfDatas--;
      target+=6;
   } 
}


void SysCoreSimulator::fillDataJoern (uint8_t* target, uint16_t numOfDatas)
{
   static unsigned int synccounter=0;
      
   for(int i = 0;i < numOfDatas; i++)
   {
      //if(((nxTimestamp + 14) >> 14) > 0) //new epoch + 2 last Epoch events
      if(makeEpochMarker)
      {
         //new epoch
          if(i+4 >= numOfDatas)
            { 
               while(i<numOfDatas)
               {
                  // fill with dummy data to be sure..
                  target[i*6]=0;
                  target[i*6+1] =0;
                  target[i*6+2] = 0;
                  target[i*6+3] = 0;
                  target[i*6+4] = 0;
                  target[i*6+5] = 0;                   
                  i++; 
               }
             break; // epoch marker, syncmarker and last event data do not fit anymore into packet: do in next packet!
            }
         nxEpoch++;
         makeEpochMarker=false;
          //send epoch marker
         target[i*6] = ((0x2 << 5) | (gC.rocNumber << 2) | 0)  & 0xFF;//msg type, roc#, unused
         target[i*6+1] = (nxEpoch >> 24) & 0xFF;
         target[i*6+2] = (nxEpoch >> 16) & 0xFF;
         target[i*6+3] = (nxEpoch >> 8) & 0xFF;
         target[i*6+4] = (nxEpoch >> 0) & 0xFF;
         target[i*6+5] = 0;//missedEvents & 0xFF;
                  
         
         
         // test if we shall send sync marker (every 4 epochs)
         if((synccounter%4)==0)
            {
                 nextSyncEvent=nxEpoch & 0xFFFFFFL;
                 i++;                 
                 target[i*6] = ((0x3 << 5) | (gC.rocNumber << 2) | 0x0)  & 0xFF;//msg type, roc#, channel
                 target[i*6+1] = ((nxEpoch & 0x1) << 7) | ((nxTimestamp >> 7) & 0x7F); // epoch low bit plus time stamp
                 target[i*6+2] = ((nxTimestamp << 1) & 0xFC) | ((nextSyncEvent >> 22) & 0x3);  
                 target[i*6+3] = (nextSyncEvent >> 14) & 0xFF;
                 target[i*6+4] = (nextSyncEvent >> 6) & 0xFF;
                 target[i*6+5] = ((nextSyncEvent << 2) | 0x0) & 0xFF; 
                 nxTimestamp+=7; // for next adc hit  
         
         
         
         
            }
         synccounter++;
         //2*lastEpoch if there is space in the packet
         if(i + 2 < numOfDatas)
         {
            for(int j = 0; j < 2; j++)
            {
               i++;                  
               fillAdcValue(target+6*i, true);
               
         
               // todo: random increment of timestamp with gaussian maximum in middle of epoch...       
               if(nxTimestamp<9000)
                  nextTimestamp(7000,400);       
               else
                  nextTimestamp(12000,200);   
               //nxTimestamp += 7;               
            }   
         }
         else
         {
            //nxTimestamp += 14;
         }
         //nxTimestamp = nxTimestamp & 0x3FFF;
      }
      else 
      {   
         
         fillAdcValue(target+i*6, false);
         
         
      
         // todo: random increment of timestamp with gaussian maximum in middle of epoch...       
          if(nxTimestamp<9000)
            nextTimestamp(7000,400);       
          else
            nextTimestamp(12000,100);   
         
         
         
         //nxTimestamp += 7;
      }
   }
   //everything between epochmarkers get a bit scrambled
   //like the real SysCore does :-)
   // one scramble per packet

///// test: how does timestamp look without scrambling?   
//   int start = 0;
//   int end = numOfDatas - 1;
//   SysCoreData data;
//   //search for end
//   for(int i = 0;i < numOfDatas; i++)
//   {
//      data.assignData(target + (i*6));
//      if(data.getMessageType() == 2)
//      {
//         end = i - 1;
//         break;
//      }
//   }
//   scramble(target, start, end);
               
}

void SysCoreSimulator::fillData (uint8_t* target, uint16_t numOfDatas)
{
   uint16_t nxChannelId = 1;
   int adcValue;
   const double pi = 3.14159265;
      
   for(int i = 0;i < numOfDatas; i++)
   {
      if(((nxTimestamp + 14) >> 14) > 0) //new epoch + 2 last Epoch events
      {
         //new epoch
         nxEpoch++;
         
         //send epoch marker
         target[i*6] = ((0x2 << 5) | (gC.rocNumber << 2) | 0)  & 0xFF;//msg type, roc#, unused
         target[i*6+1] = (nxEpoch >> 24) & 0xFF;
         target[i*6+2] = (nxEpoch >> 16) & 0xFF;
         target[i*6+3] = (nxEpoch >> 8) & 0xFF;
         target[i*6+4] = (nxEpoch >> 0) & 0xFF;
         target[i*6+5] = 0;//missedEvents & 0xFF;
         
         //2*lastEpoch if there is space in the packet
         if(i + 2 < numOfDatas)
         {
            for(int j = 0; j < 2; j++)
            {
               i++;
               adcValue = (int)floor(sin(nxTimestamp * 2 * pi / 0x3FFF) * 0x7FF);
               //send pseudo hit data
               target[i*6] = ((0x1 << 5) | (gC.rocNumber << 2) | 0)  & 0xFF;//msg type, roc#, nx#0
               target[i*6+1] = (nxTimestamp >> 9) & 0xFF;
               target[i*6+2] = (nxTimestamp >> 1) & 0xFF;
               target[i*6+3] = ((nxTimestamp << 7) | (nxChannelId & 0x7F)) & 0xFF;
               target[i*6+4] = (adcValue >> 5) & 0xFF;
               target[i*6+5] = ((adcValue << 3) & 0xFF) | 1;//last Epoch
                      
               nxTimestamp += 7;               
            }   
         }
         else
         {
            nxTimestamp += 14;
         }
         nxTimestamp = nxTimestamp & 0x3FFF;
      }
      else 
      {   
         adcValue = (int)floor(sin(nxTimestamp * 2 * pi / 0x3FFF) * 0x7FF);
         //send pseudo hit data
         target[i*6] = ((0x1 << 5) | (gC.rocNumber << 2) | 0)  & 0xFF;//msg type, roc#, nx#0
         target[i*6+1] = (nxTimestamp >> 9) & 0xFF;
         target[i*6+2] = (nxTimestamp >> 1) & 0xFF;
         target[i*6+3] = ((nxTimestamp << 7) | (nxChannelId & 0x7F)) & 0xFF;
         target[i*6+4] = (adcValue >> 5) & 0xFF;
         target[i*6+5] = (adcValue << 3) & 0xFF;
      
         nxTimestamp += 7;
      }
   }
   //everything between epoch markers get a bit scrambled
   //like the real SysCore does :-)
   // one scramble per packet
   
   int start = 0;
   int end = numOfDatas - 1;
   //search for end
   for(int i = 0;i < numOfDatas; i++)
   {
        if (((target[i*6] >> 5) & 0x7) == 2)
      {
         end = i - 1;
         break;
      }
   }
   scramble(target, start, end);
}

void SysCoreSimulator::scramble(uint8_t* target, int start, int end)
{
   if(end - start >= 6)
   {
      int temp;
      for(int i = 0; i < end - start; i++)
      {
         temp = (rand() % (end - start) + start) * 6;
         target[(start + i)*6+0] = target[temp+0];
         target[(start + i)*6+1] = target[temp+1];
         target[(start + i)*6+2] = target[temp+2];
         target[(start + i)*6+3] = target[temp+3];
         target[(start + i)*6+4] = target[temp+4];
         target[(start + i)*6+5] = target[temp+5];         
      }
   }
}

void SysCoreSimulator::fillAdcValue(uint8_t* target, bool lastepoch)
{
 const double pi = 3.14159265;
 
 static uint16_t channel=0;
 static uint16_t nxyter=1;
 static uint16_t argument=0;
 
 // random channel, random spectrum:      
 channel= rand() % 128;
 nxyter=  rand() % 3;
 argument = rand() % 0x4000;     

// spectrum linked to timestamp: 
 //argument=nxTimestamp; // later random number here  
// channel=(nxTimestamp % 0x7F); // produce some data in all channels!
// nxyter++;
// if(nxyter==3) nxyter=0;

 //printf("filladc: channel(%d), nxyter(%d), argument(%d)...\n",channel,nxyter,argument);
   
  int adcValue = (int)floor(sin(argument * 2 * pi / 0x3FFF) * 0x7FF);
         //pseudo hit data
   target[0] = ((0x1 << 5) | (gC.rocNumber << 2) | nxyter)  & 0xFF;//msg type, roc#, nx#0
   target[1] = (nxTimestamp >> 9) & 0xFF;
   target[2] = (nxTimestamp >> 1) & 0xFF;
   target[3] = ((nxTimestamp << 7) | (channel & 0x7F)) & 0xFF;
   target[4] = (adcValue >> 5) & 0xFF;
   if(lastepoch)
      target[5] = ((adcValue << 3) & 0xFF) | 1;//last Epoch  
   else
      target[5] = (adcValue << 3) & 0xFF;
  
   
}

void SysCoreSimulator::nextTimestamp(uint16_t peak, uint16_t width)
{
const double pi = 3.14159265;   
uint16_t increment=1;
uint16_t maxincrement=1000;// 500
//uint16_t maxincrement=width;// 500
//if(maxincrement<200) maxincrement=200;
// the closer to peak, the smaller the increment:
double x= (double) (nxTimestamp) / (double) (0x3FFF);
double  mu=(double)(peak) / (double)(0x3FFF);
double sig= (double)(width) / (double)(0x3FFF);
if(sig==0) sig=1;
double max=1 / sqrt(2*pi) / sig;
double gaus= max*exp( - 0.5* (x-mu) * (x-mu) / sig*sig);   
increment= 1 + (uint16_t) ((max-gaus)/max * maxincrement);
//printf("nextTimestamp: x(%e), mu(%e), sig(%e), gaus(%e), increment(%d), nxTimestamp(%d)...\n",x, mu, sig, gaus,increment,nxTimestamp);

// test if we need new epoch marker before next hits: 
if(((nxTimestamp + increment*7) >> 14) > 0)
   {
      makeEpochMarker=true;            
   }    
nxTimestamp+=increment*7;   
nxTimestamp = nxTimestamp & 0x3FFF;   


}

void SysCoreSimulator::fillDetectorHitData (uint8_t* target, uint16_t numOfDatas)
{
 static unsigned int synccounter=0;
   for(int i = 0;i < numOfDatas; i++)
   {
      //if(((nxTimestamp + 14) >> 14) > 0) //new epoch + 2 last Epoch events
      if(makeEpochMarker)
      {
         //new epoch
          if(i+2 >= numOfDatas)
            { 
               while(i<numOfDatas)
               {
                  // fill with dummy data to be sure..
                  target[i*6]=0;
                  target[i*6+1] =0;
                  target[i*6+2] = 0;
                  target[i*6+3] = 0;
                  target[i*6+4] = 0;
                  target[i*6+5] = 0;                   
                  i++; 
               }
             break; // epoch marker, syncmarker and last event data do not fit anymore into packet: do in next packet!
            }
         nxEpoch++;
         makeEpochMarker=false;
          //send epoch marker
         target[i*6] = ((0x2 << 5) | (gC.rocNumber << 2) | 0)  & 0xFF;//msg type, roc#, unused
         target[i*6+1] = (nxEpoch >> 24) & 0xFF;
         target[i*6+2] = (nxEpoch >> 16) & 0xFF;
         target[i*6+3] = (nxEpoch >> 8) & 0xFF;
         target[i*6+4] = (nxEpoch >> 0) & 0xFF;
         target[i*6+5] = 0;//missedEvents & 0xFF;
         
         
         
         // test if we shall send sync marker (every 8 epochs)
         if((synccounter%4)==0)
            {
                 
                 nextSyncEvent=nxEpoch & 0xFFFFFFL;
                 nxTimestamp=0x1111;
                 i++;                                      
                 target[i*6] = ((0x3 << 5) | (gC.rocNumber << 2) | 0x0)  & 0xFF;//msg type, roc#, channel
                 target[i*6+1] = ((nxEpoch & 0x1) << 7) | ((nxTimestamp >> 7) & 0x7F); // epoch low bit plus time stamp
                 target[i*6+2] = ((nxTimestamp << 1) & 0xFC) | ((nextSyncEvent >> 22) & 0x3);  
                 target[i*6+3] = (nextSyncEvent >> 14) & 0xFF;
                 target[i*6+4] = (nextSyncEvent >> 6) & 0xFF;
                 target[i*6+5] = ((nextSyncEvent << 2) | 0x0) & 0xFF; 
                 nextHitTimestamp();
            }
         synccounter++;
 
      }
      else 
      {   
         //if((rand() % 10) ==0)
         if((i % 7) ==0)
            {              
               int numhits=15;
               if(numOfDatas-i < numhits)
                  numhits=numOfDatas-i;
               fillHitValues(target+i*6, numhits, false);
               nextHitTimestamp();
               
            }
         else
            {
               makeEpochMarker=true; // empty epoche               
            }
            
         //nxTimestamp += 7;
      }
   }
   
   
}
  

void SysCoreSimulator::fillHitValues(uint8_t* target, int numhits, bool lastepoch)
{
 const double pi = 3.14159265;
 double mu=1500;
 double sig=100;
 //int numhits=10;
 
 // random channel, random spectrum:
 for(int i=0;  i<numhits; ++i)
 {      
    uint16_t channel= rand() % 128;
    uint16_t nxyter=  rand() % 3;
    double x = rand() % 0x1000;     
    double max=1 / sqrt(2*pi) / sig;
    double gaus= max*exp( - 0.5* (x-mu) * (x-mu) / sig*sig);   
    int adcValue = (int) gaus;
     target[6*i+0] = ((0x1 << 5) | (gC.rocNumber << 2) | nxyter)  & 0xFF;//msg type, roc#, nx#0
      target[6*i+1] = (nxTimestamp >> 9) & 0xFF;
      target[6*i+2] = (nxTimestamp >> 1) & 0xFF;
      target[6*i+3] = ((nxTimestamp << 7) | (channel & 0x7F)) & 0xFF;
      target[6*i+4] = (adcValue >> 5) & 0xFF;
      
      if(lastepoch)
         target[6*i+5] = ((adcValue << 3) & 0xFF) | 1; //last Epoch  
      else
         target[6*i+5] = (adcValue << 3) & 0xFF;
    
//    if(((nxTimestamp + 7) >> 14) > 0)
//      {
//               makeEpochMarker=true;
//               return; // discard rest of hits at epoche end                        
//      }
//    else
//      {      
//         nxTimestamp+=7;   
//         nxTimestamp = nxTimestamp & 0x3FFF;   
//      }
 }// for

}

void SysCoreSimulator::nextHitTimestamp()
{
uint16_t increment= rand() % 0x1FFF;    ;

if(((nxTimestamp + increment*7) >> 14) > 0)
   {
      makeEpochMarker=true;            
   }    
nxTimestamp+=increment*7;   
nxTimestamp = nxTimestamp & 0x3FFF;   

}

void SysCoreSimulator::makeSysMessage(uint8_t typ)
{
   SysCore_Data_Packet_Full pkt;

   pkt.msgs[0] = ((0x7 << 5) | (gC.rocNumber << 2) | 0)  & 0xFF;//msg type, roc#, unused
   pkt.msgs[1] = typ;
   pkt.msgs[2] = 0;
   pkt.msgs[3] = 0;
   pkt.msgs[4] = 0;
   pkt.msgs[5] = 0;

   pkt.nummsg = htonl(1);
   pkt.pktid = htonl(gD.headid++);
   printf("Put sys msg %u in pkt with id %u\n", typ, ntohl(pkt.pktid));
   gD.buf.push_back(pkt);
}

void SysCoreSimulator::processDataRequest(SysCore_Data_Request_Full* pkt)
{
   gD.last_req_pkt_id = pkt->reqpktid;
   gD.send_limit = pkt->frontpktid;
   
//   printf("Next send limit is %u size = %u\n",gD.send_limit, gD.buf.size());
   
   while (gD.buf.size() > 0) {
      uint32_t diff = pkt->tailpktid - ntohl(gD.buf.front().pktid);
      if ((diff !=0) && (diff < 0x1000000)) gD.buf.pop_front();
                                       else break;
      if ((gD.daqState==1) && !gD.data_taking_on)
         if (gD.buf.size() < gD.low_water)
            gD.data_taking_on = true;  
   }
   
   for (unsigned n=0; n<pkt->numresend; n++)
      if (gD.resend_size<RESEND_MAX_SIZE) {
         gD.resend_indx[gD.resend_size] = pkt->resend[n];
         gD.resend_size++;
      }
}


bool SysCoreSimulator::sendDataPacket(uint32_t pktid)
{
   if (gD.droppkt_id == pktid) {
      printf("Explicit drop of packet 0x%x\n",gD.droppkt_id);
      gD.droppkt_id = rand();
      return true;
   }
   
   if (gD.buf.size()==0) return false;
   
   

//   printf("Send data packet %u size %u front %u\n",
//             pktid, gD.buf.size(), ntohl(gD.buf.front().pktid));
   
   std::list<SysCore_Data_Packet_Full>::iterator iter = gD.buf.begin();
    
   while (iter != gD.buf.end()) {

      if (ntohl(iter->pktid) == pktid) {
          iter->lastreqid = htonl(gD.last_req_pkt_id);
          
          if (!doSendLost()) 
             dataSocket_.SendToBuf(gD.masterIp, gD.masterDataPort,
                (char*) &(*iter), sizeof(SysCore_Data_Packet) + ntohl(iter->nummsg) * 6);
          return true;
      }
        
      iter++;  
   }

   return false;   
}



void SysCoreSimulator::mainLoop()
{
   SysCore_Data_Packet_Full pkt;
   
   printf("SysCoreSimulator is running...\n");
   
   bool did_something = false;
   bool send_something = false;
   
   gD.lastMasterTime = get_clock();
   double last_tm = get_clock();
      
   while (socketHandler_.GetCount()) {
      
      if (!did_something) 
         socketHandler_.Select(0, 1000);
      
      did_something = false;
   
      if (gD.data_taking_on) {
         switch(simulationType_) { 
            case 0: fillData(pkt.msgs, 243); break;
            case 1: fillRareSyncData(pkt.msgs, 243); break;
            case 2: fillDataJoern(pkt.msgs, 243); break;
            case 3: fillDetectorHitData(pkt.msgs, 243);  break; 
            default: fillRareSyncData(pkt.msgs, 243); break;
         }
         pkt.nummsg = htonl(243);
         pkt.pktid = htonl(gD.headid++);
         gD.buf.push_back(pkt);
         
//         printf("Add packet %u  total size %u\n", ntohl(pkt.pktid), gD.buf.size());
         if (gD.buf.size() > gD.high_water) gD.data_taking_on = false;
      }
      
      
      double curr_tm = get_clock();
      
      // process flush timeout
      if (gD.daqState && (fabs(curr_tm - gD.lastFlushTime) > gC.flushTimeout*0.001)) {
         // we have always full buffer, therefore just resend last buffer  
         
         uint32_t prev_send_id = gD.next_send_id - 1;
         
         if (!send_something)
            if (gD.headid - prev_send_id <= gD.buf.size()) 
               if (sendDataPacket(prev_send_id))
                  printf("Flush buffer id 0x%x\n",prev_send_id);
            
         send_something = false;
         gD.lastFlushTime = get_clock();
      }

      if (gD.masterConnected && fabs(gD.lastMasterTime - curr_tm) > 10) {
         printf("Disconnect master after 10 sec!\n");
         gD.masterConnected = false;
         gD.data_taking_on = false;
         gD.daqState = 0;
      }
      
      if (fabs(gD.lastMasterTime - curr_tm) > 3000) {
         printf("No news from master last 3000 sec, stop main loop!!\n");
         break;
      }

      // retransmit when required
      if (gD.resend_size > 0) {
         sendDataPacket(gD.resend_indx[--gD.resend_size]);
         did_something = true;
         send_something = true;
      } else
      // if one allowed to send 
      if (gD.send_limit != gD.next_send_id)
         if (gD.send_limit - gD.next_send_id < 0x1000000)
            if (sendDataPacket(gD.next_send_id)) {
               gD.next_send_id++;
               did_something = true;
               send_something = true;
            }
      
      if (fabs(curr_tm - last_tm) > 10.) {
         if (totalSendLosts>0)
            printf("Send:%8llu Lost:%5.1f%s\n", totalSendCounter, 100. * totalSendLosts/totalSendCounter, "%");
         if (totalRecvLosts>0)
            printf("Recv:%8llu Lost:%5.1f%s\n", totalRecvCounter, 100. * totalRecvLosts/totalRecvCounter, "%");
         last_tm = curr_tm;
      }
   }
}

