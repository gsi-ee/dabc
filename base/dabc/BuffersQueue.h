// $Id$

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

#ifndef DABC_BuffersQueue
#define DABC_BuffersQueue

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

#ifndef DABC_Queue
#include "dabc/Queue.h"
#endif

#include <vector>

namespace dabc {

/** \brief %Queue of buffers
 *
 * \ingroup dabc_all_classes
 *
 *  Organizes queue of dabc::Buffer objects in memory.
 *  Locking should be done outside, when necessary.
 */

   class BuffersQueue {

      protected:
         std::vector<dabc::Buffer> vect;

         unsigned front;
         unsigned tail;
         unsigned size;

      public:
         BuffersQueue(unsigned capacity) :
            vect(),
            front(0),
            tail(0),
            size(0)
         {
            for (unsigned n=0;n<capacity;n++)
               vect.push_back(dabc::Buffer());
         }

         virtual ~BuffersQueue() { Cleanup(); }

         bool PushBuffer(Buffer& buf)
         {
            if (size == vect.size()) return false;
            vect[tail] << buf;
            size++;
            tail = (tail+1) % vect.size();
            return true;
         }

         bool PopBuffer(Buffer& buf)
         {
            if (size == 0) return false;
            buf << vect[front];
            size--;
            front = (front+1) % vect.size();
            return true;
         }

         unsigned Size() const { return size; }

         unsigned Capacity() const { return vect.size(); }

         bool Full() const { return Size() == Capacity(); }

         bool Empty() const { return size == 0; }

         void Cleanup();

         void Cleanup(Mutex* m);

         /** Returns reference on the Buffer in the queue,
          * one can create any kind of buffer copies from it */
         Buffer Item(unsigned n) const
         {
            return this->vect[(n+front) % vect.size()];
         }

   };

}


#endif
