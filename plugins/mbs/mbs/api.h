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

         mbs::ReadIterator fIter;    ///< iterator, accessed only from user side
         dabc::TimeStamp fLastNotFullTm; ///< last time when queue is not full
         dabc::TimeStamp fCurBufTm;      ///< time when buffer analysis starts
         dabc::Command fCmd;             ///< current nextbuffer cmd

         int ExecuteCommand(dabc::Command cmd) override;

         void ProcessData();

         void ProcessInputEvent(unsigned port) override;

         void ProcessTimerEvent(unsigned timer) override;

         virtual int AcceptBuffer(dabc::Buffer&) { return dabc::cmd_false; }

         bool GetEventInTime(double maxage);

      public:

         ReadoutModule(const std::string &name, dabc::Command cmd);

   };

   // =================================================================================

   /** Handle to organize readout of MBS data source */

   class ReadoutHandle : protected dabc::ModuleAsyncRef {

      DABC_REFERENCE(ReadoutHandle, dabc::ModuleAsyncRef, ReadoutModule)

      /** Connect with MBS server */
      static ReadoutHandle Connect(const std::string &url, int bufsz_mb = 8)
      {
         return DoConnect(url, "mbs::ReadoutModule", bufsz_mb);
      }

      /** Check if handle is initialized */
      bool null() const { return dabc::ModuleAsyncRef::null(); }

      /** Disconnect from MBS server */
      bool Disconnect();

      /** Retrieve next event from the server
       * One could specify timeout (how long one should wait for next event, default 1 s) and
       * maximal age of the event on client side (default -1)
       * Maximal age parameters allows to skip old events, analyzing only newest*/
      mbs::EventHeader* NextEvent(double tmout = 1.0, double maxage = -1.);

      /** Get current event pointer */
      mbs::EventHeader* GetEvent();

      protected:

      static ReadoutHandle DoConnect(const std::string &url, const char *classname, int bufsz_mb = 8);

   };

   // ==========================================================================

   class MonitorHandle : protected dabc::ModuleAsyncRef {

       DABC_REFERENCE(MonitorHandle, dabc::ModuleAsyncRef, Monitor)

       /** Connect with MBS node */
       static MonitorHandle Connect(const std::string &mbsnode, int cmdport = 6019, int logport = 6007, int statport = 6008);

       /** Check if handle is initialized */
       bool null() const { return dabc::ModuleAsyncRef::null(); }

       /** Release connection to the MBS node */
       bool Disconnect();

       /** Execute MBS command */
       bool MbsCmd(const std::string &cmd, double tmout=5.);
   };

}


#endif
