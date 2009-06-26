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

#ifndef ROC_Board
#define ROC_Board

#include <stdint.h>

#include "nxyter/Data.h"

namespace roc {

   enum BoardRole { roleNone, roleObserver, roleMaster, roleDAQ };

   // this is value of subevent iProc in mbs event
   // For raw data rocid stored in iSubcrate, for merged data iSubcrate stores higher bits
   enum ERocMbsTypes {
      proc_RocEvent     = 1,   // complete event from one roc board (iSubcrate = rocid)
      proc_ErrEvent     = 2,   // one or several events with corrupted data inside (iSubcrate = rocid)
      proc_MergedEvent  = 3    // sorted and synchronized data from several rocs (iSubcrate = upper rocid bits)
   };

   extern const char* xmlNumRocs;
   extern const char* xmlRocPool;
   extern const char* xmlTransportWindow;
   extern const char* xmlBoardIP;
   extern const char* xmlRole;

   extern const char* typeUdpDevice;

   enum ERocBufferTypes {
      rbt_RawRocData     = 234
   };

   class ReadoutModule;

   class Board {
      protected:
         BoardRole      fRole;
         uint32_t       fRocNumber;
         double         fDefaultTimeout;

         ReadoutModule *fReadout;

         Board();
         virtual ~Board();

         void SetReadout(ReadoutModule* m) { fReadout = m; }

         virtual void* getdeviceptr() = 0;

         void ShowOperRes(const char* oper, uint32_t addr, uint32_t value, int res);

      public:

         BoardRole Role() const { return fRole; }
         uint32_t rocNumber() const { return fRocNumber; }

         void setDefaultTmout(double tmout = 2.) { fDefaultTimeout = tmout>0 ? tmout : 1.; }
         double getDefaultTmout() const { return fDefaultTimeout; }

         virtual int put(uint32_t addr, uint32_t value, double tmout = 0.) = 0;
         virtual int get(uint32_t addr, uint32_t& value, double tmout = 0.) = 0;

         inline int poke(uint32_t addr, uint32_t value, double tmout = 0.)
            { return put(addr, value, tmout); }
         inline int peek(uint32_t addr, uint32_t& value, double tmout = 0.)
            { return get(addr, value, tmout); }


         // --------------------------------------------------------------------
         // methods for controlling ROC and components
         // --------------------------------------------------------------------

         static const char* VersionToStr(uint32_t ver);

         virtual uint32_t getSW_Version();

         //! getROC_Number
         /*!
          * Returns the Number of the ROC.
          */
         uint32_t getROC_Number();

         //! setROC_Number
         /*!
          * Set the Number of the ROC.
          */
         void setROC_Number(uint32_t num);

         //! getHW_Version
         /*!
          * Returns the Hardware-Version of the ROC.
          */
         uint32_t getHW_Version();


         //! getFIFO_full
         /*!
          * Returns, if the FIFO is full. (1 is Yes, 0 is No)
          */
         uint32_t getFIFO_full();


         //! getFIFO_empty
         /*!
          * Returns, if the FIFO is empty. (1 is Yes, 0 is No)
          */
         uint32_t getFIFO_empty();


         //! reset_FIFO
         /*!
          * Clears the ROC-FIFO.
          * This impacts ALL data, that means ALL hit messages and all auxiliary data.
          */
         void reset_FIFO();

         //! setParityCheck
         /*!
          * \param val Is parity checking on? (1 is yes, 0 is no)
          *
          * Activates/Deactivates the parity checking.
          * If parity checking is activated, the broken hit message is replaced by a hit parity error message.
          */
         void setParityCheck(uint32_t val);

         //! RESET
         /*!
          * Resets and reconfigures the whole ROC.
          */
         void RESET();

         void DEBUG_MODE(uint32_t val);

         //! TestPulse
         /*!
          * \param delay The testpulse sequence starts delay * 4 ns after the reset
          * \param length The rising testpulse edges occur every length * 4 ns
          * \param number number of rising testpulse edges (0 represents an eternal testpulse sequence)
          *
          * Resets all nXYTERs.
          * Then, generates a test pulse sequence.
          */
         void TestPulse(uint32_t delay, uint32_t length, uint32_t number);


         //! TestPulse
         /*!
          * \param length The rising testpulse edges occur every length * 4 ns
          * \param number number of rising testpulse edges (0 represents an eternal testpulse sequence)
          *
          * Generates a test pulse sequence instantly, without reset.
          */
         void TestPulse(uint32_t length, uint32_t number);


         //! reset_NX
         /*!
          * Resets all connected nXYTERs.
          */
         void NX_reset();

         //! activate
         /*!
          * Actiavte/deactive nXYTERs.
          */
         void NX_setActive(int nx1, int nx2, int nx3, int nx4);

         void NX_setActive(uint32_t mask);
         uint32_t NX_getActive();

         //! setLTS_Delay
         /*!
          * Sets the global LTS Delay.
          * Please, only use this function, if know what you're doing!
          */
         void setLTS_Delay(uint32_t val);

         uint32_t getTHROTTLE();
         uint32_t GPIO_getCONFIG();
         void GPIO_setCONFIG(uint32_t mask);
         void GPIO_setCONFIG(int gpio_nr, int additional, int throttling, int falling, int rising);
         void GPIO_setMHz(int gpio_nr, int MHz);
         void GPIO_setBAUD(int gpio_nr, uint32_t BAUD_START, uint32_t BAUD1, uint32_t BAUD2);
         void GPIO_setScaledown(uint32_t val);
         static bool GPIO_packCONFIG(uint32_t& mask, int gpio_nr, int additional, int throttling, int falling, int rising);
         static bool GPIO_unpackCONFIG(uint32_t mask, int gpio_nr, int& additional, int& throttling, int& falling, int& rising);

         // DAQ functions


         //! getLowHighWater
         /*!
          * \param lowWater buffer amount (KB), after which daq in ROC is resuming
          * \param highWater buffer amount (KB), after which daq in ROC is suspending
          * Sets low and high water markers in ROC, applicabe only for UDP
          */
         virtual bool getLowHighWater(int& lowWater, int& highWater) = 0;

         //! setLowHighWater
         /*!
          * \param lowWater buffer amount (KB), after which daq in ROC is resuming
          * \param highWater buffer amount (KB), after which daq in ROC is suspending
          * Sets low and high water markers in ROC, applicabe only for UDP
          */
         virtual bool setLowHighWater(int lowWater, int highWater) = 0;

         //! setUseSorter
         /*!
          * Enable / disable usage of time sorter for nxyter data
         */
         void setUseSorter(bool on = true);

         //! startDaq
         /*!
         * This function should be called when starting DAQ, not only poke(ROC_START_DAQ, 1)
         * It reset all counter, initialize buffer and at the end start DAQ
         * \param trWindow sets number of udp packet, which can be transported by board without confirmation
         */
         bool startDaq(unsigned = 40);

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

         //! resetDaq
         /*!
         * Cleanups all data on client side, brakes any data transport,
         * do not sends any command like stop daq to ROC. Always called by startDaq()
         * Kept for compatibility reasons
         */
         void resetDaq() {}



         bool getNextData(nxyter::Data& data, double tmout = 1.);

         static Board* Connect(const char* name, BoardRole role = roleDAQ);
         static bool Close(Board* brd);
   };

}

#endif
