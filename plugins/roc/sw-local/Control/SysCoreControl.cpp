#include "SysCoreControl.h"

#include "../CppSockets/UdpSocket.h"
#include "../CppSockets/SocketHandler.h"
#include "../CppSockets/StdoutLog.h"
#include "../CppSockets/Mutex.h"
#include "../CppSockets/Utility.h"

#include <netinet/in.h>
#include <string.h>

#include <iostream>
using namespace std;

#ifdef __SC_DEBUG__
#define DOUT(args) printf args
#else
#define DOUT(args)
#endif

#define EOUT(args) printf args
#define DOUT0(args) printf args


/*****************************************************************************
 * Implementations
 ****************************************************************************/

/*****************************************************************************
 * Sockets
 ****************************************************************************/

//! SysCore Control DataSocket class
/*!
 * This class encapsulates all buffermanagment, fetchment and sending for the control
 * packets.
 */
class SccCommandSocket : public UdpSocket {
   protected:
      SysCoreControl* controler_;

   public:
      SccCommandSocket(ISocketHandler& h, SysCoreControl* controler) : UdpSocket(h)
      {
         controler_ = controler;
      }

      virtual ~SccCommandSocket()
      {
         DOUT(("Command socket %d closed\n", GetPort()));
      }

      virtual void OnRawData(const char *p, size_t l, struct sockaddr *sa_from,socklen_t sa_len)
      {

         // check size of IPv4 address
         if (sa_len != sizeof(struct sockaddr_in)) return;

         struct sockaddr_in sa;
         memcpy(&sa, sa_from, sa_len);
         SysCore_Message_Packet* packet = (SysCore_Message_Packet*) p;
      //printf("Received %d bytes from: %s:%d\n", (int)l,ip.c_str(),ntohs(sa.sin_port));
         int boardnum = controler_->getBoardNumByIp(*((uint32_t*)&sa.sin_addr));
         if((boardnum<0) || (ntohl(packet->password) != ROC_PASSWORD)) return;
         
         controler_->processCtrlMessage(boardnum, packet, l);
      }
};



/*****************************************************************************
 * SysCoreControl
 ****************************************************************************/

uint16_t SysCoreControl::gControlPortBase = 5737;

SysCoreControl::SysCoreControl() :
   controlPort_(0),
   displayConsoleOutput_(true),
   stOutLog_(0),
   socketHandlerMutex_(0),
   socketHandler_(0),
   cmdSocket_(0),
   fbFetcherStop(true)
{
   DOUT(("SysCoreControl ctor..\n"));
   initSockets();
   startFetcherThread();
}

SysCoreControl::~SysCoreControl()
{
   DOUT(("SysCoreControl dtor..\n"));
    
   for (unsigned n=0;n<getNumberOfBoards();n++)
      if (board(n).isMasterMode())
         board(n).poke(ROC_MASTER_LOGOUT, 0);
   
   stopFetcherThread();

   socketHandler_->RegStdLog(0);
   delete stOutLog_; stOutLog_ = 0;

   deleteBoards(false);

   delete cmdSocket_; cmdSocket_ = 0;

   delete socketHandler_; socketHandler_ = 0;

   delete socketHandlerMutex_; socketHandlerMutex_ = 0;
}

void SysCoreControl::initSockets()
{
   stOutLog_ = new StdoutLog;

   socketHandlerMutex_ = new Mutex;

   socketHandler_ = new SocketHandler(*socketHandlerMutex_, stOutLog_);

   cmdSocket_ = new SccCommandSocket(*socketHandler_, this);

   port_t port = gControlPortBase;
   if (cmdSocket_->Bind(port, 1000) == 0) {
      controlPort_ = cmdSocket_->GetPort();   
      DOUT(("Ready to receive control data on port %d.\n", controlPort_)); 
      socketHandler_->Add(cmdSocket_);
      return; 
   }
   
   EOUT(("Command Socket Bind failed for ports %u .. %u.\n", 
                     gControlPortBase, gControlPortBase + 1000));
   delete cmdSocket_;
   cmdSocket_ = 0;
   controlPort_ = 0;
}

void SysCoreControl::fetcher()
{
   DOUT(("SysCoreControl::fetcher() THREAD started.\n"));
   socketHandler_->Select(1,0);
   double last_tm = get_clock();

   while (socketHandler_->GetCount() && !fbFetcherStop)
   {
      socketHandler_->Select(0,10000);

      double curr_tm = get_clock();

      if (curr_tm > last_tm + 0.01) {
         last_tm = curr_tm;
         for(unsigned int i = 0; i < getNumberOfBoards(); i++)
            board(i).TestTimeoutConditions(curr_tm);
      }
   }
   DOUT(("SysCoreControl::fetcher() THREAD is leaving, fFetcherStop = %d\n", fbFetcherStop));
}

