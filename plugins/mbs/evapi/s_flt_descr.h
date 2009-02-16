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
/* filter descriptor */
struct s_flt_descr
      {
      char                   hf_wrtdescr;   /* write descriptor          */
      char                   hf_fltdescr;       /* filter descriptor         */
      /* indices to filter[]  */
      char                   h_fltblkbeg;   /* begin filter block        */
      char                   h_fltblkend;       /* end filter block          */
      /* index to flt_descr[] */
      char                   h_nextdescr;       /* next descriptor           */
      char                   h_dummy;
      short                  i_descriptors;     /* number of descriptors     */
      };

