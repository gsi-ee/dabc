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
#ifndef S_VE10_1_SWAP
#define S_VE10_1_SWAP

#include "typedefs.h"
typedef struct
{
INTS4  l_dlen;   /*  Data length + 4 in words */
INTS2 i_type;
INTS2 i_subtype;
INTS2 i_dummy;   /*  Not used yet */
INTS2 i_trigger;   /*  Trigger number */
INTS4  l_count;   /*  Current event number */
} s_ve10_1 ;

#endif