void *helper_function_pointer(void * ptr)
{
   SysCoreControl* scc = static_cast<SysCoreControl*>(ptr);

   scc->fetcher();

   pthread_exit(0); 
}

void SysCoreControl::startFetcherThread()
{
   fbFetcherStop = false;

   ctrlState_ = 0;

   pthread_mutex_init(&ctrlMutex_, NULL);
   pthread_cond_init(&ctrlCond_, NULL);

   pthread_create(&fetcherThread_, 0, helper_function_pointer, this);
}

void SysCoreControl::stopFetcherThread()
{
   DOUT(("SysCoreControl::stopFetcherThread starts\n"));
   
   fbFetcherStop = true;
   
   void* res = 0; 
   pthread_join(fetcherThread_, &res);

   pthread_cond_destroy(&ctrlCond_);
   pthread_mutex_destroy(&ctrlMutex_);
   ctrlState_ = 0;
   
   DOUT(("SysCoreControl stopFetcherThread done\n"));
}

int SysCoreControl::addBoard(const char* address, bool asmaster, uint16_t portnum)
{
   ipaddr_t ip;
   if (!Utility::u2ip(std::string(address), ip)) {
      EOUT(("Cannot resolve (convert) address %s to IP\n", address));
      return -1;
   }
   
   if (cmdSocket_ == 0) {
      EOUT(("Command socket not exists !!!\n"));
      return -1;   
   }

   SysCoreBoard* scb = new SysCoreBoard(this, ip, asmaster, portnum);
   if (!scb->startDataSocket()) {
      EOUT(("Cannot start data socket for board %s\n", address));
      delete scb;
      return -1;
   }
   
   boards_.push_back(scb);

   bool iserr = false;
   
   if (asmaster) {

      iserr = scb->poke(ROC_MASTER_LOGIN, 0, 2.) != 0;
   
      if (!iserr)
         iserr = scb->poke(ROC_MASTER_DATAPORT, scb->getHostDataPort()) != 0;
   }

   uint32_t read_val = 0;

   if (!iserr) {

      iserr = scb->peek(ROC_SOFTWARE_VERSION, read_val) != 0;

      if((read_val >= 0x01070000) || (read_val < 0x01060000)) {
         EOUT(("The ROC you want to access has software version %x \n", read_val));
         EOUT(("This C++ access class only supports boards with major version 1.6 == %x\n", 0x01060000));
         iserr = true;
      }

      DOUT(("ROC software version is: 0x%x\n", read_val));
   }

   if (!iserr) {
      iserr = scb->peek(ROC_HARDWARE_VERSION, read_val) != 0;

      if((read_val >= 0x01070000) || (read_val < 0x01060000)) {
         EOUT(("The ROC you want to access has hardware version %x \n", read_val));
         EOUT(("Please update your hardware to major version 1.6 == %x\n", 0x01060000));
         iserr = false;// update requires that
      }

      DOUT(("ROC hardware version is: 0x%x\n", read_val));
   }
   
   if (!iserr) {
      iserr = iserr || scb->peek(ROC_NUMBER, read_val) != 0; 
      scb->rocNumber = read_val;

      iserr = iserr || scb->peek(ROC_DATAPORT, read_val) != 0; 
      scb->rocDataPort = read_val;
   }

   std::string boardip;
   Utility::l2ip(ip, boardip);
   DOUT(("Connecting to board %s ... %s\n", boardip.c_str(), iserr ? "failed" : "ok"));
   if (iserr) {
      EOUT(("Failed to connect to board %s\n", boardip.c_str()));
      boards_.pop_back();
      delete scb;
   } else {
      socketHandler_->Add((UdpSocket*)scb->dataSocket_);
       
      DOUT0(("Board %s, number %u is now available as board(%u) in %s mode \n", 
         boardip.c_str(), scb->rocNumber, boards_.size()-1, (asmaster ? "master" : "observer")));
   }

   return iserr ? -1 : (int) boards_.size()-1;
}

void SysCoreControl::deleteBoards(bool logout)
{
   socketHandler_->RegStdLog(0);
    
   while (boards_.size()>0) {
      SysCoreBoard* brd = boards_.back();
      
      if (logout && brd->isMasterMode())
         brd->poke(ROC_MASTER_LOGOUT, 0);
       
      delete brd;
      
      boards_.pop_back();
   }

   socketHandler_->RegStdLog(stOutLog_);
}

int SysCoreControl::getBoardNumByIp(uint32_t address)
{
   for(unsigned i = 0; i < getNumberOfBoards(); i++)
      if (board(i).getIpAddress() == address)
         return (int) i;
   return -1;
}

