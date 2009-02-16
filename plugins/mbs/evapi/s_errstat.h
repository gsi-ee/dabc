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
#include "typedefs.h"
/*  s_errstat.h
 *  ===========
 *  purpose        : structure for f_ut_error control
 *  date           : 22-Nov-1994
 *  author         : R.S. Mayer
 *
 *  update         : 12-Jan-1995: Message log flag /RSM
 *
 */
#ifndef __S_ERRSTAT__

   struct s_errstat
   {
      INTS4          if_verbose;
      INTS4          if_msglog;
      CHARS         c_taskname[16];
   };

#define __S_ERRSTAT__
#endif
