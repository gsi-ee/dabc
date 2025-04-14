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

#ifndef DABC_RootTreeOutput
#define DABC_RootTreeOutput

#include "dabc/DataIO.h"

class TTree;

namespace mbs_root {

    class DabcEvent;

   class RootTreeOutput : public dabc::FileOutput {
      public:

         RootTreeOutput(const dabc::Url& url);
         virtual ~RootTreeOutput();

         bool Write_Init(const dabc::WorkerRef &wrk, const dabc::Command &cmd) override;

         unsigned Write_Buffer(dabc::Buffer& buf) override;

      protected:

         TTree                *fTree{nullptr};
         mbs_root::DabcEvent  *fEvent{nullptr};

         int       fSplit{0};
         int       fTreeBuf{0};
         int       fCompression{0};
         int       fMaxSize;

         bool Close();

   };

}

#endif
