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

#ifndef HADAQ_CombinerModule
#define HADAQ_CombinerModule


#include <sched.h>

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

#ifndef DABC_Profiler
#include "dabc/Profiler.h"
#endif

#include "hadaq/HadaqTypeDefs.h"

#include "hadaq/Iterator.h"

#define HADAQ_NEVTIDS 64UL             /* must be 2^n */
#define HADAQ_NEVTIDS_IN_FILE 0UL      /* must be 2^n */
#define HADAQ_NUMERRPATTS 5
#define HADAQ_RINGSIZE 100

namespace hadaq {

   /** \brief %Combiner of HADAQ subevents into full events
    *
    * This module will combine the hadtu data streams into full hadaq events
    * Use functionality as in daq_evtbuild here.
    */

   class TerminalModule;

   class CombinerModule: public dabc::ModuleAsync {

      friend class TerminalModule;

      protected:

      struct InputCfg {
         hadaq::RawSubevent *subevnt; ///< actual subevent
         hadaq::RawEvent  *evnt; ///< actual event
         bool     has_data;      ///< when true, input has data (subevent or bunch of sub events)
         uint32_t data_size;     ///< padded size of current subevent, required in output buffer
         uint32_t fTrigNr;       ///< keeps current trigger sequence number
         uint32_t fLastTrigNr;   ///< keeps previous trigger sequence number - used to control data lost
         uint32_t fTrigTag;      ///< keeps current trigger tag
         uint32_t fTrigType;     ///< current subevent trigger type
         uint32_t fErrorBits;    ///< errorbit status word from payload end
         uint32_t fErrorbitStats[HADAQ_NUMERRPATTS]; ///< errorbit statistics counter
         unsigned fHubId{0};     ///< subevent id from given input
         unsigned fUdpPort{0};    ///< if configured, port id
         float fQueueLevel;      ///<  current input queue fill level
         uint32_t fLastEvtBuildTrigId; ///< remember id of last build event
         bool fDataError;        ///< indicates if subevent has data error bit set in header id
         bool fEmpty;            ///< indicates if input has empty data
         void* fInfo;            ///< Direct pointer on transport info, used only for debugging
         int fNumCanRecv;        ///< Number buffers can be received
         unsigned fLostTrig;     ///< number of lost triggers (never received by the combiner)
         unsigned fDroppedTrig;  ///< number of dropped triggers (received but dropped by the combiner)
         uint32_t  fTrigNumRing[HADAQ_RINGSIZE]; ///< values of last seen TU ID
         unsigned  fRingCnt;     ///< where next value will be written
         unsigned  fResort;       ///< enables resorting of events
         ReadIterator fIter;      ///< main iterator
         ReadIterator fResortIter; ///< additional iterator to check resort
         int          fResortIndx;  ///< index of buffer in the queue used for resort iterator (-1 - off)
         std::string fCalibr;      ///< name of calibration module, used only in terminal
         bool        fCalibrReq;   ///< when true, request was send
         int         fCalibrProgr; ///< calibration progress
         std::string fCalibrState; ///< calibration state
         double      fCalibrQuality; ///< calibration quality
         uint64_t    fHubLastSize; ///< last size
         uint64_t    fHubPrevSize; ///< last size
         int         fHubSizeTmCnt; ///< count how many time data was the same


         InputCfg() :
            subevnt(0),
            evnt(0),
            has_data(false),
            data_size(0),
            fTrigNr(0),
            fLastTrigNr(0xffffffff),
            fTrigTag(0),
            fTrigType(0),
            fErrorBits(0),
            fQueueLevel(0),
            fLastEvtBuildTrigId(0),
            fDataError(false),
            fEmpty(true),
            fInfo(0),
            fNumCanRecv(0),
            fLostTrig(0),
            fDroppedTrig(0),
            fRingCnt(0),
            fResort(false),
            fIter(),
            fResortIter(),
            fResortIndx(-1),
            fCalibr(),
            fCalibrReq(false),
            fCalibrProgr(0),
            fCalibrState(),
            fCalibrQuality(0.),
            fHubLastSize(0),
            fHubPrevSize(0),
            fHubSizeTmCnt(0)
         {
            for(int i=0;i<HADAQ_NUMERRPATTS;i++)
               fErrorbitStats[i]=0;
            for (int i=0;i<HADAQ_RINGSIZE;i++)
               fTrigNumRing[i]=0;
         }

