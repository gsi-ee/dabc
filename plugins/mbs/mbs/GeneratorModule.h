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
#ifndef MBS_GeneratorModule
#define MBS_GeneratorModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

namespace mbs {

   class GeneratorModule : public dabc::ModuleAsync {

      protected:

         dabc::PoolHandle*       fPool;

         dabc::BufferSize_t      fBufferSize;

         uint32_t    fEventCount;
         uint32_t    fStartStopPeriod;
         uint16_t    fNumSubevents;
         uint16_t    fFirstProcId;
         uint32_t    fSubeventSize;
         bool        fIsGo4RandomFormat;
         uint32_t    fFullId; /** subevent id, if number subevents==1 and nonzero */

         void   FillRandomBuffer(dabc::Buffer* buf);

      public:
         GeneratorModule(const char* name, dabc::Command* cmd = 0);

         virtual void ProcessOutputEvent(dabc::Port* port);
   };


   class ReadoutModule : public dabc::ModuleAsync {

      public:
         ReadoutModule(const char* name, dabc::Command* cmd = 0);

         virtual void ProcessInputEvent(dabc::Port* port);
   };

   class TransmitterModule : public dabc::ModuleAsync {
         void retransmit();

      public:
      	TransmitterModule(const char* name, dabc::Command* cmd = 0);

         virtual void ProcessInputEvent(dabc::Port*) { retransmit(); }

         virtual void ProcessOutputEvent(dabc::Port*) { retransmit(); }
   };

}


#endif
