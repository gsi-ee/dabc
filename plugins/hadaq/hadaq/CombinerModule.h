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


               /** indicates if subevent has data error bit set */
               bool fDataError;

               /** indicates if input has empty data */
               bool fEmpty;

               InputCfg() :
                  fTrigNr(0),
                  fTrigTag(0),
                  fDataError(false),
                  fEmpty(true)
               {
               }

               InputCfg(const InputCfg& src) :
                  fTrigNr(src.fTrigNr),
                  fTrigTag(src.fTrigTag),
                  fDataError(src.fDataError),
                  fEmpty(src.fEmpty)
               {
               }

               void Reset()
               {
                  fTrigNr = 0;
                  fTrigTag =0;
                  fDataError = false;
                  fEmpty = true;
               }
         };

      public:
         CombinerModule(const char* name, dabc::Command cmd = 0);


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



         unsigned                   fBufferSize;

         /* master stream for event building*/
         //unsigned fMasterChannel;

         /* maximum allowed difference of trigger numbers (subevent sequence number)*/
         hadaq::EventNumType fTriggerNrTolerance;

         std::vector<InputCfg> fCfg;
         std::vector<ReadIterator> fInp;
         WriteIterator fOut;
         //dabc::Buffer fOutBuf;
         bool fFlushFlag;

         bool fDoOutput;
         bool fFileOutput;
         bool fServOutput;

         /* switch between partial combining of smallest event ids (false)
                 * and building of complete events only (true)*/
         bool                          fBuildCompleteEvents;

         std::string                fEventRateName;
         std::string                fEventDiscardedRateName;
         std::string                fDataRateName;
         std::string                fInfoName;

         uint64_t           fTotalRecvBytes;
         uint64_t           fTotalRecvEvents;
         uint64_t           fTotalDroppedEvents;
         uint64_t           fTotalDiscEvents;
         uint64_t           fTotalTagErrors;
         uint64_t           fTotalDataErrors;

         /* run id from timeofday for eventbuilding*/
         RunId fRunNumber;
   };

}

#endif
