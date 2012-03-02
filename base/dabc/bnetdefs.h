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

#ifndef DABC_bnetdefs
#define DABC_bnetdefs

#include <stdint.h>

namespace bnet {

   typedef uint64_t EventId;

   enum { MAXLID = 16 };

   struct TimeSyncMessage
   {
      int64_t msgid;
      double  master_time;
      double  slave_shift;
      double  slave_time;
      double  slave_scale;
   };


}


#endif