         void Reset(bool complete = false)
         {
            // used to reset current subevent
            subevnt = 0;
            evnt = 0;
            has_data = false;
            data_size = 0;
            fTrigNr = 0;
            fTrigTag = 0;
            fTrigType = 0;
            fErrorBits = 0;
            fDataError = false;
            // fHubId = 0;
            fEmpty = true;
           // do not clear error bit statistics!
//             for(int i=0;i<HADAQ_NUMERRPATTS;++i)
//                fErrorbitStats[i]=0;
            // do not clear last fill level and last trig id
            if (complete) fLastTrigNr = 0xffffffff;
            if (complete) fUdpPort = 0;
         }

         void Close()
         {
            // used to close all iterators
            fIter.Close();
            fResortIter.Close();
            fResortIndx = -1;
         }

         std::string TriggerRingAsStr(int RingSize) {
            std::string sbuf;

            unsigned cnt = fRingCnt, prev = 0;
            if ((int)cnt < RingSize) cnt += HADAQ_RINGSIZE;
            bool first = true;
            cnt -= RingSize;
            for (int n=0;n<RingSize;n++) {
               if (first && (fTrigNumRing[cnt]==0) && (n!=RingSize-1)) {
                  // ignore 0 in the beginning
               } else if (first) {
                  sbuf.append(dabc::format(" 0x%06x",(unsigned)fTrigNumRing[cnt]));
                  first = false;
               } else {
                  sbuf.append(dabc::format(" %d",(int)fTrigNumRing[cnt] - (int)fTrigNumRing[prev]));
               }

               prev = cnt; cnt = (cnt+1) % HADAQ_RINGSIZE;
            }
            return sbuf;
         }
      };


         /* master stream for event building*/
         //unsigned fMasterChannel;

         /** maximum allowed difference of trigger numbers (subevent sequence number)*/
         int fTriggerNrTolerance;

         /** timeout in seconds since last complete event when previous buffers are dropped*/
         double fEventBuildTimeout;

         /** When true, read trigger type as in original hades event builders */
         bool fHadesTriggerType;
         unsigned fHadesTriggerHUB;

         uint32_t fLastTrigNr;  ///<  last number of build event

         std::vector<InputCfg> fCfg; ///< all input-dependent configurations

         WriteIterator      fOut;

         int                fFlushCounter;
         int32_t            fEBId; ///<  eventbuilder id <- node id
         pid_t              fPID; ///<  process id of combiner module
         bool               fWithObserver;
         bool               fEpicsSlave;
         bool               fIsTerminating;
         bool               fSkipEmpty;     ///< skip empty subevents in final event, default true

         bool               fRunToOracle;

         int                fNumReadBuffers; //< / SL: workaround counter, which indicates how many buffers were taken from queues

         unsigned           fSpecialItemId;  ///< item used to create user events
         bool               fSpecialFired;  ///< if user event was already fired
         double             fLastEventRate; ///< last event rate

         bool               fCheckTag;

         bool               fBNETsend;  ///< indicate that combiner used as BNET sender
         bool               fBNETrecv;  ///< indicate that second-level event building is performed
         int                fBNETbunch; ///< number of events delivered to same event builder
         int                fBNETNumSend; ///< number of BNET senders
         int                fBNETNumRecv; ///< number of BNET receivers

         double             fFlushTimeout;
         dabc::Command      fBnetFileCmd;  ///< current running bnet file command

         std::string        fDataRateName;
         std::string        fDataDroppedRateName;
         std::string        fEventRateName;
         std::string        fLostEventRateName;
         std::string        fInfoName;

         uint64_t           fTotalRecvBytes;
         uint64_t           fTotalBuildEvents;   ///< number of build events
         uint64_t           fTotalDroppedData;
         uint64_t           fTotalDiscEvents;
         uint64_t           fTotalTagErrors;
         uint64_t           fTotalDataErrors;
         uint64_t           fTotalFullDrops;   ///< number of complete drops

         unsigned fEventIdCount[HADAQ_NEVTIDS];

         uint32_t fErrorbitPattern[HADAQ_NUMERRPATTS];

         std::string fRunInfoToOraFilename;
         std::string fPrefix;

         /** run id from timeofday for eventbuilding*/
         uint32_t           fRunNumber;

         /** most recent run id from epics, for multi eventbuilder mode*/
         uint32_t           fEpicsRunNumber;

         /** Defines trigger sequence number range for overflow*/
         uint32_t           fMaxHadaqTrigger;
         uint32_t           fTriggerRangeMask;

