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

#ifndef DOGMA_api
#define DOGMA_api

#ifndef MBS_api
#include "mbs/api.h"
#endif

#ifndef DOGMA_Iterator
#include "dogma/Iterator.h"
#endif

namespace dogma {

   class ReadoutHandle;

   class ReadoutModule : public mbs::ReadoutModule {
      protected:

         friend class ReadoutHandle;

         dogma::RawIterator fIter2;   ///< iterator over DOGMA buffers

         int AcceptBuffer(dabc::Buffer &buf) override;

      public:

         ReadoutModule(const std::string &name, dabc::Command cmd);
   };


   class ReadoutHandle : protected mbs::ReadoutHandle {

      DABC_REFERENCE(ReadoutHandle, mbs::ReadoutHandle, dogma::ReadoutModule)

      /** Connect with data source */
      static ReadoutHandle Connect(const std::string &url, int bufsz_mb = 16);

      /** Return true if handle not initialized */
      bool null() const { return mbs::ReadoutHandle::null(); }

      /** Disconnect from MBS server */
      bool Disconnect() { return mbs::ReadoutHandle::Disconnect(); }

      unsigned NextPortion(double tm = 1.0, double maxage = -1.);

      unsigned GetKind(bool onlymbs = false);

      bool IsData() { return GetKind() != 0; };

      /** Get current raw data pointer */
      DogmaTu *GetTu();

      /** Get current event pointer */
      DogmaEvent *GetEvent();
   };

}


#endif
