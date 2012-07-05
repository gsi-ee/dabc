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


#include "hadaq/HadaqTypeDefs.h"
#include "hadaq/Iterator.h"

/*
 * This module will combine the hadtu data streams into full hadaq events
 * Use functionality as in daq_evtbuild here.
 */

namespace hadaq {

   class CombinerModule: public dabc::ModuleAsync {

         struct InputCfg {


               /** keeps current trigger sequence number */
               hadaq::EventNumType fTrigNr;

               /** keeps current trigger tag */
               hadaq::EventNumType fTrigTag;

               /* current subevent trigger type*/
               uint32_t fTrigType;

               /* current subevent id*/
               uint32_t fSubId;

               /* errorbit status word from payload end*/
               uint32_t fErrorBits;

               /* errorbit statistics counter */
               uint32_t fErrorbitStats[HADAQ_NUMERRPATTS];

               /** indicates if subevent has data error bit set in header id */
               bool fDataError;

               /** indicates if input has empty data */
               bool fEmpty;

               InputCfg() :
                  fTrigNr(0),
                  fTrigTag(0),
                  fTrigType(0),
                  fSubId(0),
                  fErrorBits(0),
                  fDataError(false),
                  fEmpty(true)
               {
                  for(int i=0;i<HADAQ_NUMERRPATTS;++i)
                     fErrorbitStats[i]=0;
               }

               InputCfg(const InputCfg& src) :
                  fTrigNr(src.fTrigNr),
                  fTrigTag(src.fTrigTag),
                  fTrigType(src.fTrigType),
                  fSubId(src.fSubId),
                  fErrorBits(0),
                  fDataError(src.fDataError),
                  fEmpty(src.fEmpty)
               {
                  for(int i=0;i<HADAQ_NUMERRPATTS;++i)
                     fErrorbitStats[i]=src.fErrorbitStats[i];

               }

               void Reset()
               {
                  fTrigNr = 0;
                  fTrigTag =0;
                  fTrigType=0;
                  fSubId=0;
                  fErrorBits=0;
                  fDataError = false;
                  fEmpty = true;
                  for(int i=0;i<HADAQ_NUMERRPATTS;++i)
                     fErrorbitStats[i]=0;
               }
         };

      public:
         CombinerModule(const char* name, dabc::Command cmd = 0);
         virtual ~CombinerModule();

         virtual void ProcessInputEvent(dabc::Port* port);
         virtual void ProcessOutputEvent(dabc::Port* port);
         virtual void ProcessConnectEvent(dabc::Port* port);
         virtual void ProcessDisconnectEvent(dabc::Port* port);

         virtual void ProcessTimerEvent(dabc::Timer* timer);

         bool IsFileOutput() const
         {
            return fFileOutput;
         }
         bool IsServOutput() const
         {
            return fServOutput;
         }

         virtual int ExecuteCommand(dabc::Command cmd);

      protected:

         bool BuildEvent();
         bool FlushOutputBuffer();

         void RegisterExportedCounters();
         bool UpdateExportedCounters();
         void ClearExportedCounters();

         void DoErrorBitStatistics(unsigned ninp);

         /* provide output buffer that has room for payload size of bytes
          * returns false if not possible*/
         bool EnsureOutputBuffer(uint32_t payload);

         virtual void BeforeModuleStart();

         virtual void AfterModuleStop();

         bool ShiftToNextSubEvent(unsigned ninp);

         bool ShiftToNextHadTu(unsigned ninp);

         /** Method should be used to skip current buffer from the queue */
         bool ShiftToNextBuffer(unsigned ninp);

         bool SkipInputBuffer(unsigned ninp);

         /* cleanup input buffers in case of too large eventnumber mismatch*/
         bool DropAllInputBuffers();

         //hadaq::EventNumType CurrEventId(unsigned int ninp) const { return fCfg[ninp].curr_evnt_num; }

         void SetInfo(const std::string& info, bool forceinfo = false);


         /* helper methods to export ebctrl parameters */
         std::string GetEvtbuildParName(const std::string& name);
         void CreateEvtbuildPar(const std::string& name);
         void SetEvtbuildPar(const std::string& name, unsigned value);

         std::string GetNetmemParName(const std::string& name);
         void CreateNetmemPar(const std::string& name);
         void SetNetmemPar(const std::string& name, unsigned value);






         unsigned                   fBufferSize;

         /* master stream for event building*/
         //unsigned fMasterChannel;

         /* maximum allowed difference of trigger numbers (subevent sequence number)*/
         hadaq::EventNumType fTriggerNrTolerance;

         dabc::Buffer fRcvBuf;

         std::vector<InputCfg> fCfg;
         std::vector<ReadIterator> fInp;
         WriteIterator fOut;
         //dabc::Buffer fOutBuf;
         bool fFlushFlag;
         bool fUpdateCountersFlag;

         bool fDoOutput;
         bool fFileOutput;
         bool fServOutput;
         bool fWithObserver;
         int fNumInputs;

         /* switch between partial combining of smallest event ids (false)
                 * and building of complete events only (true)*/
         bool                          fBuildCompleteEvents;

         std::string                fEventRateName;
         std::string                fEventDiscardedRateName;
         std::string                fEventDroppedRateName;
         std::string                fDataRateName;
         std::string                fInfoName;

         uint64_t           fTotalRecvBytes;
         uint64_t           fTotalRecvEvents;
         uint64_t           fTotalDroppedEvents;
         uint64_t           fTotalDiscEvents;
         uint64_t           fTotalTagErrors;
         uint64_t           fTotalDataErrors;

         unsigned fEventIdCount[HADAQ_NEVTIDS];

         uint32_t fErrorbitPattern[HADAQ_NUMERRPATTS];

         /* run id from timeofday for eventbuilding*/
         RunId fRunNumber;
   };

}

#endif
