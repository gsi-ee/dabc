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

#ifndef HADAQ_HadaqTypeDefs
#define HADAQ_HadaqTypeDefs

#include <fstream>
#include <stdint.h>
//#include "dabc/logging.h"

#define HLD__SUCCESS        0
#define HLD__FAILURE        1
#define HLD__CLOSE_ERR      3
#define HLD__NOFILE      2
#define HLD__NOHLDFILE   4
#define HLD__EOFILE      5
#define HLD__FULLBUF      6
#define HLD__WRITE_ERR      7
//#define PUTHLD__FILE_EXIST  101
//#define PUTHLD__TOOBIG      102
//#define PUTHLD__OPEN_ERR    103
//#define PUTHLD__EXCEED      104


#define HADAQ_TIMEOFFSET 1200000000 /* needed to reconstruct time from runId */


namespace hadaq {

   typedef uint32_t EventNumType;
   typedef uint32_t RunId;

   enum EvtId {
      EvtId_data = 1,
      EvtId_DABC = 0x00003001,      // hades DAQVERSION=3 (evtbuild.c uses DAQVERSION=2)
      EvtId_runStart = 0x00010002,
      EvtId_runStop = 0x00010003
   };

   enum EvtDecoding {
      EvtDecoding_default = 1,
      EvtDecoding_64bitAligned = (0x03 << 16) | 0x0001
   };

   enum EHadaqBufferTypes {
      mbt_HadaqEvents = 142,        // event/subevent structure
      mbt_HadaqTransportUnit = 143  //plain hadtu container with single subevents

   };


#pragma pack(push, 1)

   /*
    * HADES transport unit header
    * used as base for event and subevent
    * also common envelope for trd network data packets
    */

   struct HadTu {
         int32_t tuSize;
         int32_t tuDecoding;

         /* msb of decode word is always non zero...?*/
         bool IsSwapped()
         {
            return (tuDecoding > 0xffffff);
         }

         /* swapsave access to any data stolen from hadtu.h*/
         int32_t Value(int32_t *member)
         {

            if (IsSwapped()) {
               uint32_t tmp0;
               uint32_t tmp1;

               tmp0 = *member;
               ((char *) &tmp1)[0] = ((char *) &tmp0)[3];
               ((char *) &tmp1)[1] = ((char *) &tmp0)[2];
               ((char *) &tmp1)[2] = ((char *) &tmp0)[1];
               ((char *) &tmp1)[3] = ((char *) &tmp0)[0];
               return tmp1;
            } else {
               return *member;
            }
         }

         void SetValue(int32_t *member, int32_t val)
         {

            if (IsSwapped()) {
               uint32_t tmp0;
               tmp0 = *member;
               ((char *) &tmp0)[3] = ((char *) &val)[0];
               ((char *) &tmp0)[2] = ((char *) &val)[1];
               ((char *) &tmp0)[1] = ((char *) &val)[2];
               ((char *) &tmp0)[0] = ((char *) &val)[3];

            } else {
               *member = val;
            }
         }

         size_t GetSize()
         {
            return (size_t)(Value(&tuSize));
         }

         size_t GetPaddedSize()
            {
               size_t hedsize = GetSize();
               // TODO: take actual decoding into account
               // account padding of events to 8 byte boundaries:
               while ((hedsize % 8) != 0) {
                  hedsize++;
                  //DOUT0(("hadtu GetSize() extends for padding the length to %d",hedsize));
               }
               return hedsize;
            }


         void SetSize(size_t bytes)
         {
            SetValue(&tuSize, (uint32_t) bytes);
         }

         int32_t GetDecoding()
         {
            return Value(&tuDecoding);
         }


   };

   /*
    * Intermediate hierarchy class as common base for event and subevent
    */
   struct HadTuId : public HadTu {
           int32_t tuId;

    int32_t GetId()
         {
            return Value(&tuId);
         }

         void SetId(uint32_t id)
         {
            SetValue(&tuId, id);
         }

    bool GetDataError()
       {
             return ((GetId() & 0x80000000UL) != 0);
       }

    void SetDataError(bool on)
       {
          if(on)
             SetId(GetId() | 0x80000000UL);
          else
             SetId(GetId() & ~0x80000000UL);
       }

   };

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

   struct Event: public HadTuId {
         int32_t evtSeqNr;
         int32_t evtDate;
         int32_t evtTime;
         int32_t evtRunNr;
         int32_t evtPad;

         int32_t GetSeqNr()
         {
            return Value(&evtSeqNr);
         }

         void SetSeqNr(int32_t n)
         {
            SetValue(&evtSeqNr, n);
         }

         int32_t GetRunNr()
         {
            return Value(&evtRunNr);
         }

         void SetRunNr(int32_t n)
         {
            SetValue(&evtRunNr, n);
         }

         int32_t GetDate()
         {
            return Value(&evtDate);
         }

         void SetDate(int32_t d)
         {
            SetValue(&evtDate, d);
         }

         int32_t GetTime()
         {
            return Value(&evtTime);
         }

         void SetTime(int32_t t)
         {
            SetValue(&evtTime, t);
         }

         void Init(EventNumType evnt, RunId run=0, EvtId evid=EvtId_DABC);

         /*
          * Insert full event header at position buf. Returns pointer on this new event
          * */
         static hadaq::Event* PutEventHeader(char** buf, EvtId id);

         /*
          * Generate run id from current time.
          * Same as used in hades eventbuilders for filenames
          */
         static hadaq::RunId  CreateRunId();


         /*
          * Format a HADES-convention filename string
          * from a given run id.
          */
        static std::string FormatFilename (hadaq::RunId id);

   };

