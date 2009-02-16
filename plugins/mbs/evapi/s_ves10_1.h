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
/* Generated from SA$VES10_1.TXT  */
/* Swapping enabled           */
  /*  ================= GSI VME Subevent header =======================  */
typedef struct
{
INTS4  l_dlen;   /*  Data length +2 in words */
INTS2 i_subtype;
INTS2 i_type;
CHARS  h_control;   /*  Processor type code */
CHARS  h_subcrate;   /*  Subcrate number */
INTS2 i_procid;   /*  Processor ID [as loaded from VAX] */
} s_ves10_1 ;
  /* ------------------------------------------------------------------ */
