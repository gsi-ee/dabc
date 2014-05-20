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

#include "hadaq/api.h"

hadaq::ReadoutModule::ReadoutModule(const std::string& name, dabc::Command cmd) :
   mbs::ReadoutModule(name, cmd),
   fIter2()
{
}

int hadaq::ReadoutModule::AcceptBuffer(dabc::Buffer& buf)
{
   if (fIter2.Reset(buf)) return dabc::cmd_true;

   return dabc::cmd_false;
}


hadaq::RawEvent* hadaq::ReadoutHandle::NextEvent(double tmout, double maxage)
{
   if (null()) return 0;

   bool intime = GetObject()->GetEventInTime(maxage);

   // check that HADAQ event can be produced
   // while hadaq events can be read only from file, ignore maxage parameter here
   if (intime && GetObject()->fIter2.NextEvent())
      return GetObject()->fIter2.evnt();

   // this is a case, when hadaq event packed into MBS event
   if (mbs::ReadoutHandle::NextEvent(tmout, maxage)!=0)
      return hadaq::ReadoutHandle::GetEvent();

   // check again that HADAQ event can be produced
   if (GetObject()->fIter2.NextEvent())
      return GetObject()->fIter2.evnt();

   return 0;
}

hadaq::RawEvent* hadaq::ReadoutHandle::GetEvent()
{
   if (null()) return 0;

   if (GetObject()->fIter2.evnt()) return GetObject()->fIter2.evnt();

   mbs::EventHeader* mbsev = mbs::ReadoutHandle::GetEvent();

   if (mbsev==0) return 0;

   mbs::SubeventHeader* mbssub = mbsev->NextSubEvent(0);

   if ((mbssub!=0) && (mbssub->FullSize() == mbsev->SubEventsSize())) {
      hadaq::RawEvent* raw = (hadaq::RawEvent*) mbssub->RawData();

      if (raw && (raw->GetSize() == mbssub->RawDataSize())) return raw;
   }
   return 0;
}
