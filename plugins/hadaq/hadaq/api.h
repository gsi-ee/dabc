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

#ifndef HADAQ_api
#define HADAQ_api

#ifndef MBS_api
#include "mbs/api.h"
#endif

#ifndef HADAQ_Iterator
#include "hadaq/Iterator.h"
#endif

namespace hadaq {

   class ReadoutHandle;

   class ReadoutModule : public mbs::ReadoutModule {
      protected:

         friend class ReadoutHandle;

         hadaq::ReadIterator fIter2;   ///< iterator over HADAQ buffers

         hadaq::RawEvent *fEv1 = nullptr;
         hadaq::RawSubevent *fSub1 = nullptr;

         int AcceptBuffer(dabc::Buffer& buf) override;

      public:

         ReadoutModule(const std::string &name, dabc::Command cmd);

   };


   class ReadoutHandle : protected mbs::ReadoutHandle {

      DABC_REFERENCE(ReadoutHandle, mbs::ReadoutHandle, hadaq::ReadoutModule)

      /** Connect with data source */
      static ReadoutHandle Connect(const std::string &url, int bufsz_mb = 8);

      /** Return true if handle not initialized */
      bool null() const { return mbs::ReadoutHandle::null(); }

      /** Disconnect from MBS server */
      bool Disconnect() { return mbs::ReadoutHandle::Disconnect(); }

      /** Retrieve next event from the server */
      hadaq::RawEvent *NextEvent(double tm = 1.0, double maxage = -1.);

      /** Get current event pointer */
      hadaq::RawEvent *GetEvent();

      /** Get current Tu pointer - if any */
      hadaq::HadTu *GetTu();

      /** Retrieve next event or hadtu or partial subevent from Input combiner */
      bool NextSubEventsBlock(double tm = 1.0, double maxage = -1.);

      /** Retrieve next subevent from the block */
      hadaq::RawSubevent *NextSubEvent();
   };

}


#endif
