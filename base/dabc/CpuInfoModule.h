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

#ifndef DABC_CpuInfoModule
#define DABC_CpuInfoModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_statistic
#include "dabc/statistic.h"
#endif

namespace dabc {

   /** \brief %Module provides CPU information
    *
    * \ingroup dabc_all_classes
    */

   class CpuInfoModule : public dabc::ModuleAsync {
      protected:
         CpuStatistic    fStat;
         /** value of "Kind" configuration parameter, mask for
          *  0 - just show cpu info as double parameter
          *  1 - show CPUinfo as rate parameter
          *  2 - show infos for every cpu
          *  4 - show info for every cpu as ratemeters
          *  8 - show memory usage as ratemeter
          */
         int             fKind{0};

      public:
         CpuInfoModule(const std::string &name, dabc::Command cmd = nullptr, int kind = -1);

         void ProcessTimerEvent(unsigned) override;
   };

}

#endif
