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

#include "dabc/CommandDef.h"

#include "dabc/logging.h"
#include "dabc/Url.h"

void dabc::CommandDef::AddArg(const std::string& name, const std::string& typ_name)
{
   if (null()) return;

   Hierarchy chld = FindChild(name.c_str());
   if (chld.null()) chld = CreateChild(name);
   chld.Field("dabc:type").SetStr(typ_name);
}


dabc::Command dabc::CommandDef::CreateCommand(const std::string& query)
{
   dabc::Command res;
   if (null()) return res;

   res = dabc::Command(GetName());

   dabc::Url url(std::string("execute?")+query);

   DOUT0("CreateCommand query:%s", query.c_str());

   if (url.IsValid())
     for (unsigned n=0;n<NumChilds();n++) {
        Hierarchy h = GetChild(n);

        DOUT0("CreateCommand chld:%s", h.GetName());

        if (url.HasOption(h.GetName())) {
           int value = url.GetOptionInt(h.GetName());
           res.SetInt(h.GetName(), value);
        }
     }

   return res;
}
