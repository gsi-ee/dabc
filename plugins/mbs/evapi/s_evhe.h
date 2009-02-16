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
/* Generated from EE$ROOT:[GOOFY.VME]SA$EVHE.VMETEMP;  */
/* Swapping enabled           */
  /*  ================= GSI VME General event header ===============  */
typedef struct
{
INTS4  l_dlen;   /*  Length of data in words */
INTS2 i_subtype;
INTS2 i_type;
} s_evhe;
  /* ------------------------------------------------------------------ */
