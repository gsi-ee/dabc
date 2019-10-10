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

#ifndef DABC_Factory
#define DABC_Factory

#ifndef DABC_Object
#include "dabc/Object.h"
#endif

#ifndef DABC_Queue
#include "dabc/Queue.h"
#endif

#ifndef DABC_Command
#include "dabc/Command.h"
#endif

#ifndef DABC_threads
#include "dabc/threads.h"
#endif

#include <vector>

/** \brief Core framework classes */

namespace dabc {

   class Module;
   class DataInput;
   class DataOutput;
   class Command;
   class Device;
   class Thread;
   class Application;
   class Configuration;
   class Transport;
   class FactoryPlugin;

   /** \brief %Factory for user-specific classes
    *
    * \ingroup dabc_user_classes
    * \ingroup dabc_all_classes
    *
    * Factory class provides interfaces to add user-specific
    *    classes to the DABC code
    */

   class Factory : public Object {
      friend class Manager;
      friend class FactoryPlugin;

      public:
         Factory(const std::string &name);

         /** Factory method to create application */
         virtual Application* CreateApplication(const std::string &classname, Command cmd) { return nullptr; }

         /** Factory method to create object */
         virtual Reference CreateObject(const std::string &classname, const std::string &objname, Command cmd) { return nullptr; }

         /** Factory method to create device */
         virtual Device* CreateDevice(const std::string &classname, const std::string &devname, Command cmd) { return nullptr; }

         /** Factory method to create thread */
         virtual Reference CreateThread(Reference parent, const std::string &classname, const std::string &thrdname, const std::string &thrddev, Command cmd) { return Reference(); }

         /** Factory method to create module */
         virtual Module* CreateModule(const std::string &classname, const std::string &modulename, Command cmd) { return nullptr; }

         /** Factory method to create transport */
         virtual Module* CreateTransport(const Reference& port, const std::string &typ, Command cmd);

         /** Factory method to create data input */
         virtual DataInput* CreateDataInput(const std::string &typ) { return nullptr; }

         /** Factory method to create data output */
         virtual DataOutput* CreateDataOutput(const std::string &typ) { return nullptr; }

         /** Factory method to create arbitrary object kind */
         virtual void* CreateAny(const std::string &classname, const std::string &objname, Command cmd) { return nullptr; }

         static bool LoadLibrary(const std::string &fname);

         static void* FindSymbol(const std::string &symbol);

         static bool CreateManager(const std::string &name = "mgr", Configuration* cfg = nullptr);

         virtual const char* ClassName() const { return "Factory"; }

      protected:

         /** Method called by the manager during application start.
          * One can put arbitrary initialization code here - for instance, create some control instances */
         virtual void Initialize() {}
   };


   // ==============================================================================

   /** \brief Small helper class to correctly instantiate user-specific factories
    *
    * In some of implementation files should stay:
    *
    *     dabc::FactoryPlugin user_plugin(new UserFactory);
    */

   class FactoryPlugin {
      public:
         FactoryPlugin(dabc::Factory* f);
   };

}

#endif
