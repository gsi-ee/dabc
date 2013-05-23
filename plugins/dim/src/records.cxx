/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "dimc/records.h"

namespace dimc {

   const char* desc_HistogramRec = "L:1;F:1;F:1;C:32;C:32;C:16;L";
   const char* desc_StatusRec    = "L:1;C:16;C:16";
   const char* desc_InfoRec      = "L:1;C:16;C:128";
   const char* desc_RateRec      = "F:1;L:1;F:1;F:1;F:1;F:1;C:16;C:16;C:16";

   const char* col_Red     =  "Red";
   const char* col_Green   =  "Green";
   const char* col_Blue    =  "Blue";
   const char* col_Cyan    =  "Cyan";
   const char* col_Yellow  =  "Yellow";
   const char* col_Magenta =  "Magenta";
}
