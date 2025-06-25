// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#ifndef HADAQ_defines
#define HADAQ_defines

#include <cstdint>
#include <iostream>
#include <cstring>

/*
//Description of the Event Structure
//
//An event consists of an event header and of varying number of subevents, each with a subevent header. The size of the event header is fixed to 0x20 bytes
//
//Event header
//evtHeader
//evtSize   evtDecoding    evtId    evtSeqNr    evtDate  evtTime  runNr    expId
//
//    * evtSize - total size of the event including the event header, it is measured in bytes.
//    * evtDecoding - event decoding type: Tells the analysis the binary format of the event data, so that it can be handed to a corresponding routine for unpacking into a software usable data structure. For easier decoding of this word, the meaning of some bits is already predefined:
//      evtDecoding
//      msB    ---   ---   lsB
//      0   alignment   decoding type  nonzero
//          o The first (most significant) byte is always zero.
//          o The second byte contains the alignment of the subevents in the event. 0 = byte, 1 = 16 bit word, 2 = 32 bit word...
//          o The remaining two bytes form the actual decoding type, e.g. fixed length, zero suppressed etc.
//          o The last byte must not be zero, so the whole evtDecoding can be used to check for correct or swapped byte order.
//
//It is stated again, that the whole evtDecoding is one 32bit word. The above bit assignments are merely a rule how to select this 32bit numbers.
//
//    * evtId - event identifier: Tells the analysis the semantics of the event data, e.g. if this is a run start event, data event, simulated event, slow control data, end file event, etc..
//      evtId
//      31  30 - 16  15 - 12  11 - 8   5 - 7    4  3- 0
//      error bit    reserved    version  reserved    MU decision    DS flag  ID
//          o error bit - set if one of the subsystems has set the error bit
//          o version - 0 meaning of event ID before SEP03; 1 event ID after SEP03
//          o MU decision - 0 = negative LVL2 decision; >0 positive LVL2 decision
//            MU trigger algo result
//            1   negative decision
//            2   positive decision
//            3   positive decision due to too many leptons or dileptons
//            4   reserved
//            5   reserved
//          o DS flag - LVL1 downscaling flag; 1 = this event is written to the tape independent on the LVL2 trigger decision
//          o ID - defines the trigger code
//            ID before SEP03    description
//            0   simulation
//            1   real
//            2,3,4,5,6,7,8,9    calibration
//            13  beginrun
//            14  endrun
//
//            ID after SEP03  description
//            0   simulation
//            1,2,3,4,5    real
//            7,9    calibration
//            1   real1
//            2   real2
//            3   real3
//            4   real4
//            5   real5
//            6   special1
//            7   offspill
//            8   special3
//            9   MDCcalibration
//            10  special5
//            13  beginrun
//            14  endrun
//    * evtSeqNr - event number: This is the sequence number of the event in the file. The pair evtFileNr/evtSeqNr
//
//is unique throughout all events ever acquired by the system.
//
//    * evtDate - date of event assembly (filled by the event builder, rough precision):
//      evtDate   ISO-C date format
//      msB    ---   ---   lsB
//      0   year  month    day
//
//   1. The first (most significant) byte is zero
//   2. The second byte contains the years since 1900
//   3. The third the months since January [0-11]
//   4. The last the day of the month [1-31]
//
//    * evtTime - time of assembly (filled by the event builder, rough precision):
//      evtTime   ISO-C time format
//      msB    ---   ---   lsB
//      0   hour  minute   second
//
//   1. The first (most significant) byte is zero
//   2. The second byte contains the hours since midnight [0-23]
//   3. The third the minutes after the hour [0-59]
//   4. The last the seconds after the minute [0-60]
//
//    * runNr - file number: A unique number assigned to the file. The runNr is used as key for the RTDB.
//    * evtPad - padding: Makes the event header a multiple of 64 bits long.
*/

#pragma pack(push, 1)

namespace hadaq {

   enum EvtId {
      EvtId_data     = 0x00000001,
      EvtId_DABC     = 0x00002001,      // hades DAQVERSION=2 (same as evtbuild.c uses DAQVERSION=2)
      EvtId_runStart = 0x00010002,
      EvtId_runStop  = 0x00010003
   };

   enum EvtDecoding {
      EvtDecoding_default = 0x1,
      EvtDecoding_AloneSubevt = 0x8,   // subevent contains the only subsubevent, taking same id
      EvtDecoding_64bitAligned = (0x03 << 16) | 0x0001
   };


