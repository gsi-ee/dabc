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

#include "dogma/api.h"

dogma::ReadoutModule::ReadoutModule(const std::string &name, dabc::Command cmd) :
   mbs::ReadoutModule(name, cmd),
   fIter2("dogma_iter")
{
}

int dogma::ReadoutModule::AcceptBuffer(dabc::Buffer& buf)
{
   if (fIter2.Assign(buf))
      return dabc::cmd_true;

   return dabc::cmd_false;
}

// =========================================================================


dogma::ReadoutHandle dogma::ReadoutHandle::Connect(const std::string &src)
{
   std::string newurl = src;

   //if (((newurl.find(".hld") != std::string::npos) || (newurl.find(".HLD") != std::string::npos)) &&
   //      (newurl.find("hld://") == std::string::npos))
   //   newurl = std::string("hld://") + src;

   return DoConnect(newurl, "dogma::ReadoutModule");
}

unsigned dogma::ReadoutHandle::NextPortion(double tmout, double maxage)
{
   if (null())
      return 0;

   bool intime = GetObject()->GetEventInTime(maxage);

   // check that DOGMA event can be produced
   // while dogma events can be read only from file, ignore maxage parameter here
   if (intime && GetObject()->fIter2.NextEvent())
      return GetObject()->fIter2.EventKind();

   // this is a case, when dogma event packed into MBS event
   if (mbs::ReadoutHandle::NextEvent(tmout, maxage))
      return GetKind(true);

   // check again that DOGMA event can be produced
   if (GetObject()->fIter2.NextEvent())
      return GetObject()->fIter2.EventKind();

   return 0;
}

unsigned dogma::ReadoutHandle::GetKind(bool onlymbs)
{
   if (null())
      return 0;

   if (!onlymbs && GetObject()->fIter2.Event())
      return GetObject()->fIter2.EventKind();

   auto mbsev = mbs::ReadoutHandle::GetEvent();
   if (!mbsev)
      return 0;

   auto mbssub = mbsev->NextSubEvent(nullptr);
   return mbssub ? mbssub->Control() + 1 : 0;
}


dogma::DogmaTu *dogma::ReadoutHandle::GetTu()
{
   if (null())
      return nullptr;

   auto direct = GetObject()->fIter2.Event();
   if (direct)
      return (GetObject()->fIter2.EventKind() == 1) ? (dogma::DogmaTu *) direct : nullptr;

   auto mbsev = mbs::ReadoutHandle::GetEvent();
   if (!mbsev)
      return nullptr;

   auto mbssub = mbsev->NextSubEvent(nullptr);

   if (mbssub && (mbssub->FullSize() == mbsev->SubEventsSize()) && (mbssub->Control() == 0)) {
      auto tu = (dogma::DogmaTu *) mbssub->RawData();

      if (tu && (tu->GetSize() == mbssub->RawDataSize()))
         return tu;
   }

   return nullptr;
}

dogma::DogmaEvent *dogma::ReadoutHandle::GetEvent()
{
   if (null())
      return nullptr;

   auto direct = GetObject()->fIter2.Event();
   if (direct)
      return (GetObject()->fIter2.EventKind() == 2) ? (dogma::DogmaEvent *) direct : nullptr;

   mbs::EventHeader *mbsev = mbs::ReadoutHandle::GetEvent();
   if (!mbsev)
      return nullptr;

   auto mbssub = mbsev->NextSubEvent(nullptr);

   if (mbssub && (mbssub->FullSize() == mbsev->SubEventsSize()) && (mbssub->Control() == 1)) {
      auto evnt = (dogma::DogmaEvent *) mbssub->RawData();

      if (evnt && (evnt->GetEventLen() == mbssub->RawDataSize()))
         return evnt;
   }

   return nullptr;
}
