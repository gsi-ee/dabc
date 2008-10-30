#ifndef SYSCOREBOARD_H_
#define SYSCOREBOARD_H_

#include <list>

#ifndef __CINT__
#include <pthread.h>
#else
// fake definitions just to make CINT happy :)
typedef unsigned pthread_cond_t;
typedef unsigned pthread_mutex_t;
typedef unsigned long int pthread_t;
#endif

//address space
#include "SysCoreDefines.h"

#include "SysCoreData.h"

class SccDataSocket;
class SysCoreControl;
class SysCoreSorter;

//4mb buffer as in syscore
#define SC_BITFILE_BUFFER_SIZE 4194304


extern double get_clock();


class LockGuard {
  protected:
     pthread_mutex_t* fMutex;
  public:
     inline LockGuard(pthread_mutex_t& mutex) : fMutex(&mutex)
     {
        pthread_mutex_lock(fMutex);
     }
     inline LockGuard(pthread_mutex_t* mutex) : fMutex(mutex)
     {
        pthread_mutex_lock(fMutex);
     }
     inline ~LockGuard()
     {
        if (fMutex) pthread_mutex_unlock(fMutex);
     }
};

//! SysCore_Message_Packet struct
/*!
 * This is the struct storing the information for one peek and poke
 * it is also used as header in a udp data packet.
 * size is 24 bytes
 */

#define UDP_PAYLOAD_OFFSET 42
#define MAX_UDP_PAYLOAD 1472
#define MESSAGES_PER_PACKET 243


struct SysCore_Message_Packet
{
   uint8_t  tag;
   uint8_t  pad[3];
   uint32_t password;
   uint32_t id;
   uint32_t address;
   uint32_t value;
};

struct SysCore_Message_Packet_Full : public SysCore_Message_Packet 
{
   uint8_t rawdata[MAX_UDP_PAYLOAD - sizeof(SysCore_Message_Packet)];
};

struct SysCore_Data_Request
{
   uint32_t password;
   uint32_t reqpktid;
   uint32_t frontpktid; 
   uint32_t tailpktid;
   uint32_t numresend;
};

struct SysCore_Data_Request_Full : public SysCore_Data_Request 
{
   uint32_t resend[(MAX_UDP_PAYLOAD - sizeof(struct SysCore_Data_Request)) / 4];   
};

struct SysCore_Data_Packet
{
   uint32_t pktid;
   uint32_t lastreqid;
   uint32_t nummsg;  
// here all messages should follow   
};

struct SysCore_Data_Packet_Full : public SysCore_Data_Packet 
{
   uint8_t msgs[MAX_UDP_PAYLOAD - sizeof(struct SysCore_Data_Packet)];
};

struct SysCore_Statistic {
   uint32_t   dataRate;   // data taking rate in  B/s 
   uint32_t   sendRate;   // network send rate in B/s 
   uint32_t   recvRate;   // network recv rate in B/s 
   uint32_t   nopRate;    // double-NOP messages 1/s
   uint32_t   frameRate;  // unrecognised frames 1/s
   uint32_t   takePerf;   // relative use of time for data taing (*100000)
   uint32_t   dispPerf;   // relative use of time for packets dispatching (*100000)
   uint32_t   sendPerf;   // relative use of time for data sending (*100000)
};


//! SysCore Board class
/*!
 * This class encapsulates all functionality of one single ROC on the PC side.
 */
class SysCoreBoard
{
   friend class SccDataSocket;
   friend class SysCoreControl;

private:

    struct ResendPkt {

       ResendPkt(uint32_t id = 0) :
          pktid(id), addtm(0.), lasttm(0.), numtry(0) {}

       uint32_t pktid;  // id of packet
       double   addtm;  // last resend time
       double   lasttm;  // last resend time
       int      numtry;

    };

   enum EDaqState { daqInit, daqStarting, daqRuns, daqStopping, daqStopped, daqFails };

   SysCoreControl*   controler_;
   SccDataSocket*    dataSocket_;