   /** \brief HADES transport unit header
    *
    * used as base for event and subevent
    * also common envelope for trd network data packets
    */

   struct HadTu {
      protected:
         uint32_t tuSize;
         uint32_t tuDecoding;

      public:

         HadTu() : tuSize(0), tuDecoding(0) {}

         /** msb of decode word is always non zero...? */
         inline bool IsSwapped() const  { return  tuDecoding > 0xffffff; }

         void SetDecodingDirect(uint32_t dec) { tuDecoding = dec; }

         inline uint32_t Value(const uint32_t *member) const
         {
            return IsSwapped() ? ((*member & 0xFF) << 24) |
                                 ((*member & 0xFF00) << 8) |
                                 ((*member & 0xFF0000) >> 8) |
                                 ((*member & 0xFF000000) >> 24) : *member;

            //return IsSwapped() ? ((((uint8_t *) member)[0] << 24) |
            //                      (((uint8_t *) member)[1] << 16) |
            //                      (((uint8_t *) member)[2] << 8) |
            //                      (((uint8_t *) member)[3])) : *member;
         }

         /** swapsave method to set value stolen from hadtu.h */
         inline void SetValue(uint32_t *member, uint32_t val)
         {
            *member = IsSwapped() ? ((val & 0xFF) << 24) |
                                    ((val & 0xFF00) << 8) |
                                    ((val & 0xFF0000) >> 8) |
                                    ((val & 0xFF000000) >> 24) : val;

            // *member = IsSwapped() ?
            //      ((((uint8_t *) &val)[0] << 24) |
            //      (((uint8_t *) &val)[1] << 16) |
            //      (((uint8_t *) &val)[2] << 8) |
            //      (((uint8_t *) &val)[3])) : val;
         }


         uint32_t GetDecoding() const { return Value(&tuDecoding); }
         inline uint32_t GetSize() const { return Value(&tuSize); }

         inline uint32_t GetPaddedSize() const
         {
            uint32_t hedsize = GetSize();
            uint32_t rest = hedsize % 8;
            return (rest == 0) ? hedsize : (hedsize + 8 - rest);
         }

         void SetSize(uint32_t bytes) { SetValue(&tuSize, bytes); }

         void SetDecoding(uint32_t decod) { SetValue(&tuDecoding, decod); }

         void Dump();
   };

   // ======================================================================

   /** \brief Intermediate hierarchy class as common base for event and subevent */

   struct HadTuId : public HadTu {
      protected:
         uint32_t tuId;

      public:

         HadTuId() : HadTu(), tuId(0)  {}

         inline uint32_t GetId() const { return Value(&tuId); }
         void SetId(uint32_t id) { SetValue(&tuId, id); }

         inline bool GetDataError() const { return (GetId() & 0x80000000) != 0; }

         void SetDataError(bool on)
         {
            if(on)
               SetId(GetId() | 0x80000000);
            else
               SetId(GetId() & ~0x80000000);
         }
   };

   // =================================================================================

   /** \brief Hadaq subevent structure

   Every event contains zero to unspecified many subevents. As an empty event is allowed, data outside of any subevent are not allowed. A subevents consists out of a fixed size subevent header and a varying number of data words.

       * The subevent header
         subEvtHeader
         subEvtSize   subEvtDecoding    subEvtId    subEvtTrigNr
             o subEvtSize - size of the subevent: This includes the the subevent header, it is measured in bytes.
             o subEvtDecoding - subevent decoding type: Tells the analysis the binary format of the subevent data, so that it can be handed to a corresponding routine for unpacking into a software usable data structure. For easier decoding of this word, the meaning of some bits is already predefined:
               subEvtDecoding
               msB    ---   ---   lsB
               0   data type   decoding type  nonzero
                   + The first (most significant) byte is always zero
                   + The second byte contains the word length of the subevent data. 0 = byte, 1 = 16 bit word, 2 = 32 bit word...
                   + The remaining two bytes form the actual decoding type, e.g. fixed length, zero suppressed etc.
                   + The last byte must not be zero, so the whole subEvtDecoding can be used to check for correct or swapped byte order. It is stated again, that the whole subEvtDecoding is one 32bit word. The above bit assignments are merely a rule how to select this 32bit numbers.
             o subEvtId - subevent identifier: Tells the analysis the semantics of the subevent data, e.g. every subevent builder may get its own subEvtId. So the data structure can be analyzed by the corresponding routine after unpacking.
               1-99   DAQ
               100-199   RICH
               200-299   MDC
               300-399   SHOWER
               400-499   TOF
               500-599   TRIG
               600-699   SLOW
               700-799   TRB_RPC  common TRB, but contents is RPC
               800-899   TRB_HOD  pion-hodoscope
               900-999   TRB_FW   forward wall
               1000-1099    TRB_START   start detector
               1100-1199    TRB_TOF  TOF detector
               1200-1299    TRB RICH    RICH detector

   Additionally, all subEvtIds may have the MSB set. This indicates a sub event of the corresponding id that contains broken data (e.g. parity check failed, sub event was too long etc.).

       *
             o subEvtTrigNr - subevent Trigger Number: This is the event tag that is produced by the trigger system and is used for checking the correct assembly of several sub entities. In general, this number is not counting sequentially. The lowest significant byte represents the trigger tag generated by the CTU and has to be same for all detector subsystems in one event. The rest is filled by the EB.
       * The data words: The format of the data words (word length, compression algorithm, sub-sub-event format) is defined by the subEvtDecoding and apart from this it is completely free. The meaning of the data words (detector, geographical position, error information) is defined by the subEvtId and apart from this it is completely unknown to the data acquisition system.
   */

