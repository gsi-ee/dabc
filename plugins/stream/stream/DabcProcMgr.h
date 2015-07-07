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

#ifndef STREAM_DabcProcMgr
#define STREAM_DabcProcMgr

#include "base/ProcMgr.h"

#include "dabc/Hierarchy.h"

namespace stream {

   class DabcProcMgr : public base::ProcMgr {
      protected:

         dabc::Hierarchy fTop;

      public:
         DabcProcMgr() : base::ProcMgr(), fTop() {}
         virtual ~DabcProcMgr() {}

         void SetTop(dabc::Hierarchy& top) { fTop = top; }

         // redefine only make procedure, fill and clear should work
         virtual base::H1handle MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle = 0);

         virtual base::H2handle MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options = 0);

   };

}

#endif
