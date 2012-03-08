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

#include "dabc/BuffersQueue.h"

#include "dabc/threads.h"

bool dabc::BuffersQueue::Push(const Buffer& buf, Mutex* m)
{
   if (buf.null()) return true;

   dabc::LockGuard lock(m);

   if (MakePlaceForNext()) {
      Push(buf);
      return true;
   }

   return false;
}

void dabc::BuffersQueue::Cleanup()
{
   while (Size()>0)
     Pop().Release();
}

void dabc::BuffersQueue::Cleanup(Mutex* m)
{
   Buffer buf;

   do {
      buf.Release();

      dabc::LockGuard lock(m);

      if (Size()>0) PopBuffer(buf);

   } while (!buf.null());

}
