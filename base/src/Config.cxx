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

#include "dabc/Config.h"

#include "dabc/Worker.h"
#include "dabc/ConfigIO.h"
#include "dabc/Manager.h"


dabc::ConfigContainer::ConfigContainer(const std::string& name, Command cmd, Reference worker) :
   dabc::RecordContainer("abc"),
   fName(name),
   fCmd(cmd),
   fPar(),
   fWorker(worker),
   fReadFlag(-1)
{
//   DOUT0(("Create config container %p for %s", this, name.c_str()));
}

const char* dabc::ConfigContainer::GetField(const std::string& name, const char* dflt)
{
   const char* res = 0;

   // first search if command has specified argument

   if (!fName.empty() && name.empty()) res = fCmd.GetField(fName, 0); else
   if (fName.empty() && !name.empty()) res = fCmd.GetField(name, 0);

   DOUT3(("CFG1: GETFIELD name:%s  fName:%s  res = %s", name.c_str(), fName.c_str(), (res ? res : "---")));

   if (res!=0) return res;

   // second, check parameter of worker itself - it was read already

   std::string parname = fName.empty() ? name : fName;
   if (fPar.null() && !parname.empty()) {
      Worker* w = dynamic_cast<Worker*> (fWorker());
      if (w) fPar = w->Par(parname);
   }
   res = fPar.GetField(fName.empty() ? "" : name, 0);
   if (res!=0) return res;

   // third, try to read from xml file

   if (fReadFlag<0) {

      ConfigIO io(dabc::mgr()->cfg());

      // indicate that setfield method could be applied
      fReadFlag = 0;

      DOUT3(("Start reading %s from xml worker %s cont:%p!!!", fName.c_str(), fWorker.GetName(), this));

      io.ReadRecord(fWorker(), fName, this);

      DOUT3(("Did reading %s from xml worker %s!!!", fName.c_str(), fWorker.GetName()));

      fReadFlag = 1;
   }

   res = dabc::RecordContainer::GetField(name, 0);

   DOUT3(("CFG2: GETFIELD name %s par = %p res = %s", name.c_str(), fPar(), (res ? res : "---")));

   if (res!=0) return res;

   // forth, search for parameter in all parents

   if (fPar.null() && !parname.empty()) {

      Object* obj = fWorker.GetParent();
      // search for parameter in all parents as well
      while (fPar.null() && (obj!=0)) {
         Worker* w = dynamic_cast<Worker*> (obj);
         if (w) fPar = w->Par(parname);
         obj = obj->GetParent();
      }
   }

   res = fPar.GetField(fName.empty() ? "" : name, 0);

   DOUT3(("CFG3: GETFIELD name %s par = %p res = %s", name.c_str(), fPar(), (res ? res : "---")));

   if (res!=0) return res;

   return dabc::RecordContainer::GetField(name, dflt);
}


bool dabc::ConfigContainer::SetField(const std::string& name, const char* value, const char* kind)
{

   if (fReadFlag==0) {
      DOUT3(("*********************** Set for config %s field %s value %s", fName.c_str(), name.c_str(), (value ? value : "---")));
      return dabc::RecordContainer::SetField(name, value, kind);
   }

   // no any field is allowed for change in normal state
   return false;
}
