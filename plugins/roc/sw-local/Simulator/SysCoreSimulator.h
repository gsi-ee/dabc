#ifndef SYSCORESIMULATOR_H_
#define SYSCORESIMULATOR_H_

//============================================================================
/*! \file SysCoreSimulator.h
 *   \date  23-06-2008
 *  \version 1.2    
 *   \author Stefan Mueller-Klieser <stefanmk@kip.uni-heidelberg.de>
 * 
 * copyright: Kirchhoff-Institut für Physik
 * 
 * Changelog:
 * to Version 1.2
 * - adjust to new data protocol
 * 
 * to Version 1.1
 * - data pattern made more expedient
 * - new Doxyfile
 * 
 * Version 1.0
 * -first release
 *   
 */
//============================================================================

#define ROC_SIMULATOR_SW_VERSION 0x01060701
#define ROC_SIMULATOR_HW_VERSION 0x01060502

//Doxygen Main Page

/*! \mainpage Documentation of the SysCoreSimulator class version 1.1
 *
 * \section intro_sec Introduction
 *
 * The SysCoreSimulator class is implemented to simulate the bahavior of 
 * the SysCore Board. You can use and test the SysCoreControl class and the
 * DAQ analysis system without having an actual SysCoreBoard at your hands.
 * 
 * \section install_sec Installation
 * 
 * - Go to "/Release" subfolder
 * - type "make"
 * - run ./SysCoreSimulator [udpDataPort] [udpControlPort]
 *   these ports have to be free, the simulator will bind these both ports. You
 *   have to tell the SysCoreControl class the control port when you call .addBoard()   
 */

#include "SysCoreDefines.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>

#include "../CppSockets/StdoutLog.h"
#include "../CppSockets/SocketHandler.h"
#include "../CppSockets/UdpSocket.h"
#include "../CppSockets/Utility.h"

#define UDP_PAYLOAD_OFFSET 42
#define MAX_UDP_PAYLOAD 1472

struct SysCore_Message_Packet
{
   uint8_t  tag;
   uint8_t  pad[3];
   uint32_t password;
   uint32_t id;
   uint32_t address;
   uint32_t value;
};

struct SysCore_Message_Packet_Full : public SysCore_Message_Packet {
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

struct SysCore_Data_Request_Full : public SysCore_Data_Request {
   uint32_t resend[(MAX_UDP_PAYLOAD - sizeof(struct SysCore_Data_Request)) / 4];   
};


struct SysCore_Data_Packet
{
   uint32_t pktid;
   uint32_t lastreqid;
   uint32_t nummsg;
// here all messages should follow   
};

struct SysCore_Data_Packet_Full : public SysCore_Data_Packet {
    
   uint8_t msgs[MAX_UDP_PAYLOAD - sizeof(struct SysCore_Data_Packet)];
};





#define ETHER_ADDR_LEN 6
#define IP_ADDR_LEN 4

/*****************************************************
 * 
 * SysCoreBoard Configuration Structure adapted from firmware
 * 
 *****************************************************/  

/*****************************************************
 * SysCore Configuration Structure
 *****************************************************/  
struct sc_config
{
   uint16_t   ctrlPort;
   uint16_t   dataPort;

   int        rocNumber;
   int        flushTimeout;
};

extern struct sc_config gC;


#define RESEND_MAX_SIZE 128


struct sc_data 
{
   uint32_t  headid;          // id of buffer which is now filled 
   
   std::list<SysCore_Data_Packet_Full>  buf;
   
   unsigned  high_water;      /* Maximum number of buffers to be used in normal situation */
   unsigned  low_water;       /* Number of buffers to enable data taking after is was stopped */
   
   uint32_t  send_limit;      /* limit for sending - can be in front of existing header */
   uint32_t  next_send_id;    /* id of next packet to send */
   
   uint32_t   resend_indx[RESEND_MAX_SIZE]; /* indexes of buffers which should be resended */
   unsigned   resend_size;                  /* number of indexes to be resended */
   
   uint32_t   last_req_pkt_id; /* id of last request packet, should be always send back */
   
   bool       masterConnected; // 1 if master controller is connected

   port_t     masterDataPort;
   port_t     masterControlPort;
   ipaddr_t   masterIp;
   
   unsigned   daqState;

   double     lastFlushTime;
   double     lastMasterTime;
   
   uint32_t   droppkt_id;
   
   bool       data_taking_on;  /* indicate if we can take new buffer */


};

extern struct sc_data gD;



class SysCoreSimulator;

class ScSimuDataSocket;
class ScSimuControlSocket;



//! SysCore Simluator DataSocket class
/*!
 * This class encapsulates all DataSend 
 */
class ScSimuDataSocket : public UdpSocket
{
   SysCoreSimulator* controler_;
   
public:
   ScSimuDataSocket(ISocketHandler&, SysCoreSimulator* controler);

