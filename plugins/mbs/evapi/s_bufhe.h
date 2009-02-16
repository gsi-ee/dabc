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
/* Generated from SA$BUFHE.TXT  */
/* Swapping enabled           */
  /*  ================= GSI Buffer header =======================  */
typedef struct
{
INTS4  l_dlen;   /*  Length of data field in words */
INTS2 i_subtype;
INTS2 i_type;
CHARS  h_begin;   /*  Fragment at end of buffer */
CHARS  h_end;   /*  Fragment at begin of buffer */
INTS2 i_used;   /*  Used length of data field in words */
INTS4  l_buf;   /*  Current buffer number */
INTS4  l_evt;   /*  Number of fragments */
INTS4  l_current_i;   /*  Index, temporarily used */
INTS4  l_time[2];
INTS4  l_free[4];
} s_bufhe;
