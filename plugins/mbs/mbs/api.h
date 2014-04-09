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

#ifndef MBS_api
#define MBS_api


#ifndef MBS_MbsTypeDefs
#include "mbs/MbsTypeDefs.h"
#endif

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef MBS_Iterator
#include "mbs/Iterator.h"
#endif

#ifndef MBS_Monitor
#include "mbs/Monitor.h"
#endif

namespace mbs {

   class ReadoutHandle;

   /** Module, used to readout MBS data source and provide data to the user */

   class ReadoutModule : public dabc::ModuleAsync {
      protected:

         friend class ReadoutHandle;

         mbs::ReadIterator fIter;   ///< iterator, accessed only from user side
         dabc::Command fCmd;        ///< current nextbuffer cmd

         virtual int ExecuteCommand(dabc::Command cmd);

         virtual void ProcessInputEvent(unsigned port);

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int AcceptBuffer(dabc::Buffer& buf) { return dabc::cmd_false; }

      public:

         ReadoutModule(const std::string& name, dabc::Command cmd);

   };

   // =================================================================================

   /** Handle to organize readout of MBS data source */

   class ReadoutHandle : protected dabc::ModuleAsyncRef {

      DABC_REFERENCE(ReadoutHandle, dabc::ModuleAsyncRef, ReadoutModule)

      /** Connect with MBS server */
      static ReadoutHandle Connect(const std::string& url)
      {
         return DoConnect(url, "mbs::ReadoutModule");
      }

      /** Check if handle is initialized */
      bool null() const { return dabc::ModuleAsyncRef::null(); }

      /** Disconnect from MBS server */
      bool Disconnect();

      /** Retrieve next event from the server */
      mbs::EventHeader* NextEvent(double tm = 1.0);

      /** Get current event pointer */
      mbs::EventHeader* GetEvent();

      protected:


      static ReadoutHandle DoConnect(const std::string& url, const char* classname);

   };

   // ==========================================================================

   class MonitorHandle : protected dabc::ModuleAsyncRef {

       DABC_REFERENCE(MonitorHandle, dabc::ModuleAsyncRef, Monitor)

       /** Connect with MBS node */
       static MonitorHandle Connect(const std::string& mbsnode, int cmdport = 6019, int logport = 6007);

       /** Release connection to the MBS node */
       bool Disconnect();

       /** Execute MBS command */
       bool MbsCmd(const std::string& cmd, double tmout=5.);
   };

}


#endif
