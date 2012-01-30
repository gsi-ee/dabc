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

#ifndef DABC_ModuleItem
#define DABC_ModuleItem

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#ifndef DABC_Module
#include "dabc/Module.h"
#endif

#ifndef DABC_ModuleException
#include "dabc/ModuleException.h"
#endif

#include <stdint.h>

namespace dabc {

   class Module;

   enum EModuleItemType {
           mitPort,
           mitPool,
           mitParam,
           mitTimer,
           mitCmdQueue
    };

   enum EModuleEvents {
           evntNone = Worker::evntFirstSystem,
           evntInput,
           evntOutput,
           evntPool,
           evntTimeout,
           evntPortConnect,
           evntPortDisconnect
   };

   enum EModelItemConsts {
           moduleitemMinId = 1,         // minimum possible id of item
           moduleitemMaxId = 65534,     // maximum possible id of item
           moduleitemAnyId = 65535      // special id to identify any item (used in WaitForEvent)
   };

   class ModuleItem : public Worker {
      friend class Module;

      protected:

         ModuleItem(int typ, Reference parent, const char* name);

      public:
         virtual ~ModuleItem();

         Module* GetModule() const { return fModule; }
         inline int GetType() const { return fItemType; }
         inline unsigned ItemId() const { return fItemId; }

      protected:
         void SetItemId(unsigned id) { fItemId = id; }

         void ProduceUserEvent(uint16_t evid)
           { fModule->GetUserEvent(this, evid); }

         virtual void DoStart() {}
         virtual void DoStop() {}
         void ThisItemCleaned();

         Module*  fModule;
         int      fItemType;
         unsigned fItemId;
   };

   class ModuleItemException : public ModuleException {
      protected:
         ModuleItemException(ModuleItem* item, const char* info) throw() :
            ModuleException(item->GetModule(), info),
            fItem(item)
         {
         }

         ModuleItem*  fItem;
      public:

         ModuleItem* GetItem() const throw() { return fItem; }
   };


   class ModuleItemRef : public WorkerRef {

      DABC_REFERENCE(ModuleItemRef, WorkerRef, ModuleItem)

   };

}

#endif
