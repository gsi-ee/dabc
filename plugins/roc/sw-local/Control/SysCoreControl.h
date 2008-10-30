#ifndef SYSCORECONTROL_H_
#define SYSCORECONTROL_H_

//============================================================================
/*! \file SysCoreControl.h
 *   \date  12-05-2008
 *  \version 1.7
 *   \author Stefan Mueller-Klieser <stefanmk@kip.uni-heidelberg.de>
 *
 * copyright: Kirchhoff-Institut fuer Physik
 *
 * This file defines all high level classes used to communicate with the
 * ROC (Read Out Controller which is here synonymously used for SysCore Board)
 */
//============================================================================

//Doxygen Main Page

/*! \mainpage Documentation of the SysCoreControl class version 1.7
 *
 * \section intro_sec Introduction
 *
 * The SysCoreControl class is a PC interface for Ethernet communication with the
 * SysCore Board.
 * The SysCore Board is a Virtex4FX-20 custom board developed at
 * Kirchhoff-Institut fuer Physik in Heidelberg, Germany. It is designed to be
 * used as a digital readout controller for detectors in particle physics.
 *
 * \section Changelog
 *
 * \subsection since Version 0.6
 *
 *  - modified tranport protocol
 *  - one data socket per board
 *  - fully-configurable ports numbers for host and ROC
 *  - new method uplodSDfile to overwrite existing files on SD card
 *  - saveConfig() and loadConfig() methods to store config 
 *    on ROC side with specified file name
 *  - display of ROC debug output (including statistic block)
 *  - new possibility over callback inform user that data is arrived,
 *    used in DABC to implement non-blocking transport
 *  - bug fixes in data transport to improve performance
 *  - fully redesigned makefile
 *  - new SysCoreSorter class to sort in time all data, comming from ROC
 *  - use ring data buffer in SysCoreBoard class to reduce memcpy, allocation 
 *    and, of course, improve performance
 *
 * \subsection since Version 0.5 (v 0.4 was skipped)
 *
 *  - new retransmission logic with more efficient data list handling
 *  - new waitData and fillData methods - both blocking and non-blocking modes are supported
 *  - improved isDataInBuffer() and getNextData() - less locking and blocking mode now supported
 *  - better algorithm to define which data can be provided to user
 *  - interface change - no any longer public controller.board vector, either controller.board() or controller[] must be used
 *  - generic cleanup of interface, simplify getter methods, reduce number of constructors
 *  - handling of empty packets together with normal
 *  - startDaq/stopDaq methods to start/stop DAQ. startDaq completely reset
 *    DAQ on both PC and ROC side and activate data transport of data from ROC. stopDaq
 *    poke stop signal to ROC but not immediately stops data transport from the ROC -
 *    only when stop message is coming, data transport will be cut. To force it from user side,
 *    resetDaq method can be called
 *  - peek/poke can be used during daq running - was a problem of packet id
 *
 * \subsection since Version 0.3
 *
 *  - merge with Version 0.2x by Sergei Linev <S.Linev@gsi.de> wich changed:
 *  - New ptherad_condition in SysCoreBoard class to be used from dabc side
 *    for waiting of the next portion of data
 *  - Correctly destroy mutex and condition in SysCoreBoard destructor
 *  - Implement method CheckCreditsStatus() which send new credit to the ROC if required
 *  - Modify getNextData() method that it returns false when no data is available
 *  - Comment out packetHistory list - was a reason for high memory consumption
 *  - New .h and .cpp File for SysCoreData. Make several methods in SysCoreData
 *    class inline to improve speed.
 *  - Add several setters methods in SysCoreData to be able to change some values
 *
 * \subsection since Version 0.2
 *
 *  - rewrite most of the retransmission code. If packets get lost, data readout
 *    is blocked completely until all packets were retransmitted.
 *  - added a lot output for Debugging
 *  - connections are generally made more robust with number connection retries
 *    DEFINE found in defines.h
 *  - DEBUG_LEVEL define for code output silencing
 *  - added timer functionality in SysCoreUtility
 *  - merged bugfixes by Joern Adamczewski <J.Adamczewski@gsi.de>
 *
 * \section install_sec Installation
 *
 * \subsection Linux Building on Linux
 *
 * - Connect the SysCoreBoard to the same physical ethernet subnet as your host pc
 * - Go to the "/Release" subfolder, type "make"
 * - run ./setup, follow the steps
 * - edit the SysCoreControlApp.cpp, this is the app you should work with
 * - go to the "/Release" subfolder type "make" again
 * - the SysCoreControlApp.cpp compiles to SysCoreControl application
 * - run ./SysCoreControl
 *
 *
 */

