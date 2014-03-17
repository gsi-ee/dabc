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

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

#include "hadaq/HadaqTypeDefs.h"

#include "hadaq/Iterator.h"

#define HADAQ_NEVTIDS 64UL             /* must be 2^n */
#define HADAQ_NEVTIDS_IN_FILE 0UL      /* must be 2^n */
#define HADAQ_NUMERRPATTS 5

namespace hadaq {

   /** \brief %Combiner of HADAQ subevents into full events
    *
    * This module will combine the hadtu data streams into full hadaq events
    * Use functionality as in daq_evtbuild here.
    */

   class CombinerModule: public dabc::ModuleAsync {

      protected:

      struct InputCfg {

         /** keeps current trigger sequence number */
         uint32_t fTrigNr;

         /** keeps previous trigger sequence number - used to control lost data */
         uint32_t fLastTrigNr;

         /** keeps current trigger tag */
         uint32_t fTrigTag;

         /* current subevent trigger type*/
         uint32_t fTrigType;

         /* current subevent id*/
         uint32_t fSubId;

         /* errorbit status word from payload end*/
         uint32_t fErrorBits;

         /* errorbit statistics counter */
         uint32_t fErrorbitStats[HADAQ_NUMERRPATTS];

         /* current input queue fill level*/
         float fQueueLevel;

         /* remember id of last build event*/
         uint32_t fLastEvtBuildTrigId;

         /** indicates if subevent has data error bit set in header id */
         bool fDataError;

         /** indicates if input has empty data */
         bool fEmpty;

         InputCfg() :
            fTrigNr(0),
            fLastTrigNr(0),
            fTrigTag(0),
            fTrigType(0),
            fSubId(0),
            fErrorBits(0),
            fQueueLevel(0),
            fLastEvtBuildTrigId(0),
            fDataError(false),
            fEmpty(true)
         {
            for(int i=0;i<HADAQ_NUMERRPATTS;++i)
               fErrorbitStats[i]=0;
         }

         InputCfg(const InputCfg& src) :
            fTrigNr(src.fTrigNr),
            fLastTrigNr(src.fLastTrigNr),
            fTrigTag(src.fTrigTag),
            fTrigType(src.fTrigType),
            fSubId(src.fSubId),
            fErrorBits(src.fErrorBits),
            fQueueLevel(src.fQueueLevel),
            fLastEvtBuildTrigId(src.fLastEvtBuildTrigId),
            fDataError(src.fDataError),
            fEmpty(src.fEmpty)
         {
            for(int i=0;i<HADAQ_NUMERRPATTS;++i)
               fErrorbitStats[i]=src.fErrorbitStats[i];

         }

         void Reset()
         {
            fTrigNr = 0;
            fLastTrigNr = 0;
            fTrigTag =0;
            fTrigType=0;
            fSubId=0;
            fErrorBits=0;
            fDataError = false;
            fEmpty = true;
            for(int i=0;i<HADAQ_NUMERRPATTS;++i)
               fErrorbitStats[i]=0;
            // do not clear last fill level and last trig id
         }
      };


         /* master stream for event building*/
         //unsigned fMasterChannel;

         /* maximum allowed difference of trigger numbers (subevent sequence number)*/
         int fTriggerNrTolerance;

         uint32_t fLastTrigNr;  // last number of build event

         std::vector<InputCfg> fCfg;
         std::vector<ReadIterator> fInp;
         WriteIterator fOut;

         bool fFlushFlag;
         bool fUpdateCountersFlag;

         bool fWithObserver;
	 bool fEpicsSlave;

         bool fUseSyncSeqNumber; // if true, use vulom/roc syncnumber for event sequence number
         bool fPrintSync; // if true, print syncs with DOUT1
         uint32_t fSyncSubeventId; // id number of sync subevent
         uint32_t fSyncTriggerMask; // bit mask for accepted trigger inputs in sync mode

         int  fNumCompletedBuffers; // SL: workaround counter, which indicates how many buffers were taken from queues

         /* switch between partial combining of smallest event ids (false)
          * and building of complete events only (true)*/
         bool               fBuildCompleteEvents;
         
         double             fFlushTimeout;

         std::string        fDataRateName;
         std::string        fDataDroppedRateName;
         std::string        fEventRateName;
         std::string        fLostEventRateName;
         std::string        fInfoName;

         uint64_t           fTotalRecvBytes;
         uint64_t           fTotalRecvEvents;
         uint64_t           fTotalDroppedData;
         uint64_t           fTotalDiscEvents;
         uint64_t           fTotalTagErrors;
         uint64_t           fTotalDataErrors;

         unsigned fEventIdCount[HADAQ_NEVTIDS];

         uint32_t fErrorbitPattern[HADAQ_NUMERRPATTS];

         /* run id from timeofday for eventbuilding*/
         uint32_t           fRunNumber;
	 
	  /* most recent run id from epics, for multi eventbuilder mode*/
         uint32_t           fEpicsRunNumber;

	/* Defines trigger sequence number range for overflow*/
	 uint32_t fMaxHadaqTrigger;
	 uint32_t fTriggerRangeMask;
	 
	 /* if true, account difference of subsequent build event numbers as lost events
	  if false, do not account it (for multiple event builder mode)*/
	 bool fEvnumDiffStatistics;
	 
         bool BuildEvent();

         bool FlushOutputBuffer();

         void RegisterExportedCounters();
         bool UpdateExportedCounters();
         void ClearExportedCounters();

         void DoErrorBitStatistics(unsigned ninp);

         void DoInputSnapshot(unsigned ninp);

         virtual void BeforeModuleStart();

         virtual void AfterModuleStop();

         bool ShiftToNextSubEvent(unsigned ninp);

         bool ShiftToNextHadTu(unsigned ninp);

         /** Method should be used to skip current buffer from the queue */
         bool ShiftToNextBuffer(unsigned ninp);

         bool SkipInputBuffer(unsigned ninp);

         /* cleanup input buffers in case of too large eventnumber mismatch*/
         bool DropAllInputBuffers();

         //uint32_t CurrEventId(unsigned int ninp) const { return fCfg[ninp].curr_evnt_num; }

         void SetInfo(const std::string& info, bool forceinfo = false);


         /* helper methods to export ebctrl parameters */
         std::string GetEvtbuildParName(const std::string& name);
         void CreateEvtbuildPar(const std::string& name);
         void SetEvtbuildPar(const std::string& name, unsigned value);
         unsigned GetEvtbuildParValue(const std::string& name);

         std::string GetNetmemParName(const std::string& name);
         void CreateNetmemPar(const std::string& name);
         void SetNetmemPar(const std::string& name, unsigned value);


      public:
         CombinerModule(const std::string& name, dabc::Command cmd = 0);
         virtual ~CombinerModule();
         
         virtual void ModuleCleanup();

         virtual bool ProcessBuffer(unsigned port);
         virtual bool ProcessRecv(unsigned port) { return BuildEvent(); }
         virtual bool ProcessSend(unsigned port) { return BuildEvent(); }

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int ExecuteCommand(dabc::Command cmd);
	 
	 
	int CalcTrigNumDiff(const uint32_t& prev, const uint32_t& next); 

   };

}

#endif
