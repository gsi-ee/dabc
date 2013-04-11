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

#ifndef ROC_CombinerModule
#define ROC_CombinerModule

#include "dabc/ModuleAsync.h"
#include "dabc/Pointer.h"

#include "roc/BoardsVector.h"

#include "roc/CombinerModule.h"

#include "mbs/Iterator.h"

#include <vector>


namespace roc {

   extern const char* xmlSkipErrorData;      // if true, error data will be skipped
   extern const char* xmlIgnoreMissingEpoch; // workaround for fail FEET readout
   extern const char* xmlSyncNumber;       // specified sync channel, used for synchronization
   extern const char* xmlSyncScaleDown;    // identifies scale down of sync events
   extern const char* xmlSpillRoc;
   extern const char* xmlSpillAux;
   extern const char* xmlCalibrationPeriod;
   extern const char* xmlCalibrationLength;
   extern const char* xmlThrottleAux;
   extern const char* xmlGet4ResetPeriod;  // reset period of all GET4 chips
   extern const char* xmlGet4ResetLimit;  // how often automatic reset is allowed when GET4 error is detected

   class CombinerModule : public dabc::ModuleAsync {

      public:

         CombinerModule(const std::string& name, dabc::Command cmd);
         virtual ~CombinerModule();

         virtual void BeforeModuleStart();

         virtual void ProcessInputEvent(unsigned port);

         virtual void ProcessOutputEvent(unsigned port);

         virtual void ProcessTimerEvent(unsigned timer);

      protected:

         enum { MaxGet4 = 16, MaxGet4Ch = 4 };

         struct InputRec {
            unsigned      rocid;
            int           format;
            bool          isudp;  // indicate if optic or Ethernet transport are attached

            uint32_t   curr_epoch;
            bool       iscurrepoch;
            unsigned   curr_nbuf;
            unsigned   curr_indx;

            uint32_t   prev_epoch;  // previous epoch marker
            bool       isprev;
            unsigned   prev_nbuf;  // id of buffer where epoch is started
            unsigned   prev_indx;  // index inside buffer where epoch found
            uint32_t   prev_evnt;  // sync event number
            uint32_t   prev_stamp; // sync time stamp

            uint32_t   next_epoch;  // number of the next epoch
            bool       isnext;      //
            unsigned   next_nbuf;   // id of buffer where epoch is started
            unsigned   next_indx;  // index inside buffer where epoch found
            uint32_t   next_evnt;  // sync event number
            uint32_t   next_stamp; // sync time stamp

            bool       nummbssync;    // indicate how many mbs sync data we had
            uint32_t   firstmbssync;  // value of first found mbssync

            unsigned   data_length; // length of data between prev and next event

            bool       isready;    // indicate if epoch and next are defined and ready for combining

            bool          use;     // use input for combining
            bool          data_err; // event numbers are wrong
            dabc::Pointer ptr;     // used in combining
            unsigned      nbuf;    // used in combining
            uint32_t      epoch;   // original epoch used in combining
            unsigned      stamp;   // corrected stamp value in combining
            unsigned      stamp_shift; // (shift to event begin)

            uint64_t      last_thottle_tm; // last time, when threshold state was changed
            bool          last_throttle_state; // last state of throttle signal (on/off)

            uint32_t      lastEpoch2[MaxGet4]; // last epoch2 for each get4
            int           get4EdgeCnt[MaxGet4][MaxGet4Ch];  // edge counter for each get4 channel
            int           get4EdgeErrs[MaxGet4][MaxGet4Ch]; // number of errors when consequent same adges more than 3
            uint64_t      get4LastTm[MaxGet4][MaxGet4Ch]; // last time stamp on each channel (disregard edge)
            int           get4AllCnt[MaxGet4][MaxGet4Ch]; //  counter of all sign changes
            int           get4ErrCnt[MaxGet4][MaxGet4Ch]; //  counter of suspicious sign changes
            unsigned      lastEpoch2SyncErr[MaxGet4]; // last error difference between epoch2 when sync=1
            bool          canCheckGet4[MaxGet4]; // indicate if we could analyze data of GET4, on after first sync
            bool          canCheckAnyGet4; // true if any get4 can be checked, disabled after reconfig until sys message is available

            bool isrocid() const { return rocid!=0xffffffff; }

            bool IsDifferentRocId(unsigned id, bool& iserr)
            {
               iserr = false;
               if (rocid==id) return false;

               // in case of udp channel any different rocid is error
               if (isudp) { iserr = true; return true; }

               iserr = true;
               return true;
            }

            unsigned can_skip_buf() { return isprev ? prev_nbuf : 0; }

            void did_skip_buf(unsigned cnt)
            {
               curr_nbuf -= cnt;
               if (isprev) prev_nbuf -= cnt;
               if (isnext) next_nbuf -= cnt;
            }

