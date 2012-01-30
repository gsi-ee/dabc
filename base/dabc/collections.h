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

#ifndef DABC_collections
#define DABC_collections

#include <stdint.h>
#include <string.h>

#ifndef DABC_logging
#include "dabc/logging.h"
#endif

#ifndef DABC_defines
#include "dabc/defines.h"
#endif

#ifndef DABC_Queue
#include "dabc/Queue.h"
#endif


#include <vector>

namespace dabc {

   class Mutex;
   class Command;
   class Object;
   class Buffer;

   // ____________________________________________________________

   class PointersVector : public std::vector<void*> {
      public:
         PointersVector() : std::vector<void*>() {}

         void* pop()
         {
            if (size()==0) return 0;
            void* res = back();
            pop_back();
            return res;
         }

         bool has_ptr(void* item) const
         {
            for (unsigned n=0; n<size(); n++)
               if (at(n)==item) return true;
            return false;
         }

         bool remove_at(unsigned n)
         {
            if (n>=size()) return false;
            erase(begin()+n);
            return true;
         }

         bool remove(void* item)
         {
            for (iterator iter = begin(); iter!=end(); iter++)
               if (*iter == item) {
                  erase(iter);
                  return true;
               }
            return false;
         }
   };

};

#endif
