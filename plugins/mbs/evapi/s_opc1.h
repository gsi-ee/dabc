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
/* struct for filter opcode                  */
struct s_opc1
      {                         /* ++ byte1 (LSB) ++ */
      unsigned      b1_evtsev :1;       /* 1:event/0:subevt active for sel */
      unsigned      b1_selflt:1;        /* select event/subevent filter */
      unsigned      b1_selwrt:1;        /* select write of evt/sev          */
      unsigned      b3_opc   :3;        /* opcode                    */
      unsigned      b1_lnkf1 :1;        /* lnk filters: and:1 or:0   */
      unsigned      b1_lnkf2 :1;        /* lnk different flts blks   */
      char          h_fltspec;          /* ++ byte2  filter specification */
      char          h_next_fltblk;      /* ++ byte3 ++  next filter blk      */
      char          h_flt_len;          /* ++ byte4 (MSB) ++ length of filter*/
      };

/* REM : h_next_fltblk and h_flt_len are ABSOLUTE adresses  !!! */
