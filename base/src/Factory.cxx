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

#include "dabc/Factory.h"

#include <dlfcn.h>

#include "dabc/Manager.h"
#include "dabc/Port.h"
#include "dabc/DataIO.h"
#include "dabc/DataIOTransport.h"


std::vector<dabc::Factory::LibEntry> dabc::Factory::fLibs;

dabc::Factory* dabc::Factory::NextNewFactory()
{
   dabc::LockGuard lock(FactoriesMutex());
   return Factories()->Size()>0 ? Factories()->Pop() : 0;
}

bool dabc::Factory::LoadLibrary(const std::string& fname)
{
   void* lib = dlopen(fname.c_str(), RTLD_LAZY | RTLD_GLOBAL);

   if (lib==0) {
      EOUT(("Cannot load library %s err:%s", fname.c_str(), dlerror()));
      return false;
   }

   DOUT1(("Library loaded %s", fname.c_str()));

   dabc::LockGuard lock(FactoriesMutex());

   fLibs.push_back(LibEntry(lib, fname));

   return true;
}

bool dabc::Factory::CreateManager(const std::string& name, Configuration* cfg)
{
   if (dabc::mgr.null())
      new dabc::Manager(name.c_str(), cfg);

   return true;
}


void* dabc::Factory::FindSymbol(const std::string& symbol)
{
   if (symbol.empty()) return 0;

   dabc::LockGuard lock(FactoriesMutex());

   for (unsigned n=0;n<fLibs.size();n++) {
      void* res = dlsym(fLibs[n].fLib, symbol.c_str());
      if (res!=0) {
         DOUT3(("Found symbol %s in library %s", symbol.c_str(), fLibs[n].fLibName.c_str()));
         return res;
      }
   }

   return dlsym(RTLD_DEFAULT, symbol.c_str());
}

dabc::Factory::Factory(const char* name) :
   Object(0, name)
{
   DOUT2(("Factory %s is created", name));

   if (dabc::mgr())
      dabc::mgr()->AddFactory(this);
   else {
      dabc::LockGuard lock(FactoriesMutex());
      Factories()->Push(this);
   }
}

dabc::Transport* dabc::Factory::CreateTransport(Reference ref, const char* typ, dabc::Command cmd)
{
   PortRef portref = ref;
   if (portref.null()) {
      EOUT(("Port is not specified"));
      return 0;
   }
   portref.SetTransient(false);

   dabc::DataInput* inp = CreateDataInput(typ);
   if ((inp!=0) && !inp->Read_Init(portref, cmd)) {
      EOUT(("Input object %s cannot be initialized", typ));
      delete inp;
      inp = 0;
   }

   dabc::DataOutput* out = CreateDataOutput(typ);
   if ((out!=0) && !out->Write_Init(portref, cmd)) {
      EOUT(("Output object %s cannot be initialized", typ));
      delete out;
      out = 0;
   }

   if ((inp==0) && (out==0)) return 0;

   return new dabc::DataIOTransport(portref(), inp, out);
}
