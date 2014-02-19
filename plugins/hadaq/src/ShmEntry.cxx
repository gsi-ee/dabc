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

#include "hadaq/ShmEntry.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/XmlEngine.h"

#include <string.h>

hadaq::ShmEntry::ShmEntry(const std::string& statsname, const std::string& shmemname, ::Worker* handle, dabc::Parameter& par) :
   fStatsName(statsname),
   fShmName(shmemname),
   fWorker(handle),
   fShmPtr(0),
   fPar(par)
{
   // here hadaq worker add statistics etc.
   fShmPtr = ::Worker_addStatistic(fWorker,fStatsName.c_str());
}

hadaq::ShmEntry::~ShmEntry()
{
}

void hadaq::ShmEntry::UpdateValue(const std::string& value)
{
   int i(0);
   if (dabc::str_to_int(value.c_str(), &i) && (fShmPtr!=0))
      *fShmPtr = i;
}

void hadaq::ShmEntry::UpdateParameter()
{
   if(!fPar.null())
      fPar.SetValue((int)GetValue());
}
