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
#ifndef S_VES10_1_SWAP
#define S_VES10_1_SWAP

#include "typedefs.h"
typedef struct
{
INTS4  l_dlen;   /*  Data length +2 in words */
INTS2 i_type;
INTS2 i_subtype;
INTS2 i_procid;   /*  Processor ID [as loaded from VAX] */
CHARS  h_subcrate;   /*  Subcrate number */
CHARS  h_control;   /*  Processor type code */
} s_ves10_1 ;

#endif
