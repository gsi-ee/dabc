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


#ifndef FESA_defines
#define FESA_defines

#include <stdint.h>
#include <math.h>

namespace fesa {

   struct BeamProfile {
      enum { sizex = 16, sizey = 16 };

      uint32_t arr[sizex][sizey];

      void clear()
      {
         for (unsigned x=0;x<sizex;x++)
            for (unsigned y=0;y<sizey;y++)
               arr[x][y] = 0;
      }

      void fill(int kind = 0)
      {
         for (unsigned x=0;x<sizex;x++)
            for (unsigned y=0;y<sizey;y++)
               switch (kind) {
                  case 0: arr[x][y] = x+y; break;
                  case 1: arr[x][y] = sizex - x + sizey-y; break;
                  case 2: arr[x][y] = sizex - x + y; break;
                  case 3: arr[x][y] = x + sizey-y; break;
                  default: {
                     arr[x][y] = (unsigned) (sizex + sizey - 3*sqrt((x-sizex/2)*(x-sizex/2) + (y-sizey/2)*(y-sizey/2)));
                  }
               }
      }
   };

}

#endif
