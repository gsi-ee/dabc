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
/* +++ s_filter.h +++
 * client structure for SBS monitor
 * (see also s_filter.h for GPS-Server on (Open)VMS / clients)
 * R.S. Mayer
 * 18-Feb-1994
*/
struct s_filter
      {
      INTU4          l_pattern;
      INTS4                   l_offset;       /* offset >0: LW  <0: Words     */
      INTU4          l_opcode;
      };