      struct RawSubevent : public HadTuId  {

         protected:

            uint32_t subEvtTrigNr;

         public:

            RawSubevent() : HadTuId(), subEvtTrigNr(0) {}

            unsigned Alignment() const { return 1 << ( GetDecoding() >> 16 & 0xff); }

            uint32_t GetTrigNr() const { return Value(&subEvtTrigNr); }
            void SetTrigNr(uint32_t trigger) { SetValue(&subEvtTrigNr, trigger); }

            /* for trb3: each subevent contains trigger type in decoding word*/
            uint8_t GetTrigTypeTrb3() const { return (GetDecoding() & 0xF0) >> 4; }

            void SetTrigTypeTrb3(uint8_t typ)
            {
               uint32_t decod = GetDecoding();
               SetDecoding((decod & ~0xF0) | (typ << 4));
            }

            void Init(uint32_t trigger = 0)
            {
               SetTrigNr(trigger);
            }

            /** Return pointer on data by index - user should care itself about swapping */
            uint32_t* GetDataPtr(unsigned indx) const
            {
               return (uint32_t*) (this) + sizeof(RawSubevent)/sizeof(uint32_t) + indx;
            }

            /* returns number of payload data words, not maximum index!*/
            unsigned GetNrOfDataWords() const
            {
               unsigned datasize = GetSize() - sizeof(hadaq::RawSubevent);
               switch (Alignment()) {
                  case 4:  return datasize / sizeof(uint32_t);
                  case 2:  return datasize / sizeof(uint16_t);
               }
               return datasize / sizeof(uint8_t);
            }

            /** swap-save access to any data. stolen from hadtu.h */
            uint32_t Data(unsigned idx) const
            {
               const void* my = (char*) (this) + sizeof(hadaq::RawSubevent);

               switch (Alignment()) {
                  case 4:
                     return Value((uint32_t *) my + idx);

                  case 2: {
                     uint16_t tmp = ((uint16_t *) my)[idx];

                     if (IsSwapped()) tmp = ((tmp >> 8) & 0xff) | ((tmp << 8) & 0xff00);

                     return tmp;
                  }
               }

               return ((uint8_t*) my)[idx];
            }

            uint32_t GetErrBits()
            {
               return Data(GetNrOfDataWords()-1);
            }

            void CopyDataTo(void* buf, unsigned indx, unsigned datalen)
            {
               if (!buf) return;
               while (datalen-- > 0) {
                  *((uint32_t*) buf) = Data(indx++);
                  buf = ((uint32_t*) buf) + 1;
               }
            }

            /** Return pointer where raw data should starts */
            void* RawData() const { return (char*) (this) + sizeof(hadaq::RawSubevent); }

            /** Print subevent header and optionally raw data */
            void Dump(bool print_raw_data = false);

            /** Print raw data, optionally one can position and portion to print */
            void PrintRawData(unsigned ix = 0, unsigned len = 0xffffffff, unsigned prefix = 6);
      };

