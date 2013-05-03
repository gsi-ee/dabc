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

#ifndef MBS_GeneratorModule
#define MBS_GeneratorModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif


namespace mbs {

   class GeneratorInput : public dabc::DataInput {
      protected:
         uint32_t    fEventCount;
         uint16_t    fNumSubevents;
         uint16_t    fFirstProcId;
         uint32_t    fSubeventSize;
         bool        fIsGo4RandomFormat;
         uint32_t    fFullId; /** subevent id, if number subevents==1 and nonzero */
      public:
         GeneratorInput(const dabc::Url& url);
         virtual ~GeneratorInput() {}

         virtual bool Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd);

         virtual unsigned Read_Size() { return dabc::di_DfltBufSize; }

         virtual unsigned Read_Complete(dabc::Buffer& buf);
   };

   // ===================================================================

   class GeneratorModule : public dabc::ModuleAsync {

      protected:

         uint32_t    fEventCount;
         uint16_t    fNumSubevents;
         uint16_t    fFirstProcId;
         uint32_t    fSubeventSize;
         bool        fIsGo4RandomFormat;
         uint32_t    fFullId; /** subevent id, if number subevents==1 and nonzero */

         bool        fShowInfo;   //!< enable/disable output of generator info

         void   FillRandomBuffer(dabc::Buffer& buf);

         virtual void BeforeModuleStart();

      public:
         GeneratorModule(const std::string& name, dabc::Command cmd = 0);

         virtual bool ProcessSend(unsigned port);

         virtual int ExecuteCommand(dabc::Command cmd);
   };


   class ReadoutModule : public dabc::ModuleAsync {

      public:
         ReadoutModule(const std::string& name, dabc::Command cmd = 0);

         virtual bool ProcessRecv(unsigned port);
   };

   class TransmitterModule : public dabc::ModuleAsync {

      protected:

         bool  fReconnect; // indicate that module should try to reconnect input rather than stop execution

         bool retransmit();

      public:
      	TransmitterModule(const std::string& name, dabc::Command cmd = 0);

         virtual bool ProcessRecv(unsigned port);

         virtual bool ProcessSend(unsigned port);

         virtual void ProcessConnectEvent(const std::string& name, bool on);

         virtual void ProcessTimerEvent(unsigned timer);

         virtual void BeforeModuleStart();
   };

}


#endif
