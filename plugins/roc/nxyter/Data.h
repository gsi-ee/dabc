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

#ifndef NXYTER_Data
#define NXYTER_Data

#include <stdint.h>

// MESSAGE TYPES
#define ROC_MSG_NOP        0
#define ROC_MSG_HIT        1
#define ROC_MSG_EPOCH      2
#define ROC_MSG_SYNC       3
#define ROC_MSG_AUX        4
#define ROC_MSG_SYS        7

//! nxyter::Data class
/*!
 * This data class contains the 48 databits of one ROC message. Use
 * the member functions to access the bits.
 */

namespace nxyter {

   class Data {

      private:

         uint8_t data[6];

      public:

         Data();
         Data(const Data& src);

         void assignData(const uint8_t*);

         // print data to std output
         // Parameter kind is mask with 3 bits
         // first bit (1) - print rocid and type
         // second bit (2) - print specific fields
         // third bit (4) - print raw data
         void printData(unsigned kind = 3) const;

         inline uint8_t* getData() const { return (uint8_t*) data; }
         const char* getCharData();

         //! for all MessageTypes, 3 bits
         inline uint8_t getMessageType() const
            { return (data[0] >> 5) & 0x7; }

         //! for all MessageTypes, 3 bits
         inline uint8_t getRocNumber() const
            { return (data[0] >> 2) & 0x7; }

         inline void setMessageType(uint8_t v)
            { data[0] = (data[0] & (~(0x7 << 5))) | ((v & 0x7) << 5); }

         inline void setRocNumber(uint8_t v)
            { data[0] = (data[0] & (~(0x7 << 2))) | ((v & 0x7) << 2); }


         // _________________________ Hit Data _________________________

         // for HitData MessageType, 2 bits
         inline uint8_t getNxNumber() const
            { return data[0] & 0x3; }

         //! for HitData MessageType, 3 bits
         inline uint8_t getMsbOfLts() const
            { return (data[1] >> 5) & 0x7; }

         //! for HitData MessageType, 14 bits
         inline uint16_t getNxTs() const
         {
            return ((data[1] & 0x1f) << 9) |
                    (data[2] << 1) |
                    (data[3] >> 7);
         }

         //! for HitData MessageType, 7 bits
         inline uint8_t getId() const
            {  return data[3] & 0x7f; }

         // 1 bit unused

         //! for HitData MessageType, 12 bits
         inline uint16_t getAdcValue() const
         {
            return ((data[4] & 0x7f) << 5) |
                   ((data[5] & 0xf8) >> 3);
         }

         //! for HitData MessageType, 1 bit
         inline uint8_t getPileUp() const
            { return (data[5] >> 2) & 0x1; }

         //! for HitData MessageType, 1 bit
         inline uint8_t getOverFlow() const
            { return (data[5] >> 1) & 0x1; }

         //! for HitData MessageType, 1 bit
         inline uint8_t getLastEpoch() const
           { return data[5] & 0x1; }


         inline void setNxTs(uint16_t v)
         {
            data[1] = (data[1] & ~0x1f) | ((v >> 9) & 0x1f);
            data[2] = v >> 1;
            data[3] = (data[3] & ~0x80) | (v << 7);
         }

         inline void setLastEpoch(uint8_t v)
           { data[5] = (data[5] & ~0x1) | (v & 0x1); }


         // _________________________ Epoch marker _________________________

         // 2 bit unused
         //! for Epoch Marker MessageType, 32 bit
         inline uint32_t  getEpoch() const
         {
            return (data[1] << 24) |
                   (data[2] << 16) |
                   (data[3] << 8) |
                    data[4];
         }

         //! for Epoch Marker MessageType, 8 bit
         inline uint8_t  getMissed() const
         {
            return data[5];
         }

         inline void setEpoch(uint32_t v)
         {
             data[1] = (v >> 24) & 0xff;
             data[2] = (v >> 16) & 0xff;
             data[3] = (v >> 8) & 0xff;
             data[4] = v & 0xff;
         }

         inline void setMissed(uint8_t v)
            { data[5] = v; }


         // _________________________ Sync marker _________________________


         // channel number of sync marker, 2 bit
         inline uint8_t getSyncChNum() const
            {  return data[0] & 0x3; }

         // sync message return lower bit of epoch to which sync master belongs to, 1 bit
         inline uint8_t getSyncEpochLowBit() const
         {
            return data[1] >> 7;
         }

         // Sync time stamp, 13 bit, return stamp in ns (last bit always 0)
         inline uint16_t getSyncTs() const
         {
            // return (data[1] << 6) |  (data[2] >> 2);

            return ((data[1] & 0x7F) << 7) | ((data[2] & 0xfc) >> 1);
         }

         // sync message event number, 24 bit
         inline uint32_t getSyncEvNum() const
         {
            return ((data[2] & 0x3) << 22) |
                    (data[3] << 14) |
                    (data[4] << 6) |
                    (data[5] >> 2);
         }

         // sync message status flafs, 2 bit
         inline uint8_t getSyncStFlag() const
         {
            return data[5] & 0x3;
         }

