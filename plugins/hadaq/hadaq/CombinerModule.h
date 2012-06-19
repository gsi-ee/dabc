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

               /** keeps current event number */
               hadaq::EventNumType curr_evnt_num;

               /** indicates if input was selected for event building */
               bool selected;

               /** indicates if input has valid data */
               bool valid;

               InputCfg() :
                     curr_evnt_num(0),
                     selected(false),
                     valid(false)
               {
               }

               InputCfg(const InputCfg& src) :
                     curr_evnt_num(src.curr_evnt_num),
                     selected(src.selected),
                     valid(src.valid)
               {
               }

               void Reset()
               {
                  curr_evnt_num = 0;
                  selected = false;
                  valid = false;
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
         bool FlushBuffer();

         virtual void BeforeModuleStart();

         virtual void AfterModuleStop();

         bool ShiftToNextEvent(unsigned ninp);

         /** Method should be used to skip current buffer from the queue */
         bool ShiftToNextBuffer(unsigned ninp);

         hadaq::EventNumType CurrEventId(unsigned int ninp) const { return fCfg[ninp].curr_evnt_num; }

         void SetInfo(const std::string& info, bool forceinfo = false);



         unsigned                   fBufferSize;

         std::vector<ReadIterator> fInp;
         std::vector<InputCfg> fCfg;
         WriteIterator fOut;
         dabc::Buffer fOutBuf;
         bool fFlushFlag;

         bool fDoOutput;
         bool fFileOutput;
         bool fServOutput;

         /* switch between partial combining of smallest event ids (false)
                 * and building of complete events only (true)*/
         bool                          fBuildCompleteEvents;

         std::string                fEventRateName;
         std::string                fDataRateName;
         std::string                fInfoName;


   };

}

#endif