         /** if true, account difference of subsequent build event numbers as lost events
            if false, do not account it (for multiple event builder mode)*/
         bool               fEvnumDiffStatistics;

         bool              fExtraDebug;   ///< when true, extra debug output is created
         dabc::TimeStamp   fLastDebugTm;  ///< timer used to generate rare debugs output
         dabc::TimeStamp   fLastDropTm;   ///< timer used to avoid too often drop of data
         dabc::TimeStamp   fLastProcTm;   ///< last time when event building was called
         dabc::TimeStamp   fLastBuildTm;  ///< last time when complete event was build
         double            fMaxProcDist;  ///< maximal time between calls to BuildEvent method

         std::string       fBnetInfo;     ///< info for showing of bnet sender
         std::string       fBnetStat;     ///< gener-purpose statistic in text form

         long              fBldCalls{0};   ///< number of build event calls
         long              fInpCalls{0};   ///< number of input processing calls
         long              fOutCalls{0};   ///< number of output processing calls
         long              fBufCalls{0};   ///< number of buffer processing calls
         long              fTimerCalls{0}; ///< number of timer events calls
         dabc::Profiler    fBldProfiler;   ///< profiler of build event performance

         bool BuildEvent();

         bool FlushOutputBuffer();

         void RegisterExportedCounters();
         bool UpdateExportedCounters();

         void DoErrorBitStatistics(unsigned ninp);

         void DoInputSnapshot(unsigned ninp);

         virtual void BeforeModuleStart();

         virtual void AfterModuleStop();

         bool ShiftToNextHadTu(unsigned ninp);

         /** Shifts to next event in the input queue */
         bool ShiftToNextEvent(unsigned ninp, bool fast = false, bool dropped = false);

         /** Shifts to next subevent in the input queue */
         bool ShiftToNextSubEvent(unsigned ninp, bool fast = false, bool dropped = false);

         /** Method should be used to skip current buffer from the queue */
         bool ShiftToNextBuffer(unsigned ninp);

         /* cleanup input buffers in case of too large eventnumber mismatch*/
         bool DropAllInputBuffers();

         //uint32_t CurrEventId(unsigned int ninp) const { return fCfg[ninp].curr_evnt_num; }

         void SetInfo(const std::string &info, bool forceinfo = false);


         /* Methods to export run begin to oracle via text file*/
         void StoreRunInfoStart();

         /* Methods to export run end and statistics to oracle via text file
              if this is called on eventbuilder exit, we use local time instead ioc/runnumber time*/
         void StoreRunInfoStop(bool onexit=false, unsigned newrunid = 0);

         void ResetInfoCounters();

         /* stolen from daqdata/hadaq/logger.c to keep oracle export output format of numbers*/
         char* Unit(unsigned long v);

         /* we synthetise old and new filenames ourselves, since all communication to hldoutput is faulty,
             because of timeshift between getting new runid and open/close of actual files*/
         std::string GenerateFileName(unsigned runid);

         void DoTerminalOutput();

         /* helper methods to export ebctrl parameters */
         std::string GetEvtbuildParName(const std::string &name);
         void CreateEvtbuildPar(const std::string &name);
         void SetEvtbuildPar(const std::string &name, unsigned value);
         unsigned GetEvtbuildParValue(const std::string &name);

         std::string GetNetmemParName(const std::string &name);
         void CreateNetmemPar(const std::string &name);
         void SetNetmemPar(const std::string &name, unsigned value);

         int DestinationPort(uint32_t trignr);
         bool CheckDestination(uint32_t trignr);
         void UpdateBnetInfo();

         void StartEventsBuilding();

      public:
         CombinerModule(const std::string &name, dabc::Command cmd = nullptr);
         virtual ~CombinerModule();

         virtual void ModuleCleanup();

         virtual bool ProcessBuffer(unsigned port);
         virtual bool ProcessRecv(unsigned port) { fInpCalls++; StartEventsBuilding(); return false; }
         virtual bool ProcessSend(unsigned port) { fOutCalls++; StartEventsBuilding(); return false; }

         virtual void ProcessTimerEvent(unsigned timer);

         virtual void ProcessUserEvent(unsigned item);

         virtual int ExecuteCommand(dabc::Command cmd);

         virtual bool ReplyCommand(dabc::Command cmd);

         int CalcTrigNumDiff(const uint32_t& prev, const uint32_t& next);

   };

}

#endif