   //Subevent
   //
   //Every event contains zero to unspecified many subevents. As an empty event is allowed, data outside of any subevent are not allowed. A subevents consists out of a fixed size subevent header and a varying number of data words.
   //
   //    * The subevent header
   //      subEvtHeader
   //      subEvtSize   subEvtDecoding    subEvtId    subEvtTrigNr
   //          o subEvtSize - size of the subevent: This includes the the subevent header, it is measured in bytes.
   //          o subEvtDecoding - subevent decoding type: Tells the analysis the binary format of the subevent data, so that it can be handed to a corresponding routine for unpacking into a software usable data structure. For easier decoding of this word, the meaning of some bits is already predefined:
   //            subEvtDecoding
   //            msB    ---   ---   lsB
   //            0   data type   decoding type  nonzero
   //                + The first (most significant) byte is always zero
   //                + The second byte contains the word length of the subevent data. 0 = byte, 1 = 16 bit word, 2 = 32 bit word...
   //                + The remaining two bytes form the actual decoding type, e.g. fixed length, zero suppressed etc.
   //                + The last byte must not be zero, so the whole subEvtDecoding can be used to check for correct or swapped byte order. It is stated again, that the whole subEvtDecoding is one 32bit word. The above bit assignments are merely a rule how to select this 32bit numbers.
   //          o subEvtId - subevent identifier: Tells the analysis the semantics of the subevent data, e.g. every subevent builder may get its own subEvtId. So the data structure can be analyzed by the corresponding routine after unpacking.
   //            1-99   DAQ
   //            100-199   RICH
   //            200-299   MDC
   //            300-399   SHOWER
   //            400-499   TOF
   //            500-599   TRIG
   //            600-699   SLOW
   //            700-799   TRB_RPC  common TRB, but contents is RPC
   //            800-899   TRB_HOD  pion-hodoscope
   //            900-999   TRB_FW   forward wall
   //            1000-1099    TRB_START   start detector
   //            1100-1199    TRB_TOF  TOF detector
   //            1200-1299    TRB RICH    RICH detector
   //
   //Additionally, all subEvtIds may have the MSB set. This indicates a sub event of the corresponding id that contains broken data (e.g. parity check failed, sub event was too long etc.).
   //
   //    *
   //          o subEvtTrigNr - subevent Trigger Number: This is the event tag that is produced by the trigger system and is used for checking the correct assembly of several sub entities. In general, this number is not counting sequentially. The lowest significant byte represents the trigger tag generated by the CTU and has to be same for all detector subsystems in one event. The rest is filled by the EB.
   //    * The data words: The format of the data words (word length, compression algorithm, sub-sub-event format) is defined by the subEvtDecoding and apart from this it is completely free. The meaning of the data words (detector, geographical position, error information) is defined by the subEvtId and apart from this it is completely unknown to the data acquisition system.

   struct Subevent: public HadTuId {
         int32_t subEvtTrigNr;

         int32_t GetTrigNr()
         {
            return Value(&subEvtTrigNr);
         }

         unsigned Alignment()
         {
            return 1 << (GetDecoding() >> 16 & 0xff);
         }


         /* swapsave access to any data. stolen from hadtu.h*/
         uint32_t Data(unsigned idx)
         {
            const void* my = (char*) (this) + sizeof(hadaq::Subevent);
            //const void* my= (char*) (this) + 16;
            uint32_t val;

            switch (Alignment()) {
               case 4:
                  if (IsSwapped()) {
                     uint32_t tmp0;
                     uint32_t tmp1;
                     tmp0 = ((uint32_t *) my)[idx];
                     ((char *) &tmp1)[0] = ((char *) &tmp0)[3];
                     ((char *) &tmp1)[1] = ((char *) &tmp0)[2];
                     ((char *) &tmp1)[2] = ((char *) &tmp0)[1];
                     ((char *) &tmp1)[3] = ((char *) &tmp0)[0];
                     val = tmp1;
                  } else {
                     val = ((uint32_t *) my)[idx];
                  }
                  break;
               case 2:
                  if (IsSwapped()) {
                     uint16_t v;
                     //swab(((uint16_t *) (my)[idx]), &v, 2);
                     swab(((uint16_t *) (my)) + idx, &v, 2);
                     val = v;
                  } else {
                     val = ((uint16_t *) my)[idx];
                  }
                  break;
               default:
                  val = ((uint8_t *) my)[idx];
                  break;
            }
            return val;

         }

         void Init(int32_t trigger = 0)
         {
            subEvtTrigNr = trigger;
         }

         void* RawData()
         {
            void* my = (char*) (this) + sizeof(hadaq::Subevent);
            return my;
         }
   };

#pragma pack(pop)



   extern const char* typeHldInput;
   extern const char* typeHldOutput;
   extern const char* typeUdpDevice;
   extern const char* typeUdpInput;

   extern const char* protocolHld;
   extern const char* protocolHadaq;

   extern const char* xmlFileName;
   extern const char* xmlSizeLimit;
   extern const char* xmlUdpAddress;
   extern const char* xmlUdpPort;
   extern const char* xmlUdpBuffer;
   extern const char* xmlMTUsize;
   extern const char* xmlBuildFullEvent;

   extern const char* xmlNormalOutput;
   extern const char* xmlFileOutput;
   extern const char* xmlServerOutput;
   extern const char* xmlObserverEnabled;
   extern const char* portOutput;
   extern const char* portOutputFmt;
   extern const char* portInput;
   extern const char* portInputFmt;
   extern const char* portFileOutput;
   extern const char* portServerOutput;

   extern const char* NetmemPrefix;
   extern const char* EvtbuildPrefix;


}
;

#endif