int SysCoreControl::getBoardNumByIp(const char* address)
{
   ipaddr_t ip;
   Utility::u2ip(std::string(address), ip);
   return getBoardNumByIp(ip);
}

bool SysCoreControl::performCtrlLoop(SysCoreBoard* brd, double total_tmout_sec, bool show_progress)
{
   // normal reaction time of the ROC is 0.5 ms, therefore one can resubmit command
   // as soon as several ms without no reply
   // At the same time there are commands which takes seconds (like SD card write)
   // Therefore, one should try again and again if command is done 
    
   {
      // before we start sending, indicate that we now in control loop
      // and can accept replies from ROC
      LockGuard guard(ctrlMutex_);
      if (cmdSocket_==0) return false;
      if (ctrlState_!=0) {
         EOUT(("cannot start operation - somebody else uses control loop\n"));
         return false;
      }
      
      ctrlState_ = 1;
   }

   brd->controlSend.password = htonl(ROC_PASSWORD);
   brd->controlSend.id = htonl(brd->currentMessagePacketId++);

   bool res = false;
   
   // send alligned to 4 bytes packet to the ROC
   
   while ((brd->controlSendSize < MAX_UDP_PAYLOAD) && 
          (brd->controlSendSize + UDP_PAYLOAD_OFFSET) % 4) brd->controlSendSize++;

   if (total_tmout_sec>20.) show_progress = true;
   
   // if fast mode we will try to resend as fast as possible
   bool fast_mode = (total_tmout_sec < 10.) && !show_progress;
   int loopcnt = 0;
   bool wasprogressout = false;
   bool doresend = true;
   
   do {
      if (doresend) {
         cmdSocket_->SendToBuf(brd->getIpAddress(), brd->getControlPort(),
                               (char*)&(brd->controlSend), brd->controlSendSize);
         doresend = false;                 
      }
      
      struct timeval tp;
      gettimeofday(&tp, 0);
      
      double wait_tm = fast_mode ? (loopcnt++ < 4 ? 0.01 : loopcnt*0.1) : 1.;
      long wait_microsec = long(wait_tm*1e6);

      tp.tv_sec += (wait_microsec + tp.tv_usec) / 1000000;
      tp.tv_usec = (wait_microsec + tp.tv_usec) % 1000000;

      struct timespec tsp  = { tp.tv_sec, tp.tv_usec*1000 };

      LockGuard guard(ctrlMutex_);

      pthread_cond_timedwait(&ctrlCond_, &ctrlMutex_, &tsp);

      total_tmout_sec -= wait_tm;
      
      // resend packet in fast mode always, in slow mode only in the end of interval
      doresend = fast_mode ? true : total_tmout_sec <=5.;
      
      if (ctrlState_ == 2) 
         res = true;
      else 
      if (show_progress) {
          std::cout << ".";
          std::cout.flush();
          wasprogressout = true;
      }
   } while (!res && (total_tmout_sec>0.));

   if (wasprogressout) std::cout << std::endl;

   LockGuard guard(ctrlMutex_);
   ctrlState_ = 0;
   return res;
}

void SysCoreControl::processCtrlMessage(int boardnum, SysCore_Message_Packet* pkt, unsigned len)
{
   // procees PEEK or POKE reply messages from ROC

   if ((pkt==0) || (boardnum<0) || (boardnum>=(int) getNumberOfBoards())) return;

   SysCoreBoard* brd = &board(boardnum);


   if(pkt->tag == ROC_CONSOLE) {
      SysCore_Message_Packet_Full* full = (SysCore_Message_Packet_Full*) pkt;
      
      
      full->address = ntohl(full->address);
      
      switch (full->address) {
         case ROC_STATBLOCK:
            brd->setBoardStat(full->rawdata, displayConsoleOutput_);
            break;    
             
         case ROC_DEBUGMSG:
            if (displayConsoleOutput_)
               DOUT0(("\033[0;%dm Roc:%d %s \033[0m", 36, boardnum, (const char*) full->rawdata));
            break;
         default:
            if (displayConsoleOutput_)
               DOUT0(("Error addr 0x%04x in cosle message\n", full->address));
      }
         
      return;  
   }

   LockGuard guard(ctrlMutex_);

   // check first that user waits for reply
   if (ctrlState_ != 1) return;

   // if packed id is not the same, do not react
   if(brd->controlSend.id != pkt->id) return;

   // if packed tag is not the same, do not react
   if(brd->controlSend.tag != pkt->tag) return;
   
   if (len > sizeof(SysCore_Message_Packet_Full)) 
      len = sizeof(SysCore_Message_Packet_Full);

   memcpy(&brd->controlRecv, pkt, len);

   ctrlState_ = 2;
   pthread_cond_signal(&ctrlCond_);
}
