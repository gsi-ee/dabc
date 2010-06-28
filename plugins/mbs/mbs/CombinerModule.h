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
#ifndef MBS_CombinerModule
#define MBS_CombinerModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef MBS_Iterator
#include "mbs/Iterator.h"
#endif

#include <vector>


namespace mbs {



   class CombinerModule : public dabc::ModuleAsync {
      protected:
         dabc::PoolHandle*          fPool;
         unsigned                   fBufferSize;
         std::vector<ReadIterator>  fInp;
         WriteIterator              fOut;
         dabc::Buffer*              fOutBuf;
         int                        fTmCnt;

         bool                       fFileOutput;
         bool                       fServOutput;

         /* switch between partial combining of smallest event ids (false)
          * and building of complete events only (true)*/
         bool						fBuildCompleteEvents;

         /*switch on checking duplicate subevent ids in merged events -> indicate setup error*/
         bool						fCheckSubIds;

         /* defines maximum difference allowed between event id in merged data streams.
          * if id difference is larger, combiner may stop with error message*/
         unsigned int				fEventIdTolerance;

         dabc::RateParameter*      fEvntRate;
         dabc::RateParameter*      fDataRate;

         bool BuildEvent();
         bool FlushBuffer();

      public:
         CombinerModule(const char* name, dabc::Command* cmd = 0);
         virtual ~CombinerModule();

         virtual void ProcessInputEvent(dabc::Port* port);
         virtual void ProcessOutputEvent(dabc::Port* port);

         virtual void ProcessTimerEvent(dabc::Timer* timer);

         bool IsFileOutput() const { return fFileOutput; }
         bool IsServOutput() const { return fServOutput; }
         bool IsBuildCompleteEvents() const { return fBuildCompleteEvents; }
         bool IsCheckSubIds() const { return fCheckSubIds; }

         unsigned int GetEventIdTolerance() const { return fEventIdTolerance; }

         virtual int ExecuteCommand(dabc::Command* cmd);
   };

}


#endif
