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

#include "dabc/Factory.h"

#include <dlfcn.h>

#include "dabc/Manager.h"
#include "dabc/DataTransport.h"


// std::vector<dabc::Factory::LibEntry> dabc::Factory::fLibs;

bool dabc::Factory::LoadLibrary(const std::string &fname)
{
   void* lib = dlopen(fname.c_str(), RTLD_NOW | RTLD_GLOBAL);

   if (!lib) {
      EOUT("Cannot load library %s err:%s", fname.c_str(), dlerror());
      return false;
   }

   DOUT1("Library loaded %s", fname.c_str());

   return true;
}

bool dabc::Factory::CreateManager(const std::string &name, Configuration* cfg)
{
   if (dabc::mgr.null())
      new dabc::Manager(name.c_str(), cfg);

   return true;
}


void *dabc::Factory::FindSymbol(const std::string &symbol)
{
   return symbol.empty() ? nullptr : dlsym(RTLD_DEFAULT, symbol.c_str());
}

dabc::Factory::Factory(const std::string &name) :
   Object(nullptr, name)
{
   DOUT2("Factory %s is created", GetName());
}


dabc::Module* dabc::Factory::CreateTransport(const Reference& port, const std::string &typ, dabc::Command cmd)
{
   dabc::PortRef portref = port;

   if (portref.IsInput()) {
      dabc::DataInput* inp = CreateDataInput(typ);
      if (!inp) return nullptr;
      if (!inp->Read_Init(portref, cmd)) {
         EOUT("Input object %s cannot be initialized", typ.c_str());
         delete inp;
         return nullptr;
      }

      auto tr = new dabc::InputTransport(cmd, portref, inp, true);

      dabc::Url url(typ);
      if (url.HasOption("reconnect"))
         tr->EnableReconnect(typ);

      return tr;
   }

   if (portref.IsOutput()) {
      dabc::DataOutput* out = CreateDataOutput(typ);
      if (!out) return nullptr;
      if (!out->Write_Init()) {
         EOUT("Output object %s cannot be initialized", typ.c_str());
         delete out;
         return nullptr;
      }
      DOUT3("Creating output transport for port %p", portref());
      return new dabc::OutputTransport(cmd, portref, out, true);
   }

   return nullptr;
}

// ================================================

dabc::FactoryPlugin::FactoryPlugin(Factory* f)
{
  // for the moment do nothing, later list will be managed here

   dabc::Manager::ProcessFactory(f);
}
