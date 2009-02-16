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
/* -------> Swapped <-------- */
/* --------=========--------- */
struct s_flt_descr {
      /* indices to filter[]  */
      char                   h_fltblkend;       /* end filter block          */
      char                   h_fltblkbeg;   /* begin filter block        */
      char                   hf_fltdescr;       /* filter descriptor         */
      char                   hf_wrtdescr;   /* write descriptor          */
      /* index to flt_descr[] */
      short                  i_descriptors;     /* number of descriptors     */
      char                   h_dummy;
      char                   h_nextdescr;       /* next descriptor           */
      };