   // =================================================================================

/** \brief Hadaq event structure

 Description of the Event Structure

An event consists of an event header and of varying number of subevents, each with a subevent header. The size of the event header is fixed to 0x20 bytes

Event header
evtHeaderc
evtSize   evtDecoding    evtId    evtSeqNr    evtDate  evtTime  runNr    expId

    * evtSize - total size of the event including the event header, it is measured in bytes.
    * evtDecoding - event decoding type: Tells the analysis the binary format of the event data, so that it can be handed to a corresponding routine for unpacking into a software usable data structure. For easier decoding of this word, the meaning of some bits is already predefined:
      evtDecoding
      msB    ---   ---   lsB
      0   alignment   decoding type  nonzero
          o The first (most significant) byte is always zero.
          o The second byte contains the alignment of the subevents in the event. 0 = byte, 1 = 16 bit word, 2 = 32 bit word...
          o The remaining two bytes form the actual decoding type, e.g. fixed length, zero suppressed etc.
          o The last byte must not be zero, so the whole evtDecoding can be used to check for correct or swapped byte order.

It is stated again, that the whole evtDecoding is one 32bit word. The above bit assignments are merely a rule how to select this 32bit numbers.

    * evtId - event identifier: Tells the analysis the semantics of the event data, e.g. if this is a run start event, data event, simulated event, slow control data, end file event, etc..
      evtId
      31  30 - 16  15 - 12  11 - 8   5 - 7    4  3- 0
      error bit    reserved    version  reserved    MU decision    DS flag  ID
          o error bit - set if one of the subsystems has set the error bit
          o version - 0 meaning of event ID before SEP03; 1 event ID after SEP03
          o MU decision - 0 = negative LVL2 decision; >0 positive LVL2 decision
            MU trigger algo result
            1   negative decision
            2   positive decision
            3   positive decision due to too many leptons or dileptons
            4   reserved
            5   reserved
          o DS flag - LVL1 downscaling flag; 1 = this event is written to the tape independent on the LVL2 trigger decision
          o ID - defines the trigger code
            ID before SEP03    description
            0   simulation
            1   real
            2,3,4,5,6,7,8,9    calibration
            13  beginrun
            14  endrun

            ID after SEP03  description
            0   simulation
            1,2,3,4,5    real
            7,9    calibration
            1   real1
            2   real2
            3   real3
            4   real4
            5   real5
            6   special1
            7   offspill
            8   special3
            9   MDCcalibration
            10  special5
            13  beginrun
            14  endrun
    * evtSeqNr - event number: This is the sequence number of the event in the file. The pair evtFileNr/evtSeqNr

is unique throughout all events ever acquired by the system.

    * evtDate - date of event assembly (filled by the event builder, rough precision):
      evtDate   ISO-C date format
      msB    ---   ---   lsB
      0   year  month    day

   1. The first (most significant) byte is zero
   2. The second byte contains the years since 1900
   3. The third the months since January [0-11]
   4. The last the day of the month [1-31]

    * evtTime - time of assembly (filled by the event builder, rough precision):
      evtTime   ISO-C time format
      msB    ---   ---   lsB
      0   hour  minute   second

   1. The first (most significant) byte is zero
   2. The second byte contains the hours since midnight [0-23]
   3. The third the minutes after the hour [0-59]
   4. The last the seconds after the minute [0-60]

    * runNr - file number: A unique number assigned to the file. The runNr is used as key for the RTDB.
    * evtPad - padding: Makes the event header a multiple of 64 bits long.
*/

   struct RawEvent : public HadTuId  {

      protected:
         uint32_t evtSeqNr;
         uint32_t evtDate;
         uint32_t evtTime;
         uint32_t evtRunNr;
         uint32_t evtPad;

         /** Method to set initial header value like decoding and date/time */
         void InitHeader(uint32_t evid);


      public:

         RawEvent() : HadTuId(), evtSeqNr(0), evtDate(0), evtTime(0), evtRunNr(0), evtPad(0) {}

         uint32_t GetSeqNr() const { return Value(&evtSeqNr); }
         void SetSeqNr(uint32_t n) { SetValue(&evtSeqNr, n); }

         int32_t GetRunNr() const { return Value(&evtRunNr); }
         void SetRunNr(uint32_t n) { SetValue(&evtRunNr, n); }

         int32_t GetDate() const { return Value(&evtDate); }
         void SetDate(uint32_t d) { SetValue(&evtDate, d); }

         int32_t GetTime() const { return Value(&evtTime); }
         void SetTime(uint32_t t) { SetValue(&evtTime, t); }

         void Init(uint32_t evnt, uint32_t run=0, uint32_t id=EvtId_DABC)
         {
            InitHeader(id);
            SetSeqNr(evnt);
            SetRunNr(run);
            evtPad = 0;
         }

         void Dump();

         RawSubevent* NextSubevent(RawSubevent* prev = nullptr);

         RawSubevent* FirstSubevent();

         uint32_t AllSubeventsSize();
   };

}

#pragma pack(pop)

#endif