            InputRec() :
               rocid(0xffffffff), format(formatEth1), isudp(true),
               curr_epoch(0), iscurrepoch(false), curr_nbuf(0), curr_indx(0),
               prev_epoch(0), isprev(false), prev_nbuf(0), prev_indx(0), prev_evnt(0), prev_stamp(0),
               next_epoch(0), isnext(false), next_nbuf(0), next_indx(0), next_evnt(0), next_stamp(0),
               nummbssync(0), firstmbssync(0),
               data_length(0), isready(false),
               use(false), data_err(false), ptr(), nbuf(0), epoch(0), stamp(0), stamp_shift(0),
               last_thottle_tm(0), last_throttle_state(false)
            {
               for (int n=0;n<MaxGet4;n++) {
                  lastEpoch2[n] = 0;
                  lastEpoch2SyncErr[n] = 0;
                  canCheckGet4[n] = false;
                  for(int ch=0;ch<MaxGet4Ch;ch++) {
                     get4EdgeCnt[n][ch] = 0;
                     get4EdgeErrs[n][ch] = 0;
                     get4LastTm[n][ch] = 0;
                     get4AllCnt[n][ch] = 0;
                     get4ErrCnt[n][ch] = 0;
                  }
               }

               canCheckAnyGet4 = true;
            }

             InputRec(const InputRec& r) :
               rocid(r.rocid), format(r.format), isudp(r.isudp),
               curr_epoch(r.curr_epoch), iscurrepoch(r.iscurrepoch), curr_nbuf(r.curr_nbuf), curr_indx(r.curr_indx),
               prev_epoch(r.prev_epoch), isprev(r.isprev), prev_nbuf(r.prev_nbuf), prev_indx(r.prev_indx), prev_evnt(r.prev_evnt), prev_stamp(r.prev_stamp),
               next_epoch(r.next_epoch), isnext(r.isnext), next_nbuf(r.next_nbuf), next_indx(r.next_indx), next_evnt(r.next_evnt), next_stamp(r.next_stamp),
               nummbssync(r.nummbssync), firstmbssync(r.firstmbssync),
               data_length(r.data_length), isready(r.isready),
               use(r.use), data_err(r.data_err), ptr(r.ptr), nbuf(r.nbuf), epoch(r.epoch), stamp(r.stamp), stamp_shift(r.stamp_shift),
               last_thottle_tm(r.last_thottle_tm), last_throttle_state(r.last_throttle_state)
             {
                for (int n=0;n<MaxGet4;n++) {
                   lastEpoch2[n] = r.lastEpoch2[n];
                   canCheckGet4[n] = r.canCheckGet4[n];
                   lastEpoch2SyncErr[n] = r.lastEpoch2SyncErr[n];
                   for(int ch=0;ch<MaxGet4Ch;ch++) {
                      get4EdgeCnt[n][ch] = r.get4EdgeCnt[n][ch];
                      get4EdgeErrs[n][ch] = r.get4EdgeErrs[n][ch];
                      get4LastTm[n][ch] = r.get4LastTm[n][ch];
                      get4AllCnt[n][ch] = r.get4AllCnt[n][ch];
                      get4ErrCnt[n][ch] = r.get4ErrCnt[n][ch];
                   }
                }
                canCheckAnyGet4 = r.canCheckAnyGet4;
             }
         };

         virtual int ExecuteCommand(dabc::Command cmd);

         virtual void ModuleCleanup();

         bool FindNextEvent(unsigned recid);
         bool SkipEvent(unsigned recid);

         bool FillBuffer();
         bool FlushOutputBuffer();

         void InvokeAllGet4Reset();
         unsigned FillRawSubeventsBuffer(dabc::Pointer& outptr, roc::MessagesVector* extra = 0);
         void AddExtraMessagesToSubevent(roc::MessagesVector* extra, dabc::Pointer& outptr, unsigned& subeventsize, InputRec* rec);

         void DumpData(dabc::Buffer& buf);

         void SetInfo(const std::string& info, bool forceinfo = false);

         int                  fSyncScaleDown;  // expected distance between two events ids, should be specified in config file or in command
         int                  fSyncNumber;     // specifies sync channel, used for synchronization
         bool                 fIgnoreMissingEpoch; // ignore missing epoch for sync marker, add artificial one
         bool                 fSkipErrorData;  // if true, skip error data - necessary for complex systems with MBS/susibo

         std::vector<InputRec> fInp;

         dabc::Buffer         fOutBuf;
         dabc::Pointer        f_outptr;

         int                  fSpillRoc;      // roc number, where spill on/off signal is connected
         int                  fSpillAux;      // number of Aux channel where spill on/off signal is measured
         bool                 fSpillState;    // current state of spill, known by module
         uint64_t             fLastSpillTime; // last time when spill signal was seen

         double               fCalibrationPeriod; // how often trigger mode should be started
         double               fCalibrationLength; // how long calibration should be switched on
         dabc::TimeStamp      fLastCalibrationTime; // last time when trigger was caused

         double               fGet4ResetPeriod;  // period how often reset of Get4 should be performed

         int                  fThrottleAux;  // defines AUX signal, which analyzed at throttled

         std::list<MessagesVector*>  fExtraMessages;

         bool                 fFlushFlag;     // boolean, indicate if buffer should be flushed in next timeout

         double               fGet4ResetLimit; // how often get4 can be reseted
         dabc::TimeStamp      fLastGet4ResetTm; // time when reset was send to application
         bool                 fDetectGet4Problem;

   };
}

#endif
