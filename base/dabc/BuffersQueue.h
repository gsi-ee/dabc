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


namespace dabc {

   /** \brief Queue of buffers
    *
    * \ingroup dabc_all_classes
    */

   class BuffersQueue : protected Queue<Buffer, false> {

      protected:
         typedef Queue<Buffer, false> Parent;

      public:
         BuffersQueue(unsigned capacity) : Parent(capacity) {}

         virtual ~BuffersQueue() { Cleanup(); }

         bool PushBuffer(Buffer& buf)
         {
            Buffer* tgt = Parent::PushEmpty();
            if (tgt) {
               *tgt << buf;
               return true;
            }
            return false;
         }

         void PopBuffer(Buffer& buf) { buf << Front(); Parent::PopOnly(); }

         unsigned Size() const { return Parent::Size(); }

         unsigned Capacity() const { return Parent::Capacity(); }

         bool Full() const { return Parent::Full(); }

         bool Empty() const { return Parent::Empty(); }

         void Cleanup();

         void Cleanup(Mutex* m);

         /** Returns reference on the Buffer in the queue,
          * one can create any kind of buffer copies from it */
         Buffer& Item(unsigned n) const { return Parent::Item(n); }

         /** Return reference on the first item */
         Buffer& Front() const { return Parent::Front(); }

         /** Return reference on the last item */
         Buffer& Back() const { return Parent::Back(); }

   };

}


#endif
