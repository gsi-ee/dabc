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

#ifndef MBS_CombinerModule
#define MBS_CombinerModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef MBS_Iterator
#include "mbs/Iterator.h"
#endif

namespace mbs {

   /** \brief Combiner of several MBS inputs - event building */

   class CombinerModule : public dabc::ModuleAsync {
      protected:

         /** \brief Individual configuration and run parameters of each input */

         struct InputCfg {
            /** indicates if input is real mbs - means event produced by MBS
             * Such input is used to set event number in mbs header of output event
             * Such inputs also analyzed when special trigger numbers are appeared (14-15) */
            bool real_mbs;

            /** keeps current event number */
            mbs::EventNumType curr_evnt_num;

            /** JAM24: optional WR timestamp*/
            mbs::WRTimeStampType curr_wr_ts;

            /** indicate that we like to use white rabbit timestamps for merging */
            bool wr_timestamped;

            /** if it is start/stop daq event, treat it in special way */
            bool curr_evnt_special;

            /** Indicates if event number is in event header or moved in data body
             * Than event number will be searched in subevents */
            bool real_evnt_num;

            /** Indicates if event number in header is missing (unvalid value)
             * In this case current data from the input will be add to next mbs event */
            bool no_evnt_num;

            /** Indicates that data from input optional and can be missing BUT
             * one must ensure that next data is arrived indicating that such eventid really not exists */
            bool optional_input;

            /** Full id of subevent, where actual event id should be searched */
            uint32_t evntsrc_fullid;

            /** Shift in subevent raw data to access event id */
            uint32_t evntsrc_shift;

            /** indicates if input was selected for event building */
            bool selected;

            /** indicates if input has valid data */
            bool valid;

            /** time when last valid event was produced, can be used to remove optional input completely */
            double last_valid_tm;

            InputCfg() :
               real_mbs(true),
               curr_evnt_num(0),
               curr_wr_ts(0),
               wr_timestamped(false),
               curr_evnt_special(false),
               real_evnt_num(true),
               no_evnt_num(false),
               optional_input(false),
               evntsrc_fullid(0),
               evntsrc_shift(0),
               selected(false),
               valid(false),
               last_valid_tm(0){}

            InputCfg(const InputCfg& src) :
               real_mbs(src.real_mbs),
               curr_evnt_num(src.curr_evnt_num),
               curr_wr_ts(src.curr_wr_ts),
               wr_timestamped(src.wr_timestamped),
               curr_evnt_special(src.curr_evnt_special),
               real_evnt_num(src.real_evnt_num),
               no_evnt_num(src.no_evnt_num),
               optional_input(src.optional_input),
               evntsrc_fullid(src.evntsrc_fullid),
               evntsrc_shift(src.evntsrc_shift),
               selected(src.selected),
               valid(src.valid),
               last_valid_tm(src.last_valid_tm) {}

            void Reset()
            {
               real_mbs = true;
               curr_evnt_num = 0;
               curr_wr_ts=0;
               wr_timestamped= false;
               curr_evnt_special = false;
               real_evnt_num = true;
               no_evnt_num = false;
               optional_input = false;
               evntsrc_fullid = 0;
               evntsrc_shift = 0;
               selected = false;
               valid = false;
               last_valid_tm = 0.;
            }
         };

         std::vector<ReadIterator>  fInp;
         std::vector<InputCfg>      fCfg;
         WriteIterator              fOut;
         bool                       fFlushFlag;

         /* switch between partial combining of smallest event ids (false)
          * and building of complete events only (true)*/
         bool                       fBuildCompleteEvents;

         /** JAM24: if true, ignore event id but sort input data according to white rabbit timestamp information*/
         bool                       fBuildTimestampMergedEvents;

         /*switch on checking duplicate subevent ids in merged events -> indicate setup error*/
         bool                       fCheckSubIds;

         /** used to exclude higher bits from event id
           * Can be used when some subsystems does not provide all 32 bits
           * For instance, ROC SYNC messages has only 24-bit mask */
         mbs::EventNumType          fEventIdMask;

         /* defines maximum difference allowed between event id in merged data streams.
          * if id difference is larger, combiner may stop with error message*/
         mbs::EventNumType          fEventIdTolerance;

         /** Down limit for trigger number, which is recognized as special event
          * This event is forwarded through combiner without changes.
          * In normal MBS following special triggers present:
          * 12/13 - spill ON/OFF 14/15 DAQ start/stop
          * Therefore, limit should be 12 for such system */
         int                        fSpecialTriggerLimit;

         /** Indicates how many inputs should provide data that event is accepted as full
          *  Can be less than number of inputs while inputs may be optional */
         unsigned                   fNumObligatoryInputs;

         /** Time to exclude optional input when no data comming */
         double                     fExcludeTime;

         /** JAM24: for timestamp merger, we use new event sequence number:*/
         mbs::EventNumType fBuildevid;

         /** JAM24: accepted time interval (ns) after minimum TS for timestamp merger into single output event ("time slice buffer")*/
         mbs::WRTimeStampType fWRTimeWindow;

         std::string                fEventRateName;
         std::string                fDataRateName;
         std::string                fInfoName;
         std::string                fFileStateName;

         bool BuildEvent();
         bool FlushBuffer();

         /** JAM24: merge white rabbit timestamped input transports here*/
         bool BuildTimesortedEvent();

         void BeforeModuleStart() override;

         void AfterModuleStop() override;

         bool ShiftToNextEvent(unsigned ninp);

         /** Method should be used to skip current buffer from the queue */
         bool ShiftToNextBuffer(unsigned ninp);

         mbs::EventNumType CurrEventId(unsigned int ninp) const { return fCfg[ninp].curr_evnt_num; }

         void SetInfo(const std::string &info, bool forceinfo = false);

         bool IsOptionalInput(unsigned ninp) { return ninp<fCfg.size() ? (fCfg[ninp].optional_input || fCfg[ninp].no_evnt_num) : true; }


         /** change file on/off state in application*/
         void ChangeFileState(bool on);

      public:

         CombinerModule(const std::string &name, dabc::Command cmd = nullptr);
         virtual ~CombinerModule();

         void ModuleCleanup() override;

         bool ProcessRecv(unsigned) override { return BuildEvent(); }
         bool ProcessSend(unsigned) override { return BuildEvent(); }

         void ProcessTimerEvent(unsigned timer) override;

         unsigned NumObligatoryInputs() const { return fNumObligatoryInputs; }

         /* returns maximum possible eventnumber for overflow checking*/
         virtual unsigned int GetOverflowEventNumber() const;

         int ExecuteCommand(dabc::Command cmd) override;
   };
}


#endif