   virtual void OnRawData(const char *,size_t,struct sockaddr *,socklen_t);
};


//! SysCore Simluator ControlSocket class
/*!
 * This class encapsulates all Slow Control handling 
 */
class ScSimuControlSocket : public UdpSocket
{
   SysCoreSimulator* controler_;
   
public:
   ScSimuControlSocket(ISocketHandler&, SysCoreSimulator* controler);

   virtual void OnRawData(const char *,size_t,struct sockaddr *,socklen_t);
};

//! SysCore Simulator class
/*!
 * This is the toplevel class. Use it in your application to communicate with the ROC boards.
 */
class SysCoreSimulator 
{
private:
   uint32_t nxRegister[16];
   uint32_t syncMDelay_;
   
   SocketHandler        socketHandler_;
   StdoutLog            stOutLog_;
   ScSimuDataSocket     dataSocket_;
   ScSimuControlSocket  cmdSocket_;
    
   friend class ScSimuDataSocket;
   friend class ScSimuControlSocket;
   
   uint16_t nxTimestamp;
   uint32_t nxEpoch;
   bool makeEpochMarker;

    long double clockScrewUp;     // coefficient (1. is normal), which indicate screwup of clock
    uint64_t nextTimeHit;   // when hit will be generated
    unsigned nextHitCounter;  // reverse counter for hits generation
    uint64_t nextEventNumber; // current event number
    uint64_t nextTimeNoise; // indicate when time stamp message should be
    uint64_t nextTimeSync;  // indicate when next time sync message should appear
    uint64_t nextTimeAUX;   // when next AUX signal will be
    uint32_t lastEpoch;     // last epoch set in data stream
    uint64_t nextSyncEvent; // number of next sync event

    // list of last N packets sended to PC, used in resubmit 
    double recvLostRate; // relative loss rate of packets recieve 
    double sendLostRate; // relative loss rate of packets send
    
    unsigned recvLostCounter; // backward counter to next recv lost 
    unsigned sendLostCounter; // backward counter to next send lost 
    
    uint64_t totalSendCounter, totalSendLosts;
    uint64_t totalRecvCounter, totalRecvLosts;
    
    /** type of simulated data:
     * 0 - original: dense timestamps with one hit each for first channel, scrambled
     * 1 - rare synchronization data 
     * 2 - many random hits with peaked multiplicity for all channels in each epoche
     * 3 - rare hits for different channels, like "real" detector hits*/
    int simulationType_;
    
    void scramble(uint8_t* target, int start, int end);
    
    bool doRecvLost();
    bool doSendLost();
    
    void resetSimulator();
    
    bool sendDataPacket(uint32_t pktid);
    
public:   
   //! Constructor
   SysCoreSimulator(int rocid, int simtype);
   virtual ~SysCoreSimulator();

   bool initSockets();

   void mainLoop();   
   
   uint16_t poke (uint32_t addr, uint32_t val, uint8_t* rawdata);
   uint16_t peek (uint32_t addr, uint32_t *retVal);

    
   //! fill Data
   /*!
   * This is the main pattern generator. It is call once for every udp
   * packet sent to the SysCoreControl. 
   */
   void fillData(uint8_t* target, uint16_t numOfDatas);

   /*
   * This is pattern generator from Joern with two reproducable time peaks inside each epoch. 
    * Packets are not scramble. 
   */
   void fillDataJoern(uint8_t* target, uint16_t numOfDatas);

    /** This is generator of rare hits with sync messages */
   void fillRareSyncData(uint8_t* target, uint16_t numOfDatas);
    
     /** This is generator that shall emulate not so rare detector hits at random channels */
    void fillDetectorHitData (uint8_t* target, uint16_t numOfDatas);

    void makeSysMessage(uint8_t typ);

    void processDataRequest(SysCore_Data_Request_Full* pkt);
    
    /** produce random adc data with random channel and nxyter # JAM*/
    void fillAdcValue(uint8_t* target, bool lastepoch);
    
    /** increment nxTimestamp according to the simulated
     * hit distribution ,i.e. the event multiplicity
     * peak and width specifys gaussian? distribution*/
    void nextTimestamp(uint16_t peak, uint16_t width);
    
    /** produce numits random hit data at several random channels and nxyter # JAM*/
    void fillHitValues(uint8_t* target, int numhits, bool lastepoch);

    void nextHitTimestamp();
    
    void setLostRates(double recv = 0., double send = 0.);
   
};

#endif /*SYSCORESIMULATOR_H_*/
