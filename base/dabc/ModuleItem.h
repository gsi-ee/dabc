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

#ifndef DABC_ModuleItem
#define DABC_ModuleItem

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#include <stdint.h>

namespace dabc {

   class Module;
   class ModuleAsync;
   class ModuleSync;

   enum EModuleItemType {
           mitInpPort,   // input port
           mitOutPort,   // output port
           mitPool,      // pool handle
           mitParam,     // parameter
           mitTimer,     // timer
           mitConnTimer, // timer to reconnect ports
           mitUser       // item to produce/receive user-specific events
    };

   enum EModuleEvents {
           evntModuleNone = Worker::evntFirstSystem,
           evntInput,
           evntOutput,
           evntInputReinj,
           evntOutputReinj,
           evntTimeout,
           evntPortConnect,
           evntPortDisconnect,
           evntConnStart,           // event produce when other side of connection is stared
           evntConnStop,            // event produce when other side of connection is stopped
           evntModuleLast,          // last event id, used by Module itself
           evntUser = Worker::evntFirstUser
   };

   enum EModelItemConsts {
           moduleitemMinId = 1,         // minimum possible id of item
           moduleitemMaxId = 65534,     // maximum possible id of item
           moduleitemAnyId = 65535      // special id to identify any item (used in WaitForEvent)
   };

   /** \brief Base class for module items like ports, timers, pool handles
    *
    * \ingroup dabc_all_classes
    *
    * Access to item functionality is possible only via \ref dabc::Module methods
    */

   class ModuleItem : public Worker {
      friend class Module;
      friend class ModuleAsync;
      friend class ModuleSync;

      protected:

         int         fItemType;  // kind of the item
         unsigned    fItemId;    // sequence id of the item in complete items list
         unsigned    fSubId;     // sequence number of input/output/pool/timer port, used by module

         ModuleItem(int typ, Reference parent, const std::string& name);

         virtual bool ItemNeedThread() const { return false; }

         void SetItemId(unsigned id) { fItemId = id; }
         void SetItemSubId(unsigned id) { fSubId = id; }

         virtual void DoStart() {}
         virtual void DoStop() {}

         /** \brief Called when module object is cleaned up - should release all references if any */
         virtual void DoCleanup() {}


      public:
         virtual ~ModuleItem();

         inline int GetType() const { return fItemType; }
         inline unsigned ItemId() const { return fItemId; }
         inline unsigned ItemSubId() const { return fSubId; }
   };


   // _____________________________________________________________________

   /** \brief %Reference on \ref dabc::ModuleItem class
    *
    * \ingroup dabc_all_classes
    */

   class ModuleItemRef : public WorkerRef {
      DABC_REFERENCE(ModuleItemRef, WorkerRef, ModuleItem)

      WorkerRef GetModule();

      unsigned ItemId() const { return GetObject() ? GetObject()->ItemId() : 0; }

      unsigned ItemSubId() const { return GetObject() ? GetObject()->ItemSubId() : 0; }
   };

   // ==================================================================================

    /** \brief Provides timer event to the module
     *
     * \ingroup dabc_all_classes
     *
     * Main aim of this class is to generate periodical or single "shoot" events,
     * which can be used in module for different purposes.
     * If period parameter is positive, timer will produces periodical events.
     * Timer can work in synchrone or asynchrone mode. In first case it will try to produce
     * as many events per second as it should - 1/Period. In case if some events
     * postponed, next events comes faster.
     * In asynchron mode period only specifies minimum distance between two events,
     * therefore number of events per second can be less than 1s/Period.
     * One can also makes single shoots of the timer for activate timer event
     * only once. If delay==0, event will be activated as soon as possible,
     * delay>0 - after specified interval (in seconds). When delay<0, no shoot
     * will be done and previous shoot will be canceled. If SingleShoot() was called
     * several times before event is produced, only last call has an effect.
     */

   class Timer : public ModuleItem {

      friend class Module;

      protected:
         bool        fSysTimer;   //! indicate that timer uses module timeouts
         double      fPeriod;   // period of timer events
         long        fCounter;  // number of generated events
         bool        fActive;   // is timer active
         bool        fSynhron; // indicate if timer tries to keep number of events per second
         double      fInaccuracy; // accumulated inaccuracy of timer in synchron mode
         WorkerRef   fTimerSrc;   // source for the timer

         virtual bool ItemNeedThread() const { return !IsSysTimer(); }

         bool IsSysTimer() const { return fSysTimer; }

         void SingleShoot(double delay) { ActivateTimeout(delay); }

         virtual ~Timer();

         double GetPeriod() const { return fPeriod; }
         void SetPeriod(double v) { fPeriod = v; }

         bool IsSynchron() const { return fSynhron; }
         void SetSynchron(bool on = true) { fSynhron = on; }

         long GetCounter() const { return fCounter; }

         virtual void DoStart();
         virtual void DoStop();
         virtual void DoCleanup() { fTimerSrc.Release(); }

         virtual double ProcessTimeout(double last_diff);

      private:
         Timer(Reference parent, bool systimer, const std::string& name, double timeout = -1., bool synchron = false);

   };

   // ==================================================================================

   /** \brief Special timer to reestablish port connections in the module
    *
    * \ingroup dabc_all_classes
    *
    */

   class ConnTimer : public ModuleItem {
      friend class Module;

      protected:

         std::string fPortName;

         virtual bool ItemNeedThread() const { return true; }

         virtual double ProcessTimeout(double last_diff);

         void Activate(double period) { ActivateTimeout(period); }

      private:

         ConnTimer(Reference parent, const std::string& name, const std::string& portname);
   };

}

#endif