   uint16_t          rocCtrlPort;
   uint16_t          rocDataPort;
   uint16_t          hostDataPort;
   
   uint32_t          ipAddress;
   uint32_t          rocNumber;   // number of board as it is configured 
   
   bool              masterMode; // defines if one allowed to perform and control function (true) 

   EDaqState         daqState;   // current state of DAQ
   bool              daqActive_;  // indicate if we allows to get/request data from ROC

   SysCore_Message_Packet_Full  controlSend;
   unsigned                     controlSendSize; // size of control data to be send
   SysCore_Message_Packet_Full  controlRecv;

   uint32_t currentMessagePacketId;

   unsigned transferWindow;      // maximum value for credits

   uint32_t lastRequestId;   // last request id 
   double   lastRequestTm;   // time when last request was send
   bool     lastRequestSeen; // indicate if we seen already reply on last request
   uint32_t lastSendFrontId; // last send id of front packet
   
   pthread_mutex_t dataMutex_;    // locks access to receieved data
   pthread_cond_t  dataCond_;     // condition to synchronise data consumer 
   unsigned        dataRequired_; // specifies how many messages required consumer
   unsigned        consumerMode_; // 0 - no consumer, 1 - waits condition, 2 - call back to controller
   bool            fillingData_;  // true, if consumer thread fills data from ring buffer
   


   SysCore_Data_Packet_Full* ringBuffer; // ring buffer for data 
   unsigned      ringCapacity; // capacity of ring buffer 
   unsigned      ringHead;     // head of ring buffer (first free place)
   uint32_t      ringHeadId;   // expected packet id for head buffer 
   unsigned      ringTail;     // tail of ring buffer (last valid packet)
   unsigned      ringSize;     // number of items in ring buffer
   

   std::list<ResendPkt>   packetsToResend;
   SysCore_Statistic      brdStat;    // last avalible statistic block
   bool                   isBrdStat;  // is block statistic contains valid data

   uint8_t readBuf_[1800];       // intermidiate buffer for getNextData()
   unsigned readBufSize_;
   unsigned readBufPos_;

   uint64_t fTotalRecvPacket;
   uint64_t fTotalResubmPacket;
   
   SysCoreSorter*        sorter_;

   bool startDataSocket();

   int parseBitfileHeader(char* pBuffer, unsigned int nLen);

   void addDataPacket(const char* p, unsigned l);

   void TestTimeoutConditions(double curr_tm);

   bool _checkDataRequest(SysCore_Data_Request_Full* req, double curr_tm, bool check_retrans);
   bool _checkAvailData(unsigned num_msg);

   void sendDataRequest(SysCore_Data_Request_Full* pkt);
   
   bool uploadDataToRoc(char* buf, unsigned len);

   int pokeRawData(uint32_t address, const void* rawdata, uint32_t rawdatelen, double tmout = 2.);

   void  setBoardStat(void* rawdata, bool print);

   static uint16_t gDataPortBase;

public:

   SysCoreBoard(SysCoreControl* controler, uint32_t address, bool asmaster, uint16_t portnum);

   virtual ~SysCoreBoard();
   
   bool isMasterMode() const { return masterMode; }

   void setUseSorter(bool on = true);

   uint8_t calcBinXOR(const char* filename);


   /*******************************
    * Fastcontrol
    *******************************/

   //! Check if a data class is held in the FIFO
   /*!
    * \param hostIp Target Ip address to use. Parses the Ip address as String.
    */
   bool isDataInBuffer(double tmout = 0.);

    //! Copy next Data Class from networkbuffer to Memory
   /*!
    * \param target Takes the Memory target address by reference and fills it.
    */
   bool getNextData(SysCoreData &target, double tmout = 0.);


    // Alternative way to access data from board

    // Wait that at least specified number of messages is arrived
    // if wait_sec == 0, just check if data already there
    bool waitData(unsigned num_msgs = 1, double wait_sec = 0.1);

    // fill binary buffer with SysCoreData, sz in bytes
    // returns true when at least one message is filled
    bool fillData(void* buf, unsigned& sz);
    