#include "SysCoreBoard.h"

#include <vector>

class SccCommandSocket;
class SocketHandler;
class StdoutLog;
class Mutex;


//! SysCore Control class
/*!
 * This is the toplevel class. Use it in your application to communicate with the ROC boards.
 */
class SysCoreControl
{
   friend class SccCommandSocket;
   friend class SysCoreBoard;

private:
   uint16_t       controlPort_;
   bool           displayConsoleOutput_;

   StdoutLog      *stOutLog_;
   Mutex          *socketHandlerMutex_;
   SocketHandler  *socketHandler_;

   SccCommandSocket *cmdSocket_;

   pthread_t fetcherThread_;

   pthread_mutex_t ctrlMutex_;
   pthread_cond_t ctrlCond_;
   int  ctrlState_;     // peek/poke state 0 - doing nothing (all received packets discard), 1 - waiting for reply, 2 - get packet from ROC

   bool fbFetcherStop;

   std::vector<SysCoreBoard*> boards_;

   void initSockets();
   void startFetcherThread();
   void stopFetcherThread();
   
   void processCtrlMessage(int boardnum, SysCore_Message_Packet* pkt, unsigned len);

   //! Perform communiction loop to the ROC
   /*!
   * Sends packet to ROC and waits for response.
    * If during single_tmout_sec no data is arrived, packet will be transmitted again
    * If total_tmout_sec is expired, error code is return
   */
   bool performCtrlLoop(SysCoreBoard* brd, double total_tmout_sec, bool show_progress = false);
   
   static uint16_t gControlPortBase;
   
protected:

   virtual void DataCallBack(SysCoreBoard* brd) {}

public:
   //! Constructor
   /*!
    * \param controlPort should be set to nonhostIp Host Ip address to use.
    * Only required by simulator - ROC just reply always on requested IP.
    * It should be in string decimal format "127.0.0.1"
    */
   SysCoreControl();

   virtual ~SysCoreControl();

   //don't use it, it is for multithreading support
   void fetcher();

   /********************************************************************************
    * Host Configuration
    ********************************************************************************/
   inline uint16_t getControlPort() const { return controlPort_; }

   //! Show Remote Debug Output
   /*!
   * Chooses if debug output from ROC will be print to stdout
   */
   void showRemoteDebugOutput(bool b) { displayConsoleOutput_ = b; }
   /********************************************************************************
    * Slowcontrol
    ********************************************************************************/
   //! addBoard
   /*!
   * \param address SysCore Board IP Address
   * \param ctlPort UDP Port to use for Control packets
   *
   *  Initialisize a new SysCore Board Class instance, and try
   * to connect to SysCore Board with given IP and Port.
   */
   int addBoard(const char* address, bool asmaster = true, uint16_t portnum = ROC_DEFAULT_CTRLPORT);

   //! Delete all board instances
   void deleteBoards(bool logout = true);

   //! Get number of currently used ROCs
   /*!
   * Tells you how many boards are now conected.
   */
   inline unsigned getNumberOfBoards() const { return boards_.size(); }

   inline bool isValidBoardId(unsigned num) const { return num < getNumberOfBoards(); }

   inline bool isValidBoardNum(unsigned num) const { return num < getNumberOfBoards(); }

   //! Return reference on SysCoreBoard object
   inline SysCoreBoard& board(unsigned num) { return *(boards_.at(num)); }

   //! Return reference on SysCoreBoard object
   inline SysCoreBoard& operator[](unsigned num) { return board(num); }


   //! Get Boardnumber by IP
   /*!
   * Searches the stack of added boards for the board with the specified IP.
   */
   int getBoardNumByIp(uint32_t address);
   int getBoardNumByIp(const char* address);

   //! setControlPortBase
   /*!
   * Set base for control port, used by SysCoreControl classes.
   * Change this value only if default number (5737) not works on your system
   */
   static void setControlPortBase(uint16_t base) { gControlPortBase = base; }
};

#endif /*SYSCORECONTROL_H_*/