         inline void setSyncEpochLowBit(uint8_t v)
         {
            data[1] = (data[1] & 0x7f) | ((v & 0x1) << 7);
         }

         // set value of sync ts in ns, lower bit is ignored
         inline void setSyncTs(uint16_t v)
         {
            data[1] = (data[1] & 0x80) | ((v >> 7) & 0x7f);
            data[2] = (data[2] & ~0xfc) | ((v << 1) & 0xfc);
         }

         inline void setSyncEvNum(uint32_t v)
         {
            data[2] = (data[2] & ~0x3) | ((v >> 22) & 0x3);
            data[3] = v >> 14;
            data[4] = v >> 6;
            data[5] = (data[5] & ~0xfc) | (v << 2);
         }


         // _________________________ AUX marker _________________________

         // AUX channel number, 7 bit
         inline uint8_t getAuxChNum() const
         {
            return ((data[0] & 0x3) << 5) |
                   ((data[1] & 0xf8) >> 3);
         }

         // lower bit of epoch when AUX was taken, 1 bit
         inline uint8_t getAuxEpochLowBit() const
         {
            return (data[1] & 0x4) >> 2;
         }

         // AUX time stamp, 13 bit, returns stamp in ns (last bit always 0)
         inline uint16_t getAuxTs() const
         {
   //         return ((data[1] & 0x7) << 11) |
   //                 (data[2] << 3) |
   //                ((data[3] & 0xe0) >> 5);

            return ((data[1] & 0x3) << 12) |
                    (data[2] << 4) |
                   ((data[3] & 0xe0) >> 4);
         }

         // AUX rising or failing edge, 1 bit
         inline uint8_t getAuxFalling() const
         {
            return (data[3] >> 4) & 1;
         }

         inline void setAucChNum(uint8_t v)
         {
            data[0] = (data[0] & ~0x3) | ((v >> 5) & 0x3);
            data[1] = (data[1] & ~0xf8) | (v << 3);
         }

         inline void setAuxEpochLowBit(uint8_t v)
         {
            data[1] = (data[1] & ~0x4) | ((v & 0x1) << 2);
         }

         inline void setAuxTs(uint16_t v)
         {
   //         data[1] = (data[1] & ~0x7) | ((v >> 11) & 0x7);
   //         data[2] = v >> 3;
   //         data[3] = (data[3] & ~0xe0) | (v << 5);

            data[1] = (data[1] & ~0x3) | ((v >> 12) & 0x3);
            data[2] = (v >> 4) & 0xFF;
            data[3] = (data[3] & ~0xE0) | ((v << 4) & 0xE0);
         }


         // _________________________ System message _________________________

         // 2 bit unused
         //! for System Message MessageType, 8 bit
         inline uint8_t getSysMesType() const
            { return data[1]; }

         //! for System Message MessageType, 32 bit
         /*!
          * In future versions it will contain the starting and ending epoch.
          */
         inline uint32_t getSysMesEpoch() const
         {
            return (data[2] << 24) |
                   (data[3] << 16) |
                   (data[4] << 8) |
                    data[5];
         }

         inline void setSysMesType(uint8_t v)
         {
            data[1] = v;
         }

         // ____________________________ common function ______________________

         inline bool isNopMsg() const { return getMessageType() == ROC_MSG_NOP; }
         inline bool isHitMsg() const { return getMessageType() == ROC_MSG_HIT; }
         inline bool isEpochMsg() const { return getMessageType() == ROC_MSG_EPOCH; }
         inline bool isSyncMsg() const { return getMessageType() == ROC_MSG_SYNC; }
         inline bool isAuxMsg() const { return getMessageType() == ROC_MSG_AUX; }
         inline bool isSysMsg() const { return getMessageType() == ROC_MSG_SYS; }

         // return full time stamp for   hit, sync, aux   messages
         inline uint64_t getMsgFullTime(uint32_t epoch) const
         {
            switch (getMessageType()) {
               case ROC_MSG_HIT:
                  return FullTimeStamp(getLastEpoch() ? epoch - 1 : epoch, getNxTs());
               case ROC_MSG_EPOCH:
                  return FullTimeStamp(getEpoch(), 0);
               case ROC_MSG_SYNC:
                  return FullTimeStamp((getSyncEpochLowBit() == (epoch & 0x1)) ? epoch : epoch - 1, getSyncTs());
               case ROC_MSG_AUX:
                  return FullTimeStamp((getAuxEpochLowBit() == (epoch & 0x1)) ? epoch : epoch - 1, getAuxTs());
               default:
                  return FullTimeStamp(epoch, 0);
            }
            return 0;
         }

         inline bool isStartDaqMsg() const { return isSysMsg() && (getSysMesType()==1); }
         inline bool isStopDaqMsg() const { return isSysMsg() && (getSysMesType()==2); }

         // __________________________________________________________________

         inline static uint64_t FullTimeStamp(uint32_t epoch, uint16_t stamp)
         {
             return ((uint64_t) epoch << 14) | (stamp & 0x3fff);
         }

         static uint64_t CalcDistance(uint64_t start, uint64_t stop);
   };
}

#endif