    // non-blocking way to request some portion of data from board
    // If data already availible, returns 1
    // If somebody else already made the request, returns 0 (error)
    // If method returns 2, SysCoreController::DataCallBack will be called
    // when required amount of data will be availible
    int requestData(unsigned num_msgs = 1);


   /*******************************
    * Getter
    *******************************/
   inline unsigned getRocNumber() const { return rocNumber; }
   inline uint32_t getIpAddress() const { return ipAddress; }
   inline uint16_t getControlPort() const { return rocCtrlPort; }
   inline uint16_t getDataPort() const { return rocDataPort; }
   inline uint16_t getHostDataPort() const { return hostDataPort; }
   inline uint16_t getTransferWindow() const { return transferWindow; }

   double getLostRate() const { return  fTotalRecvPacket == 0 ? 0 : 1. * fTotalResubmPacket / fTotalRecvPacket; }
   void resetLostRate() { fTotalResubmPacket = 0; fTotalResubmPacket = 0; }
   uint64_t totalRecvPackets() const { return fTotalRecvPacket; }
   uint64_t totalLostPackets() const { return fTotalResubmPacket; }

   bool daqActive() const { return daqActive_; }

   /*******************************
    * "kernel"
    *******************************/

   //! PEEK
   /*!
   * This is the main interface for reading values from the Readout Controlers. Look in
   * manual or the defines.h for the addressspace of the ROC.
   * \param tmout defines how long method should try to get value from roc
   */
   int peek(uint32_t address, uint32_t &readValue, double tmout = 2.);


   //! POKE
   /*!
   * This is the main interface for writing values to the Readout Controlers. Look in
   * manual or the defines.h for the addressspace of the ROC.
   * \param tmout defines how long method should try to set value to roc
   */

   int poke(uint32_t address, uint32_t writeValue = 0, double tmout = 2.);


   //! startDaq
   /*!
   * This function should be called when startting DAQ, not only poke(ROC_START_DAQ, 1)
   * It reset all counter, initialise buffer and at the end start DAQ
   * \param trWindow sets number of udp packet, which can be transported by board without confirmation
   */
   bool startDaq(unsigned trWindow = 40);

   //! suspendDaq
   /*!
   * Sends to ROC ROC_SUSPEND_DAQ command. ROC will produce stop message
   * and will deliver it sometime to host PC. User can continue to take data until
   * this message. Once it is detected, no more messages will be delivered.
   */
   bool suspendDaq();

   //! stopDaq
   /*!
   * Complimentary function for startDaq.
   * Disables any new data requests, all coming data will be discarded
   */
   bool stopDaq();

   //! stopDaq
   /*!
   * Cleanups all data on client side, brakes any data transport,
   * do not sends any command like stop daq to ROC. Always called by startDaq()
   */
   void resetDaq();



   /*******************************
    * other stuff
    *******************************/

   //! Send console command
   /*!
   * Via this function you can send remote console commands. 
   * It works like telnet. 
   * It is developed mainly for debugging reasons.
   */
   bool sendConsoleCommand(const char* cmd);

   //! Upload bit file to ROC
   /*!
   * Uploads bitfile to specified position (0 or 1)
   * Returns true if upload was successfull.
   */
   bool uploadBitfile(const char* filename, int position);
   
   bool uploadSDfile(const char* filename, const char* sdfilename = 0);
   
   bool saveConfig(const char* filename = 0);
   bool loadConfig(const char* filename = 0);
   
   //! takeStat
   /*!
   * Returns statistic block over SysCoreBoard.
   * If \param tmout bigger than 0, first request to board will be produced,
   * otherwise last avalible statistic block will be delivered
   * If \para, print is enabled, statistic block will be printed on dysplay
   */
   
   SysCore_Statistic* takeStat(double tmout = 0.01, bool print = false);
   //! setDataPortBase
   /*!
   * Set base for data ports, used by SysCoreBoard classes.
   * Change this value only if default number (5846) not works for your system
   */
   static void setDataPortBase(uint16_t base) { gDataPortBase = base; }
};


#endif
