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
#ifndef F_EVHE_SWAP
#define F_EVHE_SWAP


#include "typedefs.h"
typedef struct
{
INTS4  l_dlen;   /*  Length of data in words */
INTS2 i_type;
INTS2 i_subtype;
} s_evhe;


#endif
