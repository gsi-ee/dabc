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

hadaq::ReadoutModule::ReadoutModule(const std::string &name, dabc::Command cmd) :
   mbs::ReadoutModule(name, cmd),
   fIter2()
{
}

int hadaq::ReadoutModule::AcceptBuffer(dabc::Buffer& buf)
{
   if (fIter2.Reset(buf))
      return dabc::cmd_true;

   return dabc::cmd_false;
}

// =========================================================================


hadaq::ReadoutHandle hadaq::ReadoutHandle::Connect(const std::string &src)
{
   std::string newurl = src;

   if (((newurl.find(".hld") != std::string::npos) || (newurl.find(".HLD") != std::string::npos)) &&
         (newurl.find("hld://") == std::string::npos))
      newurl = std::string("hld://") + src;

   return DoConnect(newurl, "hadaq::ReadoutModule");
}


hadaq::RawEvent *hadaq::ReadoutHandle::NextEvent(double tmout, double maxage)
{
   if (null()) return nullptr;

   bool intime = GetObject()->GetEventInTime(maxage);

   // check that HADAQ event can be produced
   // while hadaq events can be read only from file, ignore maxage parameter here
   if (intime && GetObject()->fIter2.NextEvent())
      return GetObject()->fIter2.evnt();

   // this is a case, when hadaq event packed into MBS event
   if (mbs::ReadoutHandle::NextEvent(tmout, maxage))
      return hadaq::ReadoutHandle::GetEvent();

   // check again that HADAQ event can be produced
   if (GetObject()->fIter2.NextEvent())
      return GetObject()->fIter2.evnt();

   return nullptr;
}

hadaq::RawEvent *hadaq::ReadoutHandle::GetEvent()
{
   if (null()) return nullptr;

   if (GetObject()->fIter2.evnt()) return GetObject()->fIter2.evnt();

   mbs::EventHeader *mbsev = mbs::ReadoutHandle::GetEvent();

   if (!mbsev) return nullptr;

   mbs::SubeventHeader *mbssub = mbsev->NextSubEvent(nullptr);

   if (mbssub && (mbssub->FullSize() == mbsev->SubEventsSize())) {
      hadaq::RawEvent *raw = (hadaq::RawEvent*) mbssub->RawData();

      if (raw && (raw->GetSize() == mbssub->RawDataSize())) return raw;
   }
   return nullptr;
}
